/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/
#include "hphp/runtime/ext/apc/ext_apc.h"

#include <fstream>

#ifndef _MSC_VER
#include <dlfcn.h>
#endif
#include <memory>
#include <set>
#include <vector>
#include <stdexcept>
#include <type_traits>

#include <folly/portability/SysTime.h>

#include "hphp/util/alloc.h"
#include "hphp/util/async-job.h"
#include "hphp/util/boot-stats.h"
#include "hphp/util/hdf.h"
#include "hphp/util/logger.h"

#include "hphp/runtime/base/apc-file-storage.h"
#include "hphp/runtime/base/array-init.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/base/comparisons.h"
#include "hphp/runtime/base/concurrent-shared-store.h"
#include "hphp/runtime/base/config.h"
#include "hphp/runtime/base/execution-context.h"
#include "hphp/runtime/base/ini-setting.h"
#include "hphp/runtime/base/program-functions.h"
#include "hphp/runtime/base/runtime-option.h"
#include "hphp/runtime/base/variable-serializer.h"
#include "hphp/runtime/ext/apc/snapshot-builder.h"
#include "hphp/runtime/ext/fb/ext_fb.h"
#include "hphp/runtime/server/cli-server.h"

using HPHP::ScopedMem;

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

namespace {

std::aligned_storage<
  sizeof(ConcurrentTableSharedStore),
  alignof(ConcurrentTableSharedStore)
>::type s_apc_storage;

using UserAPCCache = folly::AtomicHashMap<uid_t, ConcurrentTableSharedStore*>;

std::aligned_storage<
  sizeof(UserAPCCache),
  alignof(UserAPCCache)
>::type s_user_apc_storage;

UserAPCCache& apc_store_local() {
  void* vpUserStore = &s_user_apc_storage;
  return *static_cast<UserAPCCache*>(vpUserStore);
}

ConcurrentTableSharedStore& apc_store_local(uid_t uid) {
  auto& cache = apc_store_local();
  auto iter = cache.find(uid);
  if (iter != cache.end()) return *(iter->second);
  auto table = new ConcurrentTableSharedStore;
  auto res = cache.insert(uid, table);
  if (!res.second) delete table;
  return *res.first->second;
}

ConcurrentTableSharedStore& apc_store() {
  if (UNLIKELY(!RuntimeOption::RepoAuthoritative &&
               RuntimeOption::EvalUnixServerQuarantineApc)) {
    if (auto uc = get_cli_ucred()) {
      return apc_store_local(uc->uid);
    }
  }
  void* vpStore = &s_apc_storage;
  return *static_cast<ConcurrentTableSharedStore*>(vpStore);
}

}

//////////////////////////////////////////////////////////////////////

void initialize_apc() {
  APCStats::Create();
  // Note: we never destruct APC, currently.
  void* vpStore = &s_apc_storage;
  new (vpStore) ConcurrentTableSharedStore;

  if (UNLIKELY(!RuntimeOption::RepoAuthoritative &&
               RuntimeOption::EvalUnixServerQuarantineApc)) {
    new (&s_user_apc_storage) UserAPCCache(10);
  }
}

//////////////////////////////////////////////////////////////////////

const StaticString
  s_delete("delete");

extern void const_load();

typedef ConcurrentTableSharedStore::KeyValuePair KeyValuePair;
typedef ConcurrentTableSharedStore::DumpMode DumpMode;

static void* keep_alive;

void apcExtension::moduleLoad(const IniSetting::Map& ini, Hdf config) {
  if (!keep_alive && ini.isString()) {
    // this is a hack to preserve some dynamic entry points
    switch (ini.toString().size()) {
      case 0: keep_alive = (void*)const_load; break;
      case 2: keep_alive = (void*)const_load_impl_compressed; break;
      case 4: keep_alive = (void*)apc_load_impl_compressed; break;
    }
  }

  Config::Bind(Enable, ini, config, "Server.APC.EnableApc", true);
  Config::Bind(EnableConstLoad, ini, config, "Server.APC.EnableConstLoad",
               false);
  Config::Bind(ForceConstLoadToAPC, ini, config,
               "Server.APC.ForceConstLoadToAPC", true);
  Config::Bind(PrimeLibrary, ini, config, "Server.APC.PrimeLibrary");
  Config::Bind(LoadThread, ini, config, "Server.APC.LoadThread", 15);
  Config::Bind(CompletionKeys, ini, config, "Server.APC.CompletionKeys");
  Config::Bind(EnableApcSerialize, ini, config, "Server.APC.EnableApcSerialize",
               true);
  Config::Bind(ExpireOnSets, ini, config, "Server.APC.ExpireOnSets");
  Config::Bind(PurgeFrequency, ini, config, "Server.APC.PurgeFrequency", 4096);
  Config::Bind(PurgeRate, ini, config, "Server.APC.PurgeRate", -1);

  Config::Bind(AllowObj, ini, config, "Server.APC.AllowObject");
  Config::Bind(TTLLimit, ini, config, "Server.APC.TTLLimit", -1);
  // Any TTL > TTLMaxFinite will be made infinite. NB: Applied *after* TTLLimit.
  Config::Bind(TTLMaxFinite, ini, config, "Server.APC.TTLMaxFinite",
               std::numeric_limits<int64_t>::max());
  Config::Bind(HotPrefix, ini, config, "Server.APC.HotPrefix");
  Config::Bind(HotSize, ini, config, "Server.APC.HotSize", 30000);
  Config::Bind(HotLoadFactor, ini, config, "Server.APC.HotLoadFactor", 0.5);
  Config::Bind(HotKeyAllocLow, ini, config, "Server.APC.HotKeyAllocLow", false);
  Config::Bind(HotMapAllocLow, ini, config, "Server.APC.HotMapAllocLow", false);

  // Loads .so PrimeLibrary, writes snapshot output to this file, then exits.
  Config::Bind(PrimeLibraryUpgradeDest, ini, config,
               "Server.APC.PrimeLibraryUpgradeDest");

  // FileStorage
  Config::Bind(UseFileStorage, ini, config, "Server.APC.FileStorage.Enable");
  FileStorageChunkSize = Config::GetInt64(ini, config,
                                          "Server.APC.FileStorage.ChunkSize",
                                          1LL << 29);
  Config::Bind(FileStoragePrefix, ini, config, "Server.APC.FileStorage.Prefix",
               "/tmp/apc_store");
  Config::Bind(FileStorageFlagKey, ini, config,
               "Server.APC.FileStorage.FlagKey", "_madvise_out");
  Config::Bind(FileStorageAdviseOutPeriod, ini, config,
               "Server.APC.FileStorage.AdviseOutPeriod", 1800);
  Config::Bind(FileStorageKeepFileLinked, ini, config,
               "Server.APC.FileStorage.KeepFileLinked");

#ifdef NO_M_DATA
  Config::Bind(UseUncounted, ini, config, "Server.APC.MemModelTreadmill", true);
#else
  Config::Bind(UseUncounted, ini, config, "Server.APC.MemModelTreadmill",
               RuntimeOption::ServerExecutionMode());
#endif
  Config::Bind(ShareUncounted, ini, config, "Server.APC.ShareUncounted", true);
  if (!UseUncounted && ShareUncounted) ShareUncounted = false;

  IniSetting::Bind(this, IniSetting::PHP_INI_SYSTEM, "apc.enabled", &Enable);
  IniSetting::Bind(this, IniSetting::PHP_INI_SYSTEM, "apc.stat",
                   RuntimeOption::RepoAuthoritative ? "0" : "1", &Stat);
  IniSetting::Bind(this, IniSetting::PHP_INI_SYSTEM, "apc.enable_cli",
                   &EnableCLI);
}

void apcExtension::moduleInit() {
#ifdef NO_M_DATA
  if (!UseUncounted) {
    Logger::Error("Server.APC.MemModelTreadmill=false ignored in lowptr build");
    UseUncounted = true;
  }
#endif // NO_M_DATA
  if (UseFileStorage) {
    // We use 32 bits to represent offset into a chunk, so don't make it too
    // large.
    constexpr int64_t MaxChunkSize = 1LL << 31;
    if (FileStorageChunkSize > MaxChunkSize) {
      Logger::Warning("Server.APC.FileStorage.ChunkSize too large, "
                      "resetting to %" PRId64, MaxChunkSize);
      FileStorageChunkSize = MaxChunkSize;
    }
    s_apc_file_storage.enable(FileStoragePrefix, FileStorageChunkSize);
  }

  HHVM_RC_INT(APC_ITER_TYPE, 0x1);
  HHVM_RC_INT(APC_ITER_KEY, 0x2);
  HHVM_RC_INT(APC_ITER_FILENAME, 0x4);
  HHVM_RC_INT(APC_ITER_DEVICE, 0x8);
  HHVM_RC_INT(APC_ITER_INODE, 0x10);
  HHVM_RC_INT(APC_ITER_VALUE, 0x20);
  HHVM_RC_INT(APC_ITER_MD5, 0x40);
  HHVM_RC_INT(APC_ITER_NUM_HITS, 0x80);
  HHVM_RC_INT(APC_ITER_MTIME, 0x100);
  HHVM_RC_INT(APC_ITER_CTIME, 0x200);
  HHVM_RC_INT(APC_ITER_DTIME, 0x400);
  HHVM_RC_INT(APC_ITER_ATIME, 0x800);
  HHVM_RC_INT(APC_ITER_REFCOUNT, 0x1000);
  HHVM_RC_INT(APC_ITER_MEM_SIZE, 0x2000);
  HHVM_RC_INT(APC_ITER_TTL, 0x4000);
  HHVM_RC_INT(APC_ITER_NONE, 0x0);
  HHVM_RC_INT(APC_ITER_ALL, 0xFFFFFFFFFF);
  HHVM_RC_INT(APC_LIST_ACTIVE, 1);
  HHVM_RC_INT(APC_LIST_DELETED, 2);

  HHVM_FE(apc_add);
  HHVM_FE(apc_store);
  HHVM_FE(apc_store_as_primed_do_not_use);
  HHVM_FE(apc_fetch);
  HHVM_FE(apc_delete);
  HHVM_FE(apc_clear_cache);
  HHVM_FE(apc_inc);
  HHVM_FE(apc_dec);
  HHVM_FE(apc_cas);
  HHVM_FE(apc_exists);
  HHVM_FE(apc_size);
  HHVM_FE(apc_cache_info);
  HHVM_FE(apc_sma_info);
  loadSystemlib();
}

void apcExtension::moduleShutdown() {
  if (UseFileStorage) {
    s_apc_file_storage.cleanup();
  }
}


bool apcExtension::Enable = true;
bool apcExtension::EnableConstLoad = false;
bool apcExtension::ForceConstLoadToAPC = true;
std::string apcExtension::PrimeLibrary;
int apcExtension::LoadThread = 15;
std::set<std::string> apcExtension::CompletionKeys;
bool apcExtension::EnableApcSerialize = true;
bool apcExtension::ExpireOnSets = false;
int apcExtension::PurgeFrequency = 4096;
int apcExtension::PurgeRate = -1;
bool apcExtension::AllowObj = false;
int apcExtension::TTLLimit = -1;
int64_t apcExtension::TTLMaxFinite = std::numeric_limits<int64_t>::max();
int apcExtension::HotSize = 30000;
double apcExtension::HotLoadFactor = 0.5;
std::vector<std::string> apcExtension::HotPrefix;
bool apcExtension::HotKeyAllocLow = false;
bool apcExtension::HotMapAllocLow = false;
std::string apcExtension::PrimeLibraryUpgradeDest;
bool apcExtension::UseFileStorage = false;
int64_t apcExtension::FileStorageChunkSize = int64_t(1LL << 29);
std::string apcExtension::FileStoragePrefix = "/tmp/apc_store";
int apcExtension::FileStorageAdviseOutPeriod = 1800;
std::string apcExtension::FileStorageFlagKey = "_madvise_out";
bool apcExtension::FileStorageKeepFileLinked = false;
#ifdef NO_M_DATA
bool apcExtension::UseUncounted = true;
#else
bool apcExtension::UseUncounted = false;
#endif
bool apcExtension::ShareUncounted = true;
bool apcExtension::Stat = true;
// Different from zend default but matches what we've been returning for years
bool apcExtension::EnableCLI = true;

static apcExtension s_apc_extension;

Variant HHVM_FUNCTION(apc_store,
                      const Variant& key_or_array,
                      const Variant& var /* = null */,
                      int64_t ttl /* = 0 */) {
  if (!apcExtension::Enable) return Variant(false);

  if (key_or_array.isArray()) {
    Array valuesArr = key_or_array.toArray();

    for (ArrayIter iter(valuesArr); iter; ++iter) {
      Variant key = iter.first();
      if (!key.isString()) {
        throw_invalid_argument("apc key: (not a string)");
        return Variant(false);
      }
      Variant v = iter.second();
      apc_store().set(key.toString(), v, ttl);
    }

    return Variant(staticEmptyArray());
  }

  if (!key_or_array.isString()) {
    throw_invalid_argument("apc key: (not a string)");
    return Variant(false);
  }
  String strKey = key_or_array.toString();
  apc_store().set(strKey, var, ttl);
  return Variant(true);
}

/**
 * Stores the key in a similar fashion as "priming" would do (no TTL limit).
 * Using this function is equivalent to adding your key to apc_prime.so.
 */
bool HHVM_FUNCTION(apc_store_as_primed_do_not_use,
                   const String& key,
                   const Variant& var) {
  if (!apcExtension::Enable) return false;
  apc_store().setWithoutTTL(key, var);
  return true;
}

Variant HHVM_FUNCTION(apc_add,
                      const Variant& key_or_array,
                      const Variant& var /* = null */,
                      int64_t ttl /* = 0 */) {
  if (!apcExtension::Enable) return false;

  if (key_or_array.isArray()) {
    Array valuesArr = key_or_array.toArray();

    // errors stores all keys corresponding to entries that could not be cached
    ArrayInit errors(valuesArr.size(), ArrayInit::Map{});

    for (ArrayIter iter(valuesArr); iter; ++iter) {
      Variant key = iter.first();
      if (!key.isString()) {
        throw_invalid_argument("apc key: (not a string)");
        return false;
      }
      Variant v = iter.second();
      if (!apc_store().add(key.toString(), v, ttl)) {
        errors.add(key, -1);
      }
    }
    return errors.toVariant();
  }

  if (!key_or_array.isString()) {
    throw_invalid_argument("apc key: (not a string)");
    return false;
  }
  String strKey = key_or_array.toString();
  return apc_store().add(strKey, var, ttl);
}

TypedValue HHVM_FUNCTION(apc_fetch, const Variant& key, bool& success) {
  if (!apcExtension::Enable) return make_tv<KindOfBoolean>(false);

  Variant v;

  if (key.isArray()) {
    bool tmp = false;
    Array keys = key.toArray();
    ArrayInit init(keys.size(), ArrayInit::Map{});
    for (ArrayIter iter(keys); iter; ++iter) {
      Variant k = iter.second();
      if (!k.isString()) {
        throw_invalid_argument("apc key: (not a string)");
        return make_tv<KindOfBoolean>(false);
      }
      String strKey = k.toString();
      if (apc_store().get(strKey, v)) {
        tmp = true;
        init.set(strKey, v);
      }
    }
    success = tmp;
    return tvReturn(init.toVariant());
  }

  if (apc_store().get(key.toString(), v)) {
    success = true;
  } else {
    success = false;
    v = false;
  }
  return tvReturn(std::move(v));
}

Variant HHVM_FUNCTION(apc_delete,
                      const Variant& key) {
  if (!apcExtension::Enable) return false;

  if (key.isArray()) {
    Array keys = key.toArray();
    PackedArrayInit init(keys.size());
    for (ArrayIter iter(keys); iter; ++iter) {
      Variant k = iter.second();
      if (!k.isString()) {
        raise_warning("apc key is not a string");
        init.append(k);
      } else if (!apc_store().eraseKey(k.toCStrRef())) {
        init.append(k);
      }
    }
    return init.toVariant();
  } else if(key.is(KindOfObject)) {
    if (!key.getObjectData()->getVMClass()->
         classof(SystemLib::s_APCIteratorClass)) {
      raise_error(
        "apc_delete(): apc_delete object argument must be instance"
        " of APCIterator"
      );
      return false;
    }
    const Func* method =
      SystemLib::s_APCIteratorClass->lookupMethod(s_delete.get());
    return Variant::attach(
      g_context->invokeFuncFew(method, key.getObjectData())
    );
  }

  return apc_store().eraseKey(key.toString());
}

bool HHVM_FUNCTION(apc_clear_cache, const String& /*cache_type*/ /* = "" */) {
  if (!apcExtension::Enable) return false;
  return apc_store().clear();
}

Variant HHVM_FUNCTION(apc_inc,
                      const String& key,
                      int64_t step /* = 1 */,
                      VRefParam success /* = null */) {
  if (!apcExtension::Enable) return false;

  bool found = false;
  int64_t newValue = apc_store().inc(key, step, found);
  success.assignIfRef(found);
  if (!found) return false;
  return newValue;
}

Variant HHVM_FUNCTION(apc_dec,
                      const String& key,
                      int64_t step /* = 1 */,
                      VRefParam success /* = null */) {
  if (!apcExtension::Enable) return false;

  bool found = false;
  int64_t newValue = apc_store().inc(key, -step, found);
  success.assignIfRef(found);
  if (!found) return false;
  return newValue;
}

bool HHVM_FUNCTION(apc_cas,
                   const String& key,
                   int64_t old_cas,
                   int64_t new_cas) {
  if (!apcExtension::Enable) return false;
  return apc_store().cas(key, old_cas, new_cas);
}

Variant HHVM_FUNCTION(apc_exists,
                      const Variant& key) {
  if (!apcExtension::Enable) return false;

  if (key.isArray()) {
    Array keys = key.toArray();
    PackedArrayInit init(keys.size());
    for (ArrayIter iter(keys); iter; ++iter) {
      Variant k = iter.second();
      if (!k.isString()) {
        throw_invalid_argument("apc key: (not a string)");
        return false;
      }
      String strKey = k.toString();
      if (apc_store().exists(strKey)) {
        init.append(strKey);
      }
    }
    return init.toVariant();
  }

  return apc_store().exists(key.toString());
}

TypedValue HHVM_FUNCTION(apc_size, const String& key) {
  if (!apcExtension::Enable) return make_tv<KindOfNull>();

  bool found = false;
  int64_t size = apc_store().size(key, found);

  return found ? make_tv<KindOfInt64>(size) : make_tv<KindOfNull>();
}

const StaticString s_user("user");
const StaticString s_start_time("start_time");
const StaticString s_ttl("ttl");
const StaticString s_cache_list("cache_list");
const StaticString s_info("info");
const StaticString s_in_memory("in_memory");
const StaticString s_mem_size("mem_size");
const StaticString s_type("type");
const StaticString s_c_time("creation_time");
const StaticString s_mtime("mtime");

// This is a guess to the size of the info array. It is significantly
// bigger than what we need but hard to control all the info that we
// may want to add here.
// Try to keep it such that we do not have to resize the array
const uint32_t kCacheInfoSize = 40;
// Number of elements in the entry array
const int32_t kEntryInfoSize = 7;

Variant HHVM_FUNCTION(apc_cache_info,
                      const String& cache_type,
                      bool limited /* = false */) {
  ArrayInit info(kCacheInfoSize, ArrayInit::Map{});
  info.add(s_start_time, start_time());
  if (cache_type.size() != 0 && !cache_type.same(s_user)) {
    return info.toArray();
  }

  info.add(s_ttl, apcExtension::TTLLimit);

  std::map<const StringData*, int64_t> stats;
  APCStats::getAPCStats().collectStats(stats);
  for (auto it = stats.begin(); it != stats.end(); it++) {
    info.add(Variant(it->first, Variant::PersistentStrInit{}), it->second);
  }
  if (!limited) {
    auto const entries = apc_store().getEntriesInfo();
    PackedArrayInit ents(entries.size());
    for (auto& entry : entries) {
      ArrayInit ent(kEntryInfoSize, ArrayInit::Map{});
      ent.add(s_info,
              Variant::attach(StringData::Make(entry.key.c_str())));
      ent.add(s_in_memory, entry.inMem);
      ent.add(s_ttl, entry.ttl);
      ent.add(s_mem_size, entry.size);
      ent.add(s_type, static_cast<int64_t>(entry.type));
      ent.add(s_c_time, entry.c_time);
      ent.add(s_mtime, entry.mtime);
      ents.append(ent.toArray());
    }
    info.add(s_cache_list, ents.toArray(), false);
  }
  return info.toArray();
}

Array HHVM_FUNCTION(apc_sma_info, bool /*limited*/ /* = false */) {
  return empty_darray();
}

///////////////////////////////////////////////////////////////////////////////
// loading APC from archive files

typedef void(*PFUNC_APC_LOAD)();

// Structure to hold cache meta data
// Same definition in ext_apc.cpp
struct cache_info {
  char *a_name;
  bool use_const;
};

static Mutex dl_mutex;
static PFUNC_APC_LOAD apc_load_func(void *handle, const char *name) {
#ifdef _MSC_VER
  throw Exception("apc_load_func is not currently supported under MSVC!");
#else
  PFUNC_APC_LOAD p = (PFUNC_APC_LOAD)dlsym(handle, name);
  if (p == nullptr) {
    throw Exception("Unable to find %s in %s", name,
                    apcExtension::PrimeLibrary.c_str());
  }
  return p;
#endif
}

struct ApcLoadJob {
  ApcLoadJob(void *handle, int index) : m_handle(handle), m_index(index) {}
  void *m_handle; int m_index;
};

struct ApcLoadWorker {
  void onThreadEnter() {
    g_context.getCheck();
  }
  void doJob(std::shared_ptr<ApcLoadJob> job) {
    char func_name[128];
    MemoryManager::SuppressOOM so(*tl_heap);
    snprintf(func_name, sizeof(func_name), "_apc_load_%d", job->m_index);
    apc_load_func(job->m_handle, func_name)();
  }
  void onThreadExit() {
    hphp_memory_cleanup();
  }
};

static size_t s_const_map_size = 0;

static SnapshotBuilder s_snapshotBuilder;

void apc_load(int thread) {
#ifndef _MSC_VER
  static void *handle = nullptr;
  if (handle ||
      apcExtension::PrimeLibrary.empty() ||
      !apcExtension::Enable) {
    return;
  }
  BootStats::Block timer("loading APC data",
                         RuntimeOption::ServerExecutionMode());
  if (apc_store().primeFromSnapshot(apcExtension::PrimeLibrary.c_str())) {
    return;
  }
  Logger::Info("Fall back to shared object format");
  handle = dlopen(apcExtension::PrimeLibrary.c_str(), RTLD_LAZY);
  if (!handle) {
    throw Exception("Unable to open apc prime library %s: %s",
                    apcExtension::PrimeLibrary.c_str(), dlerror());
  }

  auto upgradeDest = apcExtension::PrimeLibraryUpgradeDest;
  if (!upgradeDest.empty()) {
    thread = 1; // SnapshotBuilder is not (yet) thread-safe.
    // TODO(9755792): Ensure APCFileStorage is enabled.
  }

  if (thread <= 1) {
    apc_load_func(handle, "_apc_load_all")();
  } else {
    int count = ((int(*)())apc_load_func(handle, "_apc_load_count"))();

    std::vector<std::shared_ptr<ApcLoadJob>> jobs;
    jobs.reserve(count);
    for (int i = 0; i < count; i++) {
      jobs.push_back(std::make_shared<ApcLoadJob>(handle, i));
    }
    JobDispatcher<ApcLoadJob, ApcLoadWorker>(std::move(jobs), thread).run();
  }

  apc_store().primeDone();
  if (!upgradeDest.empty()) {
    s_snapshotBuilder.writeToFile(upgradeDest);
  }

  // We've copied all the data out, so close it out.
  dlclose(handle);
#endif
}

void apc_advise_out() {
  apc_store().adviseOut();
}

size_t get_const_map_size() {
  return s_const_map_size;
}

///////////////////////////////////////////////////////////////////////////////
// Constant and APC priming (always with compressed data).

EXTERNALLY_VISIBLE
void const_load_impl_compressed(
  struct cache_info* /*info*/, int* /*int_lens*/, const char* /*int_keys*/,
  long long* /*int_values*/, int* /*char_lens*/, const char* /*char_keys*/,
  char* /*char_values*/, int* /*string_lens*/, const char* /*strings*/,
  int* /*object_lens*/, const char* /*objects*/, int* /*thrift_lens*/,
  const char* /*thrifts*/, int* /*other_lens*/, const char* /*others*/) {
  // TODO(8117903): Unused; remove after updating www side.
}

EXTERNALLY_VISIBLE
void apc_load_impl_compressed
    (struct cache_info *info,
     int *int_lens, const char *int_keys, long long *int_values,
     int *char_lens, const char *char_keys, char *char_values,
     int *string_lens, const char *strings,
     int *object_lens, const char *objects,
     int *thrift_lens, const char *thrifts,
     int *other_lens, const char *others) {
  bool readOnly = apcExtension::EnableConstLoad && info && info->use_const;
  if (readOnly && info->a_name) Logger::FInfo("const archive {}", info->a_name);
  auto& s = apc_store();
  SnapshotBuilder* snap = apcExtension::PrimeLibraryUpgradeDest.empty() ?
    nullptr : &s_snapshotBuilder;
  {
    int count = int_lens[0];
    int len = int_lens[1];
    if (count) {
      std::vector<KeyValuePair> vars(count);
      char *keys = gzdecode(int_keys, len);
      if (keys == nullptr) throw Exception("bad compressed apc archive.");
      ScopedMem holder(keys);
      const char *k = keys;
      long long* v = int_values;
      for (int i = 0; i < count; i++) {
        auto& item = vars[i];
        item.key = k;
        item.readOnly = readOnly;
        s.constructPrime(*v++, item);
        if (UNLIKELY(snap != nullptr)) snap->addInt(v[-1], item);
        k += int_lens[i + 2] + 1; // skip \0
      }
      s.prime(std::move(vars));
      assertx((k - keys) == len);
    }
  }
  {
    int count = char_lens[0];
    int len = char_lens[1];
    if (count) {
      std::vector<KeyValuePair> vars(count);
      char *keys = gzdecode(char_keys, len);
      if (keys == nullptr) throw Exception("bad compressed apc archive.");
      ScopedMem holder(keys);
      const char *k = keys;
      char *v = char_values;
      for (int i = 0; i < count; i++) {
        auto& item = vars[i];
        item.key = k;
        item.readOnly = readOnly;
        switch (*v++) {
          case 0:
            s.constructPrime(false, item);
            if (UNLIKELY(snap != nullptr)) snap->addFalse(item);
            break;
          case 1:
            s.constructPrime(true, item);
            if (UNLIKELY(snap != nullptr)) snap->addTrue(item);
            break;
          case 2:
            s.constructPrime(uninit_null(), item);
            if (UNLIKELY(snap != nullptr)) snap->addNull(item);
            break;
        default:
          throw Exception("bad apc archive, unknown char type");
        }
        k += char_lens[i + 2] + 1; // skip \0
      }
      s.prime(std::move(vars));
      assertx((k - keys) == len);
    }
  }
  {
    int count = string_lens[0] / 2;
    int len = string_lens[1];
    if (count) {
      std::vector<KeyValuePair> vars(count);
      char *decoded = gzdecode(strings, len);
      if (decoded == nullptr) throw Exception("bad compressed apc archive.");
      ScopedMem holder(decoded);
      const char *p = decoded;
      for (int i = 0; i < count; i++) {
        auto& item = vars[i];
        item.key = p;
        item.readOnly = readOnly;
        p += string_lens[i + i + 2] + 1; // skip \0
        // Strings would be copied into APC anyway.
        String value(p, string_lens[i + i + 3], CopyString);
        // todo: t2539893: check if value is already a static string
        s.constructPrime(value, item, false);
        if (UNLIKELY(snap != nullptr)) snap->addString(value, item);
        p += string_lens[i + i + 3] + 1; // skip \0
      }
      s.prime(std::move(vars));
      assertx((p - decoded) == len);
    }
  }
  {
    int count = object_lens[0] / 2;
    int len = object_lens[1];
    if (count) {
      std::vector<KeyValuePair> vars(count);
      char *decoded = gzdecode(objects, len);
      if (decoded == nullptr) throw Exception("bad compressed APC archive.");
      ScopedMem holder(decoded);
      const char *p = decoded;
      for (int i = 0; i < count; i++) {
        auto& item = vars[i];
        item.key = p;
        item.readOnly = readOnly;
        p += object_lens[i + i + 2] + 1; // skip \0
        String value(p, object_lens[i + i + 3], CopyString);
        s.constructPrime(value, item, true);
        if (UNLIKELY(snap != nullptr)) snap->addObject(value, item);
        p += object_lens[i + i + 3] + 1; // skip \0
      }
      s.prime(std::move(vars));
      assertx((p - decoded) == len);
    }
  }
  {
    int count = thrift_lens[0] / 2;
    int len = thrift_lens[1];
    if (count) {
      std::vector<KeyValuePair> vars(count);
      char *decoded = gzdecode(thrifts, len);
      if (decoded == nullptr) throw Exception("bad compressed apc archive.");
      ScopedMem holder(decoded);
      const char *p = decoded;
      for (int i = 0; i < count; i++) {
        auto& item = vars[i];
        item.key = p;
        item.readOnly = readOnly;
        p += thrift_lens[i + i + 2] + 1; // skip \0
        String value(p, thrift_lens[i + i + 3], CopyString);
        Variant success;
        Variant v = HHVM_FN(fb_unserialize)(value, ref(success));
        if (same(success, false)) {
          throw Exception("bad apc archive, fb_unserialize failed");
        }
        s.constructPrime(v, item);
        if (UNLIKELY(snap != nullptr)) snap->addThrift(value, item);
        p += thrift_lens[i + i + 3] + 1; // skip \0
      }
      s.prime(std::move(vars));
      assertx((p - decoded) == len);
    }
  }
  {
    int count = other_lens[0] / 2;
    int len = other_lens[1];
    if (count) {
      std::vector<KeyValuePair> vars(count);
      char *decoded = gzdecode(others, len);
      if (decoded == nullptr) throw Exception("bad compressed apc archive.");
      ScopedMem holder(decoded);
      const char *p = decoded;
      for (int i = 0; i < count; i++) {
        auto& item = vars[i];
        item.key = p;
        item.readOnly = readOnly;
        p += other_lens[i + i + 2] + 1; // skip \0
        String value(p, other_lens[i + i + 3], CopyString);
        Variant v =
          unserialize_from_string(value, VariableUnserializer::Type::Internal);
        if (same(v, false)) {
          // we can't possibly get here if it was a boolean "false" that's
          // supposed to be serialized as a char
          throw Exception("bad apc archive, unserialize_from_string failed");
        }
        s.constructPrime(v, item);
        if (UNLIKELY(snap != nullptr)) snap->addOther(value, item);
        p += other_lens[i + i + 3] + 1; // skip \0
      }
      s.prime(std::move(vars));
      assertx((p - decoded) == len);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

static double my_time() {
  struct timeval a;
  double t;
  gettimeofday(&a, nullptr);
  t = a.tv_sec + (a.tv_usec/1000000.00);
  return t;
}

const StaticString
  s_total("total"),
  s_current("current"),
  s_filename("filename"),
  s_name("name"),
  s_done("done"),
  s_temp_filename("temp_filename"),
  s_cancel_upload("cancel_upload"),
  s_rate("rate");

#define RFC1867_TRACKING_KEY_MAXLEN 63
#define RFC1867_NAME_MAXLEN 63
#define RFC1867_FILENAME_MAXLEN 127

int apc_rfc1867_progress(apc_rfc1867_data* rfc1867ApcData, unsigned int event,
                         void* event_data, void** /*extra*/) {
  switch (event) {
  case MULTIPART_EVENT_START: {
    multipart_event_start *data = (multipart_event_start *) event_data;
    rfc1867ApcData->content_length = data->content_length;
    rfc1867ApcData->tracking_key.clear();
    rfc1867ApcData->name.clear();
    rfc1867ApcData->cancel_upload = 0;
    rfc1867ApcData->temp_filename = "";
    rfc1867ApcData->start_time = my_time();
    rfc1867ApcData->bytes_processed = 0;
    rfc1867ApcData->prev_bytes_processed = 0;
    rfc1867ApcData->rate = 0;
    rfc1867ApcData->update_freq = RuntimeOption::Rfc1867Freq;

    if (rfc1867ApcData->update_freq < 0) {
      assertx(false); // TODO: support percentage
      // frequency is a percentage, not bytes
      rfc1867ApcData->update_freq =
        rfc1867ApcData->content_length * RuntimeOption::Rfc1867Freq / 100;
    }
    break;
  }

  case MULTIPART_EVENT_FORMDATA: {
    multipart_event_formdata *data = (multipart_event_formdata *)event_data;
    if (data->name &&
        !strncasecmp(data->name, RuntimeOption::Rfc1867Name.c_str(),
                     RuntimeOption::Rfc1867Name.size()) &&
        data->value && data->length &&
        data->length < RFC1867_TRACKING_KEY_MAXLEN -
                       RuntimeOption::Rfc1867Prefix.size()) {
      int len = RuntimeOption::Rfc1867Prefix.size();
      if (len > RFC1867_TRACKING_KEY_MAXLEN) {
        len = RFC1867_TRACKING_KEY_MAXLEN;
      }
      rfc1867ApcData->tracking_key =
        std::string(RuntimeOption::Rfc1867Prefix.c_str(), len);
      len = strlen(*data->value);
      int rem = RFC1867_TRACKING_KEY_MAXLEN -
                rfc1867ApcData->tracking_key.size();
      if (len > rem) len = rem;
      rfc1867ApcData->tracking_key +=
        std::string(*data->value, len);
      rfc1867ApcData->bytes_processed = data->post_bytes_processed;
    }
    /* Facebook: Temporary fix for a bug in PHP's rfc1867 code,
       fixed here for convenience:
       http://cvs.php.net/viewvc.cgi/php-src/main/
       rfc1867.c?r1=1.173.2.1.2.11&r2=1.173.2.1.2.12 */
    (*data->newlength) = data->length;
    break;
  }

  case MULTIPART_EVENT_FILE_START:
    if (!rfc1867ApcData->tracking_key.empty()) {
      multipart_event_file_start *data =
        (multipart_event_file_start *)event_data;

      rfc1867ApcData->bytes_processed = data->post_bytes_processed;
      int len = strlen(*data->filename);
      if (len > RFC1867_FILENAME_MAXLEN) len = RFC1867_FILENAME_MAXLEN;
      rfc1867ApcData->filename = std::string(*data->filename, len);
      rfc1867ApcData->temp_filename = "";
      len = strlen(data->name);
      if (len > RFC1867_NAME_MAXLEN) len = RFC1867_NAME_MAXLEN;
      rfc1867ApcData->name = std::string(data->name, len);
      ArrayInit track(6, ArrayInit::Map{});
      track.set(s_total, rfc1867ApcData->content_length);
      track.set(s_current, rfc1867ApcData->bytes_processed);
      track.set(s_filename, rfc1867ApcData->filename);
      track.set(s_name, rfc1867ApcData->name);
      track.set(s_done, 0);
      track.set(s_start_time, rfc1867ApcData->start_time);
      HHVM_FN(apc_store)(rfc1867ApcData->tracking_key, track.toVariant(), 3600);
    }
    break;

  case MULTIPART_EVENT_FILE_DATA:
    if (!rfc1867ApcData->tracking_key.empty()) {
      multipart_event_file_data *data =
        (multipart_event_file_data *) event_data;
      rfc1867ApcData->bytes_processed = data->post_bytes_processed;
      if (rfc1867ApcData->bytes_processed -
          rfc1867ApcData->prev_bytes_processed >
          rfc1867ApcData->update_freq) {
        Variant v;
        if (apc_store().get(rfc1867ApcData->tracking_key, v)) {
          if (v.isArray()) {
            ArrayInit track(6, ArrayInit::Map{});
            track.set(s_total, rfc1867ApcData->content_length);
            track.set(s_current, rfc1867ApcData->bytes_processed);
            track.set(s_filename, rfc1867ApcData->filename);
            track.set(s_name, rfc1867ApcData->name);
            track.set(s_done, 0);
            track.set(s_start_time, rfc1867ApcData->start_time);
            HHVM_FN(apc_store)(rfc1867ApcData->tracking_key, track.toVariant(),
                               3600);
          }
          rfc1867ApcData->prev_bytes_processed =
            rfc1867ApcData->bytes_processed;
        }
      }
    }
    break;

  case MULTIPART_EVENT_FILE_END:
    if (!rfc1867ApcData->tracking_key.empty()) {
      multipart_event_file_end *data =
        (multipart_event_file_end *)event_data;
      rfc1867ApcData->bytes_processed = data->post_bytes_processed;
      rfc1867ApcData->cancel_upload = data->cancel_upload;
      rfc1867ApcData->temp_filename = data->temp_filename;
      ArrayInit track(8, ArrayInit::Map{});
      track.set(s_total, rfc1867ApcData->content_length);
      track.set(s_current, rfc1867ApcData->bytes_processed);
      track.set(s_filename, rfc1867ApcData->filename);
      track.set(s_name, rfc1867ApcData->name);
      track.set(s_temp_filename, rfc1867ApcData->temp_filename);
      track.set(s_cancel_upload, rfc1867ApcData->cancel_upload);
      track.set(s_done, 0);
      track.set(s_start_time, rfc1867ApcData->start_time);
      HHVM_FN(apc_store)(rfc1867ApcData->tracking_key, track.toVariant(), 3600);
    }
    break;

  case MULTIPART_EVENT_END:
    if (!rfc1867ApcData->tracking_key.empty()) {
      double now = my_time();
      multipart_event_end *data = (multipart_event_end *)event_data;
      rfc1867ApcData->bytes_processed = data->post_bytes_processed;
      if (now>rfc1867ApcData->start_time) {
        rfc1867ApcData->rate =
          8.0*rfc1867ApcData->bytes_processed/(now-rfc1867ApcData->start_time);
      } else {
        rfc1867ApcData->rate =
          8.0*rfc1867ApcData->bytes_processed;  /* Too quick */
        ArrayInit track(8, ArrayInit::Map{});
        track.set(s_total, rfc1867ApcData->content_length);
        track.set(s_current, rfc1867ApcData->bytes_processed);
        track.set(s_rate, rfc1867ApcData->rate);
        track.set(s_filename, rfc1867ApcData->filename);
        track.set(s_name, rfc1867ApcData->name);
        track.set(s_cancel_upload, rfc1867ApcData->cancel_upload);
        track.set(s_done, 1);
        track.set(s_start_time, rfc1867ApcData->start_time);
        HHVM_FN(apc_store)(rfc1867ApcData->tracking_key, track.toVariant(),
                           3600);
      }
    }
    break;
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// apc serialization

String apc_serialize(const_variant_ref value) {
  VariableSerializer::Type sType =
    apcExtension::EnableApcSerialize ?
      VariableSerializer::Type::APCSerialize :
      VariableSerializer::Type::Internal;
  VariableSerializer vs(sType);
  return vs.serialize(value, true);
}

Variant apc_unserialize(const char* data, int len) {
  VariableUnserializer::Type sType =
    apcExtension::EnableApcSerialize ?
      VariableUnserializer::Type::APCSerialize :
      VariableUnserializer::Type::Internal;
  return unserialize_ex(data, len, sType);
}

String apc_reserialize(const String& str) {
  if (str.empty() ||
      !apcExtension::EnableApcSerialize) return str;

  VariableUnserializer uns(str.data(), str.size(),
                           VariableUnserializer::Type::APCSerialize);
  StringBuffer buf;
  uns.reserialize(buf);

  return buf.detach();
}

///////////////////////////////////////////////////////////////////////////////
// debugging support

bool apc_dump(const char *filename, bool keyOnly, bool metaDump) {
  DumpMode mode;
  std::ofstream out(filename);

  // only one of these should ever be specified
  if (keyOnly && metaDump) {
    return false;
  }

  if (out.fail()) {
    return false;
  }

  if (keyOnly) {
    mode = DumpMode::KeyOnly;
  } else if (metaDump) {
    mode = DumpMode::KeyAndMeta;
  } else {
    mode = DumpMode::KeyAndValue;
  }

  apc_store().dump(out, mode);
  out.close();
  return true;
}

bool apc_dump_prefix(const char *filename,
                     const std::string &prefix,
                     uint32_t count) {
  std::ofstream out(filename);
  if (out.fail()) {
    return false;
  }
  SCOPE_EXIT { out.close(); };

  apc_store().dumpPrefix(out, prefix, count);
  return true;
}

bool apc_get_random_entries(std::ostream &out, uint32_t count) {
  apc_store().dumpRandomKeys(out, count);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
}
