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

#include "hphp/runtime/ext/hh/ext_hh.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <folly/json.h>

#include "hphp/runtime/base/array-init.h"
#include "hphp/runtime/base/autoload-handler.h"
#include "hphp/runtime/base/backtrace.h"
#include "hphp/runtime/base/container-functions.h"
#include "hphp/runtime/base/execution-context.h"
#include "hphp/runtime/base/file-stream-wrapper.h"
#include "hphp/runtime/base/request-tracing.h"
#include "hphp/runtime/base/stream-wrapper-registry.h"
#include "hphp/runtime/base/unit-cache.h"
#include "hphp/runtime/base/variable-serializer.h"
#include "hphp/runtime/ext/fb/ext_fb.h"
#include "hphp/runtime/ext/collections/ext_collections-pair.h"
#include "hphp/runtime/vm/class-meth-data-ref.h"
#include "hphp/runtime/vm/extern-compiler.h"
#include "hphp/runtime/vm/memo-cache.h"
#include "hphp/runtime/vm/runtime.h"
#include "hphp/runtime/vm/vm-regs.h"
#include "hphp/util/file.h"
#include "hphp/util/match.h"

namespace HPHP {

//////////////////////////////////////////////////////////////////////

const StaticString
  s_set_frame_metadata("HH\\set_frame_metadata"),
  // The following are used in serialize_memoize_tv to serialize objects that
  // implement the IMemoizeParam interface
  s_IMemoizeParam("HH\\IMemoizeParam"),
  s_getInstanceKey("getInstanceKey"),
  // The following are used in serialize_memoize_param(); they are what
  // serialize_memoize_tv would have produced on some common inputs; having
  // these lets us avoid creating a StringBuffer
  s_nullMemoKey("\xf0"),
  s_trueMemoKey("\xf1"),
  s_falseMemoKey("\xf2");

///////////////////////////////////////////////////////////////////////////////
bool HHVM_FUNCTION(autoload_set_paths,
                   const Variant& map,
                   const String& root) {
  if (map.isArray()) {
    return AutoloadHandler::s_instance->setMap(map.toCArrRef(), root);
  }
  if (!(map.isObject() && map.toObject()->isCollection())) {
    return false;
  }
  // Assume we have Map<string, Map<string, string>> - convert to
  // array<string, array<string, string>>
  //
  // Exception for 'failure' which should be a callable.
  auto as_array = map.toArray();
  for (auto it = as_array.begin(); !it.end(); it.next()) {
    if (it.second().isObject() && it.second().toObject()->isCollection()) {
      as_array.set(it.first(), it.second().toArray());
    }
  }
  return AutoloadHandler::s_instance->setMap(as_array, root);
}

bool HHVM_FUNCTION(could_include, const String& file) {
  return lookupUnit(file.get(), "", nullptr /* initial_opt */,
                    Native::s_noNativeFuncs, false) != nullptr;
}

namespace {

/**
 * Param serialization for memoization.
 *
 * This code is based on fb_compact_serialize; it was forked because
 * fb_compact_serialize would sometimes ignore insertion order in arrays;
 * e.g. [1 => 1, 2 => 2] would serialize the same as [2 => 2, 1 => 1], which
 * is not acceptable for the memoization use case. The forked code is not
 * compatible with fb_serialize or fb_compact_serialize. The values returned
 * are not intended for deserialization or for shipping across the network,
 * only for use as dict keys; this means it is possible to change it without
 * notice, and it is also why it is safe to do everything in host byte-order.
 *
 * === Format ===
 *
 * Integer values are returned as-is. Strings where the first byte is < 0xf0
 * are returned as-is. All other values are converted to strings of the form
 * <c> <data> where c is a byte (0xf0 | code), and data is a sequence of 0 or
 * more bytes. The codes are:
 *
 * 0 (NULL): KindOfUninit or KindOfNull; data has length 0
 * 1 (TRUE),
 * 2 (FALSE): KindOfBoolean; data has length 0
 * 3 (INT8),
 * 4 (INT16),
 * 5 (INT32),
 * 6 (INT64): KindOfInt64; data has length 1, 2, 4 or 8 respectively, and is
 *            a signed value in host byte order
 * 7 (DOUBLE): KindOfDouble; data has length 8, is in host byte order
 * 8 (STRING): string; data is a int serialized as above, followed by that
 *             many bytes of string data
 * 9 (OBJECT): KindOfObject that extends IMemoizeParam; data is an int
 *             serialized as above, followed by that many bytes of serialized
 *             object data (obtained by calling getInstanceKey on the object)
 * 10 (CONTAINER): any PHP array, collection, or hack array; data is the
 *                 keys and values of the container in insertion order,
 *                 serialized as above, followed by the STOP code
 * 11 (STOP): terminates a CONTAINER encoding
 */

enum SerializeMemoizeCode {
  SER_MC_NULL      = 0,
  SER_MC_TRUE      = 1,
  SER_MC_FALSE     = 2,
  SER_MC_INT8      = 3,
  SER_MC_INT16     = 4,
  SER_MC_INT32     = 5,
  SER_MC_INT64     = 6,
  SER_MC_DOUBLE    = 7,
  SER_MC_STRING    = 8,
  SER_MC_OBJECT    = 9,
  SER_MC_CONTAINER = 10,
  SER_MC_STOP      = 11,
};

const uint64_t kCodeMask DEBUG_ONLY = 0x0f;
const uint64_t kCodePrefix          = 0xf0;

ALWAYS_INLINE void serialize_memoize_code(StringBuffer& sb,
                                          SerializeMemoizeCode code) {
  assertx(code == (code & kCodeMask));
  uint8_t v = (kCodePrefix | code);
  sb.append(reinterpret_cast<char*>(&v), 1);
}

ALWAYS_INLINE void serialize_memoize_int64(StringBuffer& sb, int64_t val) {
  if (val == (int8_t)val) {
    serialize_memoize_code(sb, SER_MC_INT8);
    uint8_t nval = val;
    sb.append(reinterpret_cast<char*>(&nval), 1);
  } else if (val == (int16_t)val) {
    serialize_memoize_code(sb, SER_MC_INT16);
    uint16_t nval = val;
    sb.append(reinterpret_cast<char*>(&nval), 2);
  } else if (val == (int32_t)val) {
    serialize_memoize_code(sb, SER_MC_INT32);
    uint32_t nval = val;
    sb.append(reinterpret_cast<char*>(&nval), 4);
  } else {
    serialize_memoize_code(sb, SER_MC_INT64);
    uint64_t nval = val;
    sb.append(reinterpret_cast<char*>(&nval), 8);
  }
}

ALWAYS_INLINE void serialize_memoize_string_data(StringBuffer& sb,
                                                 const StringData* str) {
  int len = str->size();
  serialize_memoize_int64(sb, len);
  sb.append(str->data(), len);
}

void serialize_memoize_tv(StringBuffer& sb, int depth, TypedValue tv);

void serialize_memoize_tv(StringBuffer& sb, int depth, const TypedValue *tv) {
  serialize_memoize_tv(sb, depth, *tv);
}

ALWAYS_INLINE void serialize_memoize_arraykey(StringBuffer& sb,
                                              const Cell& c) {
  switch (c.m_type) {
    case KindOfPersistentString:
    case KindOfString:
      serialize_memoize_code(sb, SER_MC_STRING);
      serialize_memoize_string_data(sb, c.m_data.pstr);
      break;
    case KindOfInt64:
      serialize_memoize_int64(sb, c.m_data.num);
      break;
    default:
      always_assert(false);
  }
}

void serialize_memoize_array(StringBuffer& sb, int depth, const ArrayData* ad) {
  serialize_memoize_code(sb, SER_MC_CONTAINER);
  IterateKV(ad, [&] (Cell k, TypedValue v) {
    serialize_memoize_arraykey(sb, k);
    serialize_memoize_tv(sb, depth, v);
    return false;
  });
  serialize_memoize_code(sb, SER_MC_STOP);
}

ALWAYS_INLINE
void serialize_memoize_col(StringBuffer& sb, int depth, ObjectData* obj) {
  assertx(obj->isCollection());
  auto const ad = collections::asArray(obj);
  if (LIKELY(ad != nullptr)) {
    serialize_memoize_array(sb, depth, ad);
  } else {
    assertx(obj->collectionType() == CollectionType::Pair);
    auto const pair = reinterpret_cast<const c_Pair*>(obj);
    serialize_memoize_code(sb, SER_MC_CONTAINER);
    serialize_memoize_int64(sb, 0);
    serialize_memoize_tv(sb, depth, pair->get(0));
    serialize_memoize_int64(sb, 1);
    serialize_memoize_tv(sb, depth, pair->get(1));
    serialize_memoize_code(sb, SER_MC_STOP);
  }
}

void serialize_memoize_obj(StringBuffer& sb, int depth, ObjectData* obj) {
  if (obj->isCollection()) {
    serialize_memoize_col(sb, depth, obj);
  } else if (obj->instanceof(s_IMemoizeParam)) {
    Variant ser = obj->o_invoke_few_args(s_getInstanceKey, 0);
    serialize_memoize_code(sb, SER_MC_OBJECT);
    serialize_memoize_string_data(sb, ser.toString().get());
  } else {
    auto msg = folly::format(
      "Cannot serialize object of type {} because it does not implement "
        "HH\\IMemoizeParam",
      obj->getClassName().asString()
    ).str();
    SystemLib::throwInvalidArgumentExceptionObject(msg);
  }
}

void serialize_memoize_tv(StringBuffer& sb, int depth, TypedValue tv) {
  if (depth > 256) {
    SystemLib::throwInvalidArgumentExceptionObject("Array depth exceeded");
  }
  depth++;

  switch (tv.m_type) {
    case KindOfUninit:
    case KindOfNull:
      serialize_memoize_code(sb, SER_MC_NULL);
      break;

    case KindOfBoolean:
      serialize_memoize_code(sb, tv.m_data.num ? SER_MC_TRUE : SER_MC_FALSE);
      break;

    case KindOfInt64:
      serialize_memoize_int64(sb, tv.m_data.num);
      break;

    case KindOfDouble:
      serialize_memoize_code(sb, SER_MC_DOUBLE);
      sb.append(reinterpret_cast<const char*>(&tv.m_data.dbl), 8);
      break;

    case KindOfFunc:
      serialize_memoize_code(sb, SER_MC_STRING);
      serialize_memoize_string_data(sb, funcToStringHelper(tv.m_data.pfunc));
      break;

    case KindOfClass:
      serialize_memoize_code(sb, SER_MC_STRING);
      serialize_memoize_string_data(sb, classToStringHelper(tv.m_data.pclass));
      break;

    case KindOfPersistentString:
    case KindOfString:
      serialize_memoize_code(sb, SER_MC_STRING);
      serialize_memoize_string_data(sb, tv.m_data.pstr);
      break;

    case KindOfPersistentVec:
    case KindOfVec:
    case KindOfPersistentDict:
    case KindOfDict:
    case KindOfPersistentKeyset:
    case KindOfKeyset:
    case KindOfPersistentShape:
    case KindOfShape:
    case KindOfPersistentArray:
    case KindOfArray:
      serialize_memoize_array(sb, depth, tv.m_data.parr);
      break;

    case KindOfClsMeth:
      raiseClsMethToVecWarningHelper();
      serialize_memoize_array(
        sb, depth, clsMethToVecHelper(tv.m_data.pclsmeth).get());
      break;
    case KindOfObject:
      serialize_memoize_obj(sb, depth, tv.m_data.pobj);
      break;

    case KindOfResource:
    case KindOfRecord: // TODO(T41025646)
    case KindOfRef: {
      auto msg = folly::format(
        "Cannot Serialize unexpected type {}",
        tname(tv.m_type)
      ).str();
      SystemLib::throwInvalidArgumentExceptionObject(msg);
      break;
    }
  }
}

ALWAYS_INLINE TypedValue serialize_memoize_string_top(StringData* str) {
  if (str->empty()) {
    return make_tv<KindOfPersistentString>(staticEmptyString());
  } else if ((unsigned char)str->data()[0] < 0xf0) {
    // serialize_memoize_string_data always returns a string with the first
    // character >= 0xf0, so anything less than that can't collide. There's no
    // worry about int-like strings because we won't perform key coercion.
    str->incRefCount();
    return make_tv<KindOfString>(str);
  }

  StringBuffer sb;
  serialize_memoize_code(sb, SER_MC_STRING);
  serialize_memoize_string_data(sb, str);
  return make_tv<KindOfString>(sb.detach().detach());
}

} // end anonymous namespace

TypedValue serialize_memoize_param_arr(ArrayData* arr) {
  StringBuffer sb;
  serialize_memoize_array(sb, 0, arr);
  return tvReturn(sb.detach());
}

TypedValue serialize_memoize_param_obj(ObjectData* obj) {
  StringBuffer sb;
  serialize_memoize_obj(sb, 0, obj);
  return tvReturn(sb.detach());
}

TypedValue serialize_memoize_param_col(ObjectData* obj) {
  StringBuffer sb;
  serialize_memoize_col(sb, 0, obj);
  return tvReturn(sb.detach());
}

TypedValue serialize_memoize_param_str(StringData* str) {
  return serialize_memoize_string_top(str);
}

TypedValue serialize_memoize_param_dbl(double val) {
  StringBuffer sb;
  serialize_memoize_code(sb, SER_MC_DOUBLE);
  sb.append(reinterpret_cast<const char*>(&val), 8);
  return tvReturn(sb.detach());
}

TypedValue HHVM_FUNCTION(serialize_memoize_param, TypedValue param) {
  // Memoize throws in the emitter if any function parameters are references, so
  // we can just assert that the param is cell here
  assertx(cellIsPlausible(param));
  auto const type = param.m_type;

  if (type == KindOfInt64) {
    return param;
  } else if (isStringType(type)) {
    return serialize_memoize_string_top(param.m_data.pstr);
  } else if (type == KindOfUninit || type == KindOfNull) {
    return make_tv<KindOfPersistentString>(s_nullMemoKey.get());
  } else if (type == KindOfBoolean) {
    return make_tv<KindOfPersistentString>(
      param.m_data.num ? s_trueMemoKey.get() : s_falseMemoKey.get()
    );
  }

  StringBuffer sb;
  serialize_memoize_tv(sb, 0, &param);
  return tvReturn(sb.detach());
}

namespace {

void clearValueLink(rds::Link<Cell, rds::Mode::Normal> valLink) {
  if (valLink.bound() && valLink.isInit()) {
    auto oldVal = *valLink;
    valLink.markUninit();
    tvDecRefGen(oldVal);
  }
}

void clearCacheLink(rds::Link<MemoCacheBase*, rds::Mode::Normal> cacheLink) {
    if (cacheLink.bound() && cacheLink.isInit()) {
      auto oldCache = *cacheLink;
      cacheLink.markUninit();
      if (oldCache) req::destroy_raw(oldCache);
    }
}

} // end anonymous namespace

bool HHVM_FUNCTION(clear_static_memoization,
                   TypedValue clsStr, TypedValue funcStr) {
  auto clear = [] (const Func* func) {
    if (!func->isMemoizeWrapper()) return false;
    clearValueLink(rds::attachStaticMemoValue(func));
    clearCacheLink(rds::attachStaticMemoCache(func));
    return true;
  };

  if (isStringType(clsStr.m_type)) {
    auto const cls = Unit::loadClass(clsStr.m_data.pstr);
    if (!cls) return false;
    if (isStringType(funcStr.m_type)) {
      auto const func = cls->lookupMethod(funcStr.m_data.pstr);
      return func && func->isStatic() && clear(func);
    }
    auto ret = false;
    for (auto i = cls->numMethods(); i--; ) {
      auto const func = cls->getMethod(i);
      if (func->isStatic()) {
        if (clear(func)) ret = true;
      }
    }
    return ret;
  }

  if (isStringType(funcStr.m_type)) {
    auto const func = Unit::loadFunc(funcStr.m_data.pstr);
    return func && clear(func);
  }

  return false;
}

String HHVM_FUNCTION(ffp_parse_string_native, const String& str) {
  std::string program = str.get()->data();

  auto result = ffp_parse_file("", program.c_str(), program.size());

  FfpJSONString res;
  match<void>(
    result,
    [&](FfpJSONString& r) {
      res = std::move(r);
    },
    [&](std::string& err) {
      SystemLib::throwInvalidArgumentExceptionObject(
        "FFP failed to parse string");
    }
  );
  return res.value;
}

bool HHVM_FUNCTION(clear_lsb_memoization,
                   const String& clsStr, TypedValue funcStr) {
  auto const clear = [](const Class* cls, const Func* func) {
    if (!func->isStatic()) return false;
    if (!func->isMemoizeWrapperLSB()) return false;
    clearValueLink(rds::attachLSBMemoValue(cls, func));
    clearCacheLink(rds::attachLSBMemoCache(cls, func));
    return true;
  };

  auto const cls = Unit::loadClass(clsStr.get());
  if (!cls) return false;

  if (isStringType(funcStr.m_type)) {
    auto const func = cls->lookupMethod(funcStr.m_data.pstr);
    return func && clear(cls, func);
  }

  auto ret = false;
  for (auto i = cls->numMethods(); i--; ) {
    auto const func = cls->getMethod(i);
    if (clear(cls, func)) ret = true;
  }
  return ret;
}

bool HHVM_FUNCTION(clear_instance_memoization, const Object& obj) {
  auto const cls = obj->getVMClass();
  if (!cls->hasMemoSlots()) return false;

  if (!obj->getAttribute(ObjectData::UsedMemoCache)) return true;

  auto const nSlots = cls->numMemoSlots();
  for (Slot i = 0; i < nSlots; ++i) {
    auto slot = UNLIKELY(obj->hasNativeData())
      ? obj->memoSlotNativeData(i, cls->getNativeDataInfo()->sz)
      : obj->memoSlot(i);
    if (slot->isCache()) {
      if (auto cache = slot->getCache()) {
        slot->resetCache();
        req::destroy_raw(cache);
      }
    } else {
      auto const oldVal = *slot->getValue();
      tvWriteUninit(*slot->getValue());
      tvDecRefGen(oldVal);
    }
  }

  return true;
}

void HHVM_FUNCTION(set_frame_metadata, const Variant&) {
  SystemLib::throwInvalidArgumentExceptionObject(
    "Unsupported dynamic call of set_frame_metadata()");
}

namespace {

ArrayData* from_stats(rqtrace::EventStats stats) {
  return make_dict_array(
    "duration", stats.total_duration, "count", stats.total_count).detach();
}

template<class T>
ArrayData* from_stats_list(T stats) {
  DictInit init(stats.size());
  for (auto& pair : stats) {
    init.set(String(pair.first), Array::attach(from_stats(pair.second)));
  }
  return init.create();
}

bool HHVM_FUNCTION(is_enabled) {
  return g_context->getRequestTrace() != nullptr;
}

void HHVM_FUNCTION(force_enable) {
  if (g_context->getRequestTrace()) return;
  if (auto const transport = g_context->getTransport()) {
    transport->forceInitRequestTrace();
    g_context->setRequestTrace(transport->getRequestTrace());
  }
}

TypedValue HHVM_FUNCTION(all_request_stats) {
  if (auto const trace = g_context->getRequestTrace()) {
    return tvReturn(from_stats_list(trace->stats()));
  }
  return tvReturn(staticEmptyDArray());
}

TypedValue HHVM_FUNCTION(all_process_stats) {
  req::vector<std::pair<StringData*, rqtrace::EventStats>> stats;

  rqtrace::visit_process_stats(
    [&] (const StringData* name, rqtrace::EventStats s) {
      stats.emplace_back(const_cast<StringData*>(name), s);
    }
  );

  return tvReturn(from_stats_list(stats));
}

TypedValue HHVM_FUNCTION(request_event_stats, StringArg event) {
  if (auto const trace = g_context->getRequestTrace()) {
    auto const stats = folly::get_default(trace->stats(), event->data());
    return tvReturn(from_stats(stats));
  }
  return tvReturn(from_stats({}));
}

TypedValue HHVM_FUNCTION(process_event_stats, StringArg event) {
  return tvReturn(from_stats(rqtrace::process_stats_for(event->data())));
}

int64_t HHVM_FUNCTION(get_request_count) {
  return requestCount();
}

Array HHVM_FUNCTION(get_compiled_units, int64_t kind) {
  auto const& units = g_context.getNoCheck()->m_evaledFiles;
  KeysetInit init{units.size()};
  for (auto& u : units) {
    switch (u.second.flags) {
    case FileLoadFlags::kDup:     break;
    case FileLoadFlags::kHitMem:  break;
    case FileLoadFlags::kWaited:  if (kind < 2) break;
    case FileLoadFlags::kHitDisk: if (kind < 1) break;
    case FileLoadFlags::kCompiled:if (kind < 0) break;
    case FileLoadFlags::kEvicted:
      init.add(const_cast<StringData*>(u.first));
    }
  }
  return init.toArray();
}

TypedValue HHVM_FUNCTION(dynamic_fun, StringArg fun) {
  auto const func = Unit::loadFunc(fun.get());
  if (!func) {
    SystemLib::throwInvalidArgumentExceptionObject(
      folly::sformat("Unable to find function {}", fun.get()->data())
    );
  }
  if (!func->isDynamicallyCallable()) {
    auto const level = RuntimeOption::EvalDynamicFunLevel;
    if (level == 2) {
      SystemLib::throwInvalidArgumentExceptionObject(
        folly::sformat("Function {} not marked dynamic", fun.get()->data())
      );
    }
    if (level == 1) {
      raise_warning("Function %s not marked dynamic", fun.get()->data());
    }
  }
  return tvReturn(func);
}

TypedValue HHVM_FUNCTION(dynamic_class_meth, StringArg cls, StringArg meth) {
  auto const c = Unit::loadClass(cls.get());
  if (!c) {
    SystemLib::throwInvalidArgumentExceptionObject(
      folly::sformat("Unable to find class {}", cls.get()->data())
    );
  }
  auto const func = c->lookupMethod(meth.get());
  if (!func) {
    SystemLib::throwInvalidArgumentExceptionObject(
      folly::sformat("Unable to method {}::{}",
                     cls.get()->data(), meth.get()->data())
    );
  }
  if (!func->isStatic()) {
    SystemLib::throwInvalidArgumentExceptionObject(
      folly::sformat("Method {}::{} is not static",
                     cls.get()->data(), meth.get()->data())
    );
  }
  if (!func->isPublic()) {
    auto const ctx = fromCaller(
      [] (const ActRec* fp, Offset) { return fp->func()->cls(); }
    );
    if (func->attrs() & AttrPrivate) {
      if (func->cls() != ctx) {
        SystemLib::throwInvalidArgumentExceptionObject(
          folly::sformat("Method {}::{} is marked Private",
                         cls.get()->data(), meth.get()->data())
        );
      }
    } else if (func->attrs() & AttrProtected) {
      if (!ctx || !ctx->classof(func->cls())) {
        SystemLib::throwInvalidArgumentExceptionObject(
          folly::sformat("Method {}::{} is marked Protected",
                         cls.get()->data(), meth.get()->data())
        );
      }
    }
  }
  if (!func->isDynamicallyCallable()) {
    auto const level = RuntimeOption::EvalDynamicClsMethLevel;
    if (level == 2) {
      SystemLib::throwInvalidArgumentExceptionObject(
        folly::sformat("Method {}::{} not marked dynamic",
                       cls.get()->data(), meth.get()->data())
      );
    }
    if (level == 1) {
      raise_warning("Method %s::%s not marked dynamic",
                    cls.get()->data(), meth.get()->data());
    }
  }
  return tvReturn(ClsMethDataRef::create(c, func));
}

}

static struct HHExtension final : Extension {
  HHExtension(): Extension("hh", NO_EXTENSION_VERSION_YET) { }
  void moduleInit() override {
    HHVM_NAMED_FE(HH\\autoload_set_paths, HHVM_FN(autoload_set_paths));
    HHVM_NAMED_FE(HH\\could_include, HHVM_FN(could_include));
    HHVM_NAMED_FE(HH\\serialize_memoize_param,
                  HHVM_FN(serialize_memoize_param));
    HHVM_NAMED_FE(HH\\clear_static_memoization,
                  HHVM_FN(clear_static_memoization));
    HHVM_NAMED_FE(HH\\ffp_parse_string_native,
                  HHVM_FN(ffp_parse_string_native));
    HHVM_NAMED_FE(HH\\clear_lsb_memoization,
                  HHVM_FN(clear_lsb_memoization));
    HHVM_NAMED_FE(HH\\clear_instance_memoization,
                  HHVM_FN(clear_instance_memoization));
    HHVM_NAMED_FE(HH\\set_frame_metadata, HHVM_FN(set_frame_metadata));
    HHVM_NAMED_FE(HH\\get_request_count, HHVM_FN(get_request_count));
    HHVM_NAMED_FE(HH\\get_compiled_units, HHVM_FN(get_compiled_units));

    HHVM_NAMED_FE(HH\\rqtrace\\is_enabled, HHVM_FN(is_enabled));
    HHVM_NAMED_FE(HH\\rqtrace\\force_enable, HHVM_FN(force_enable));
    HHVM_NAMED_FE(HH\\rqtrace\\all_request_stats, HHVM_FN(all_request_stats));
    HHVM_NAMED_FE(HH\\rqtrace\\all_process_stats, HHVM_FN(all_process_stats));
    HHVM_NAMED_FE(HH\\rqtrace\\request_event_stats,
                  HHVM_FN(request_event_stats));
    HHVM_NAMED_FE(HH\\rqtrace\\process_event_stats,
                  HHVM_FN(process_event_stats));
    HHVM_NAMED_FE(HH\\dynamic_fun, HHVM_FN(dynamic_fun));
    HHVM_NAMED_FE(HH\\dynamic_class_meth, HHVM_FN(dynamic_class_meth));
    loadSystemlib();
  }
} s_hh_extension;

static struct XHPExtension final : Extension {
  XHPExtension(): Extension("xhp", NO_EXTENSION_VERSION_YET) { }
  bool moduleEnabled() const override { return RuntimeOption::EnableXHP; }
} s_xhp_extension;

///////////////////////////////////////////////////////////////////////////////
}
