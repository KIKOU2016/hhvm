/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
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
#include "hphp/hhbbc/dce.h"

#include <vector>
#include <string>
#include <utility>
#include <bitset>
#include <sstream>
#include <algorithm>
#include <set>

#include <boost/dynamic_bitset.hpp>
#include <boost/container/flat_map.hpp>

#include <folly/gen/Base.h>
#include <folly/gen/String.h>

#include "hphp/runtime/base/array-iterator.h"

#include "hphp/util/bitset-utils.h"
#include "hphp/util/dataflow-worklist.h"
#include "hphp/util/trace.h"

#include "hphp/hhbbc/analyze.h"
#include "hphp/hhbbc/cfg-opts.h"
#include "hphp/hhbbc/cfg.h"
#include "hphp/hhbbc/func-util.h"
#include "hphp/hhbbc/interp-state.h"
#include "hphp/hhbbc/interp.h"
#include "hphp/hhbbc/optimize.h"
#include "hphp/hhbbc/options.h"
#include "hphp/hhbbc/representation.h"
#include "hphp/hhbbc/type-system.h"
#include "hphp/hhbbc/unit-util.h"

#include "hphp/runtime/base/mixed-array.h"

namespace HPHP { namespace HHBBC {

TRACE_SET_MOD(hhbbc_dce);

//////////////////////////////////////////////////////////////////////

/*
 * This module contains code to perform both local DCE and global DCE.
 *
 * The local DCE algorithm addresses dead eval stack manipulations and
 * dead stores to locals that are visible within a single basic block.
 *
 * The global DCE performs a liveness analysis and then uses this
 * information to allow dead stores to locals to be eliminated across
 * blocks.
 *
 * Both types of DCE here need to be type-aware, but they must visit
 * blocks backward.  They accomplish this by forward-propagating the
 * block input states from a FuncAnalysis to each instruction in the
 * block prior to doing the backward iteration.
 *
 * Eval stack:
 *
 *   During backward traversal of a block, we maintain a "backwards"
 *   stack, indicating which eval stack slots are going to be required
 *   in the future or not.
 *
 *   During this traversal, each instruction that pops when going
 *   forward instead "pushes" information about whether that input
 *   will be required.  If it is not required, it also pushes an
 *   accumulating set of instruction ids that must be removed if the
 *   instruction which produces the stack slot is removed.  (All
 *   instructions in these sets must be removed if any are, in order
 *   to keep the stack depths correct.)
 *
 *   Similarly, each instruction that would push a value going forward
 *   instead "pops" the information about whether its stack output is
 *   going to be needed.  If not, the instruction can mark itself (and
 *   all downstream instructions that depended on it) as removable.
 *
 * Locals:
 *
 *   While a block is iterated backward, the set of live locals is
 *   tracked.  The initial state of this live set depends on whether
 *   we are performing global or local DCE, and in the local case
 *   includes all locals in the function.
 *
 *   When a local may be read, it is added to the live set.  When a
 *   local is definitely-written, it is removed from the set.
 *
 *   If a instruction may write to a local that is not live, it can be
 *   marked as removable if it is known to have no other side-effects.
 *   Currently this is only hooked up to SetL.
 *
 * Liveness analysis:
 *
 *   The global algorithm first performs a liveness analysis to
 *   propagate live out sets to each block.
 *
 *   This analysis is basically normal, but slightly modified from the
 *   usual in order to deal with the way exceptional control flow is
 *   represented in our CFG.
 *
 *   It essentially is the liveness analysis algorithm described at
 *   http://dl.acm.org/citation.cfm?id=316171, except that we don't
 *   need to track kill sets for each PEI because we don't have a
 *   means of determining which exceptional edges may be traversed by any
 *   given PEI.  (Maybe that may be more usual to have in the context
 *   of a language with declared exception clauses...)
 *
 *   Since we only deal with the most pessimistic exception case, this
 *   means for each block we just determine a gen and kill set, along
 *   with a subset of the latter set that is the locals that must be
 *   killed before any PEI.  (See killBeforePEI below.)
 *
 * Final note about types:
 *
 *   Global DCE can change the types of locals in a way that spans
 *   basic blocks.  For a simple example, take the following code:
 *
 *      // $foo :: Uninit here.
 *      $foo = 12;
 *      // ... code that breaks a block ...
 *      $foo = 100;
 *
 *   If the first store to $foo is removed, the type of $foo before
 *   the SetL in the later block is now Uninit, instead of Int.
 *
 *   In addition, by killing dead eval stack slots across basic
 *   blocks, the saved entry stacks in the FuncAnalysis no longer
 *   match the actual stack depths.
 *
 *   This means that after calling global_dce on a function, its
 *   FuncAnalysis can no longer be considered accurate.
 *
 *   Moreover, since global DCE makes use of type information to
 *   determine whether a store is dead, we need to be careful that
 *   this never changes whether the assumptions used to perform DCE
 *   were correct.
 *
 *   This is ok right now: DCE only uses the types to figure out which
 *   values can either have lifetimes extended or shortened without
 *   visible side-effects, or which values may be refs (so we can't
 *   omit stores to them).  If we omit a store that changes the type
 *   of a local globally, this means the new type cannot be different
 *   with regard to these features (or we wouldn't have omitted it),
 *   which means we won't have made different decisions elsewhere in
 *   the algorithm based on the new type.
 *
 *   Specifically: we will never omit a store where the old local type
 *   was something that could've had side effects, and if a new value
 *   is stored into a local where destroying it could have
 *   side-effects, some point along the path to the function exit (if
 *   nothing else, the RetC) will have added it to the gen set and the
 *   store also won't be removable.
 *
 *   In contrast, note that local DCE can not change types across
 *   block boundaries.
 *
 */

namespace {

//////////////////////////////////////////////////////////////////////

// The number of pops as seen by dce.
uint32_t numPop(const Bytecode& bc) {
  if (bc.op == Op::CGetL2) return 1;
  return bc.numPop();
}

// The number of pushes as seen by dce.
uint32_t numPush(const Bytecode& bc) {
  if (bc.op == Op::CGetL2) return 2;
  return bc.numPush();
}

// Returns whether a set on something containing type t could have
// side-effects (running destuctors, or modifying arbitrary things via
// a Ref).
bool setCouldHaveSideEffects(const Type& t) {
  return t.couldBe(BRef);
}

// Some reads could raise warnings and run arbitrary code.
bool readCouldHaveSideEffects(const Type& t) {
  return t.couldBe(BUninit);
}

//////////////////////////////////////////////////////////////////////

/*
 * Use information of a stack cell.
 */
enum class Use {
  // Indicates that the cell is (possibly) used.
  Used = 0,

  // Indicates that the cell is (unconditionally) not used.
  Not = 1,

  /*
   * Indicates that the stack slot contains an array-like being
   * constructed by AddElemCs, which looks like it can be optimized to
   * a NewStructArray, NewPackedArray, or NewVecArray.
   */
  AddElemC = 3,

  /*
   * Modifier or-ed into the above to indicate that this use is linked
   * to the stack slot below it (in stack order - the bottom of the
   * stack is below the top); either they are both optimized, or
   * neither is.
   */
  Linked = 4,
};

Use operator|(Use a, Use b) {
  return static_cast<Use>(static_cast<int>(a) | static_cast<int>(b));
}

Use operator&(Use a, Use b) {
  return static_cast<Use>(static_cast<int>(a) & static_cast<int>(b));
}

bool any(Use a) {
  return static_cast<int>(a) != 0;
}

Use mask_use(Use a) { return a & static_cast<Use>(3); }

struct InstrId {
  BlockId blk;
  uint32_t idx;
};

bool operator<(const InstrId& i1, const InstrId& i2) {
  if (i1.blk != i2.blk) return i1.blk < i2.blk;
  // sort by decreasing idx so that kill_marked_instrs works
  return i1.idx > i2.idx;
}

bool operator==(const InstrId& i1, const InstrId& i2) {
  return i1.blk == i2.blk && i1.idx == i2.idx;
}

struct LocationId {
  BlockId  blk;
  uint32_t id;
  bool     isSlot;
};

bool operator<(const LocationId& i1, const LocationId& i2) {
  if (i1.blk != i2.blk) return i1.blk < i2.blk;
  if (i1.id != i2.id)   return i1.id < i2.id;
  return i2.isSlot && !i1.isSlot;
}

struct LocationIdHash {
  size_t operator()(const LocationId& l) const {
    return folly::hash::hash_combine(hash_int64_pair(l.blk, l.id), l.isSlot);
  }
};

bool operator==(const LocationId& i1, const LocationId& i2) {
  return i1.blk == i2.blk &&
         i1.id == i2.id &&
         i1.isSlot == i2.isSlot;
}

using LocationIdSet = hphp_fast_set<LocationId,LocationIdHash>;

struct DceAction {
  enum Action {
    Kill,
    PopInputs,
    PopOutputs,
    Replace,
    PopAndReplace,
    MinstrStackFinal,
    MinstrStackFixup
  } action;
  using MaskType = uint32_t;
  static constexpr size_t kMaskSize = 32;
  MaskType mask{0};


  DceAction() = default;
  /* implicit */ DceAction(Action a) : action{a} {}
  DceAction(Action a, MaskType m) : action{a}, mask{m} {}
};

bool operator==(const DceAction& a, const DceAction& b) {
  return a.action == b.action && a.mask == b.mask;
}

using DceActionMap = std::map<InstrId, DceAction>;
using DceReplaceMap = std::map<InstrId, CompactVector<Bytecode>>;

struct UseInfo {
  explicit UseInfo(Use u) : usage(u) {}
  UseInfo(Use u, DceActionMap&& ids) : usage(u), actions(std::move(ids)) {}

  Use usage;
  /*
   * Set of actions that should be performed if we decide to
   * discard the corresponding stack slot.
   */
  DceActionMap actions;
  /*
   * Used for stack slots that are live across blocks to indicate a
   * stack slot that should be considered Used at the entry to blk.
   *
   * If its not live across blocks, location.blk will stay NoBlockId.
   */
  LocationId location { NoBlockId, 0, false };
};

//////////////////////////////////////////////////////////////////////

struct DceState {
  explicit DceState(const Index& index, const FuncAnalysis& ainfo) :
      index(index), ainfo(ainfo) {}
  const Index& index;
  const FuncAnalysis& ainfo;
  /*
   * Used to accumulate a set of blk/stack-slot pairs that
   * should be marked used.
   */
  LocationIdSet forcedLiveLocations;

  /*
   * Eval stack use information.  Stacks slots are marked as being
   * needed or not needed.  If they aren't needed, they carry a set of
   * instructions that must be removed if the instruction that
   * produces the stack value is also removable.
   */
  std::vector<UseInfo> stack;

  /*
   * Locals known to be live at a point in a DCE walk.  This is used
   * when we're actually acting on information we discovered during
   * liveness analysis.
   */
  std::bitset<kMaxTrackedLocals> liveLocals;

  /*
   * Instructions marked in this set will be processed by dce_perform.
   * They must all be processed together to keep eval stack consumers
   * and producers balanced.
   */
  DceActionMap actionMap;

  /*
   * Actions of type Replace in the actionMap have an entry here with
   * the replacement bytecodes.
   */
  DceReplaceMap replaceMap;

  /*
   * The set of locals and that were ever live in this block.  (This
   * includes ones that were live going out of this block.)  This set
   * is used by global DCE to remove locals that are completely unused
   * in the entire function.
   */
  std::bitset<kMaxTrackedLocals> usedLocals;

  /*
   * Can be set by a minstr-final instruction to indicate the actions
   * to take provided all previous minstrs in the group are non-pei.
   *
   * eg if the result of a QueryM is unused, and the whole sequence is
   * non-pei we can kill it. Or if the result is a literal, and the
   * whole sequence is non-pei, we can replace it with pops plus the
   * constant.
   */
  folly::Optional<UseInfo> minstrUI;
  /*
   * Flag to indicate local vs global dce.
   */
  bool isLocal{false};

  /*
   * When we do add elem opts, it can enable further optimization
   * opportunities. Let the optimizer know...
   */
  bool didAddElemOpts{false};
};

//////////////////////////////////////////////////////////////////////
// debugging

const char* show(DceAction action) {
  switch (action.action) {
    case DceAction::Kill:             return "Kill";
    case DceAction::PopInputs:        return "PopInputs";
    case DceAction::PopOutputs:       return "PopOutputs";
    case DceAction::Replace:          return "Replace";
    case DceAction::PopAndReplace:    return "PopAndReplace";
    case DceAction::MinstrStackFinal: return "MinstrStackFinal";
    case DceAction::MinstrStackFixup: return "MinstrStackFixup";
  };
  not_reached();
}

std::string show(InstrId id) {
  return folly::sformat("{}:{}", id.blk, id.idx);
}

inline void validate(Use u) {
  assert(!any(u & Use::Linked) || mask_use(u) == Use::Not);
}

const char* show(Use u) {
  validate(u);
  auto ret = [&] {
    switch (mask_use(u)) {
      case Use::Used:          return "*U";
      case Use::Not:           return "*0";
      case Use::AddElemC:      return "*AE";
      case Use::Linked: not_reached();
    }
    not_reached();
  }();

  return !any(u & Use::Linked) ? ret + 1 : ret;
}

std::string show(const LocationId& id) {
  return folly::sformat("{}:{}{}", id.blk, id.id, id.isSlot ? "(slot)" : "");
}

std::string show(const DceActionMap::value_type& elm) {
  return folly::sformat("{}={}", show(elm.first), show(elm.second));
}

std::string show(const DceActionMap& actions) {
  using namespace folly::gen;
  return from(actions)
    | map([](const DceActionMap::value_type& elm) { return show(elm); })
    | unsplit<std::string>(";")
    ;
}

std::string DEBUG_ONLY show(const UseInfo& ui) {
  return folly::sformat("{}({})", show(ui.usage), show(ui.actions));
}

std::string DEBUG_ONLY loc_bits_string(const php::Func* func,
                                       std::bitset<kMaxTrackedLocals> locs) {
  std::ostringstream out;
  if (func->locals.size() < kMaxTrackedLocals) {
    for (auto i = func->locals.size(); i-- > 0;) {
      out << (locs.test(i) ? '1' : '0');
    }
  } else {
    out << locs;
  }
  return out.str();
}

//////////////////////////////////////////////////////////////////////

struct Env {
  DceState& dceState;
  const Bytecode& op;
  InstrId id;
  LocalId loc;
  const State& stateBefore;
  const StepFlags& flags;
  const State& stateAfter;
};

//////////////////////////////////////////////////////////////////////
// Properties of UseInfo

bool isLinked(const UseInfo& ui) {
  return any(ui.usage & Use::Linked);
}

bool lastUiIsLinked(const UseInfo& ui) {
  return isLinked(ui);
}

template <typename... Args>
bool lastUiIsLinked(const UseInfo& /*ui*/, const Args&... args) {
  return lastUiIsLinked(args...);
}

bool allUnused() { return true; }
template<class... Args>
bool allUnused(const UseInfo& ui, const Args&... args) {
  return (mask_use(ui.usage)) == Use::Not &&
    allUnused(args...);
}

bool allUnusedIfNotLastRef() { return true; }
template<class... Args>
bool allUnusedIfNotLastRef(const UseInfo& ui, const Args&... args) {
  auto u = mask_use(ui.usage);
  return u == Use::Not && allUnusedIfNotLastRef(args...);
}

bool alwaysPop(const UseInfo& ui) {
  return
    ui.location.blk != NoBlockId ||
    ui.actions.size() > 1;
}

template<typename... Args>
bool alwaysPop(const UseInfo& ui, const Args&... args) {
  return alwaysPop(ui) || alwaysPop(args...);
}

bool maybePop(Env& env, const UseInfo& ui) {
  return
    (ui.actions.size() == 1 &&
     ui.actions.begin()->first.idx > env.id.idx + 1);
}

template<typename... Args>
bool maybePop(Env& env, const UseInfo& ui, const Args&... args) {
  return maybePop(env, ui) || maybePop(env, args...);
}

/*
 * Determine whether its worth inserting PopCs after an instruction
 * that can't be dced in order to execute the dependent actions.
 */
template<typename... Args>
bool shouldPopOutputs(Env& env, const Args&... args) {
  if (alwaysPop(args...)) return true;
  return maybePop(env, args...);
}

//////////////////////////////////////////////////////////////////////
// query eval stack

Type topT(Env& env, uint32_t idx = 0) {
  assert(idx < env.stateBefore.stack.size());
  return env.stateBefore.stack[env.stateBefore.stack.size() - idx - 1].type;
}

Type topC(Env& env, uint32_t idx = 0) {
  auto const t = topT(env, idx);
  assert(t.subtypeOf(BInitCell));
  return t;
}

//////////////////////////////////////////////////////////////////////
// locals

void addLocGenSet(Env& env, std::bitset<kMaxTrackedLocals> locs) {
  FTRACE(4, "      loc-conservative: {}\n",
         loc_bits_string(env.dceState.ainfo.ctx.func, locs));
  env.dceState.liveLocals |= locs;
}

void addLocGen(Env& env, uint32_t id) {
  FTRACE(2, "      loc-gen: {}\n", id);
  if (id >= kMaxTrackedLocals) return;
  env.dceState.liveLocals[id] = 1;
}

/*
 * Indicate that this instruction will use the value of loc unless we
 * kill it. handle_push will take care of calling addLocGen if
 * appropriate.
 */
void scheduleGenLoc(Env& env, LocalId loc) {
  env.loc = loc;
}

void addLocKill(Env& env, uint32_t id) {
  FTRACE(2, "     loc-kill: {}\n", id);
  if (id >= kMaxTrackedLocals) return;
  env.dceState.liveLocals[id] = 0;
}

bool isLocLive(Env& env, uint32_t id) {
  if (id >= kMaxTrackedLocals) {
    // Conservatively assume it's potentially live.
    return true;
  }
  return env.dceState.liveLocals[id];
}

Type locRaw(Env& env, LocalId loc) {
  return env.stateBefore.locals[loc];
}

bool setLocCouldHaveSideEffects(Env& env, LocalId loc, bool forExit = false) {
  // A "this" local is protected by the $this in the ActRec.
  if (loc == env.stateBefore.thisLoc) return false;

  // Normally, if there's an equivLocal this isn't the last reference,
  // so overwriting it won't run any destructors. But if we're
  // destroying all the locals (eg RetC) they can't all protect each
  // other; in that case we require a lower equivLoc to ensure that
  // one local from each equivalence set will still be marked as
  // having effects (choosing lower numbers also means we mark the
  // params as live, which makes it more likely that we can eliminate
  // a local).
  static_assert(NoLocalId == std::numeric_limits<LocalId>::max(),
                "NoLocalId must be greater than all valid local ids");
  if (env.stateBefore.equivLocals.size() > loc) {
    auto l = env.stateBefore.equivLocals[loc];
    if (l != NoLocalId) {
      if (!forExit) return false;
      do {
        if (l < loc) return false;
        l = env.stateBefore.equivLocals[l];
      } while (l != loc);
    }
  }

  return setCouldHaveSideEffects(locRaw(env, loc));
}

void readDtorLocs(Env& env) {
  for (auto i = LocalId{0}; i < env.stateBefore.locals.size(); ++i) {
    if (setLocCouldHaveSideEffects(env, i, true)) {
      addLocGen(env, i);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// manipulate eval stack

void pop(Env& env, UseInfo&& ui) {
  FTRACE(2, "      pop({})\n", show(ui.usage));
  env.dceState.stack.push_back(std::move(ui));
}

void pop(Env& env, Use u, DceActionMap actions) {
  pop(env, {u, std::move(actions)});
}

void pop(Env& env, Use u, InstrId id) {
  pop(env, {u, {{id, DceAction::Kill}}});
}

void pop(Env& env) { pop(env, Use::Used, DceActionMap{}); }

void discard(Env& env) {
  pop(env, Use::Not, env.id);
}

//////////////////////////////////////////////////////////////////////

/*
 * Mark a UseInfo used; if its linked also mark the ui below it used.
 * As we mark them used, we may also need to add them to
 * forcedLiveLocations to prevent inconsistencies in the global state
 * (see markUisLive).
 */
void use(LocationIdSet& forcedLive,
         std::vector<UseInfo>& uis, uint32_t i) {
  while (true) {
    auto& ui = uis[i];
    auto linked = isLinked(ui);
    if (ui.usage != Use::Used &&
        ui.location.blk != NoBlockId) {
      forcedLive.insert(ui.location);
    }
    ui.usage = Use::Used;
    ui.actions.clear();
    if (!linked) break;
    assert(i);
    i--;
  }
}

void combineActions(DceActionMap& dst, const DceActionMap& src) {
  for (auto& i : src) {
    auto ret = dst.insert(i);
    if (!ret.second) {
      if (i.second.action == DceAction::MinstrStackFixup ||
          i.second.action == DceAction::MinstrStackFinal) {
        assertx(i.second.action == ret.first->second.action);
        ret.first->second.mask |= i.second.mask;
        continue;
      }

      if (i.second.action == ret.first->second.action) {
        continue;
      }

      assertx(i.second.action == DceAction::Kill ||
              ret.first->second.action == DceAction::Kill);

      if (i.second.action == DceAction::PopAndReplace ||
          ret.first->second.action == DceAction::PopAndReplace) {
        ret.first->second.action = DceAction::Replace;
        continue;
      }
      if (i.second.action == DceAction::Replace ||
          ret.first->second.action == DceAction::Replace) {
        ret.first->second.action = DceAction::Replace;
        continue;
      }
      if (ret.first->second.action == DceAction::PopInputs ||
          i.second.action == DceAction::PopInputs) {
        ret.first->second.action = DceAction::Kill;
        continue;
      }
      always_assert(false);
    }
  }
}

/*
 * A UseInfo has been popped, and we want to perform its actions. If
 * its not linked we'll just add them to the dceState; otherwise we'll
 * add them to the UseInfo on the top of the stack.
 */
DceActionMap& commitActions(Env& env, bool linked, const DceActionMap& am) {
  if (!linked) {
    FTRACE(2, "     committing {}: {}\n", show(env.id), show(am));
  }

  assert(!linked || env.dceState.stack.back().usage != Use::Used);

  auto& dst = linked ?
    env.dceState.stack.back().actions : env.dceState.actionMap;

  combineActions(dst, am);

  if (!am.count(env.id)) {
    dst.emplace(env.id, DceAction::Kill);
  }
  return dst;
}

/*
 * Combine a set of UseInfos into the first, and return the combined one.
 */
UseInfo& combineUis(UseInfo& ui) { return ui; }
template<class... Args>
UseInfo& combineUis(UseInfo& accum, const UseInfo& ui, const Args&... args) {
  accum.actions.insert(begin(ui.actions), end(ui.actions));
  if (accum.location.blk == NoBlockId ||
      accum.location < ui.location) {
    accum.location = ui.location;
  }
  return combineUis(accum, args...);
}

/*
 * The current instruction is going to be replaced with PopCs. Perform
 * appropriate pops (Use::Not)
 */
void ignoreInputs(Env& env, bool linked, DceActionMap&& actions) {
  auto const np = numPop(env.op);
  if (!np) return;

  auto usage = [&] (uint32_t i) {
    auto ret = Use::Not;
    if (linked) ret = ret | Use::Linked;
    return ret;
  };

  for (auto i = np; --i; ) {
    pop(env, usage(i), DceActionMap {});
    linked = true;
  }
  pop(env, usage(0), std::move(actions));
}

DceActionMap& commitUis(Env& env, bool linked, const UseInfo& ui) {
  return commitActions(env, linked, ui.actions);
}

template<typename... Args>
DceActionMap& commitUis(Env& env, bool linked,
                        const UseInfo& ui, const Args&... args) {
  commitUis(env, linked, ui);
  return commitUis(env, linked, args...);
}

/*
 * During global dce, a particular stack element could be pushed
 * on multiple paths. eg:
 *
 *   $a ? f() : 42
 *
 * If f() is known to return a non-counted type, we have FCall -> PopC on one
 * path, and Int 42 -> PopC on another, and the PopC marks its value Use::Not.
 * When we get to the Int 42 it thinks both instructions can be killed; but when
 * we get to the FCall it does nothing. So any time we decide to ignore a
 * Use::Not , we have to record that fact so we can prevent the other paths from
 * trying to use that information. We communicate this via the ui.location
 * field, and the forcedLiveLocations set.
 *
 * [ We deal with this case now by inserting a PopC after the
 *   FCall, which allows the 42/PopC to be removed - but there are
 *   other cases that are not yet handled. ]
 */
void markUisLive(Env& env, bool linked, const UseInfo& ui) {
  validate(ui.usage);
  if (ui.usage != Use::Used &&
      ui.location.blk != NoBlockId) {
    env.dceState.forcedLiveLocations.insert(ui.location);
  }
  if (linked) {
    use(env.dceState.forcedLiveLocations,
        env.dceState.stack, env.dceState.stack.size() - 1);
  }
}

template<typename... Args>
void markUisLive(Env& env, bool linked,
                 const UseInfo& ui, const Args&... args) {
  markUisLive(env, false, ui);
  markUisLive(env, linked, args...);
}

template<typename... Args>
void popOutputs(Env& env, bool linked, const Args&... args) {
  if (shouldPopOutputs(env, args...)) {
    auto& actions = commitUis(env, linked, args...);
    actions[env.id] = DceAction::PopOutputs;
    return;
  }
  markUisLive(env, linked, args...);
}

void markDead(Env& env) {
  env.dceState.actionMap[env.id] = DceAction::Kill;
  FTRACE(2, "     Killing {}\n", show(env.id));
}

void pop_inputs(Env& env, uint32_t numPop) {
  for (auto i = uint32_t{0}; i < numPop; ++i) {
    pop(env);
  }
}

enum class PushFlags {
  MarkLive,
  MarkDead,
  MarkUnused,
  PopOutputs,
  AddElemC
};

template<typename... Args>
void handle_push(Env& env, PushFlags pf, Args&... uis) {
  auto linked = lastUiIsLinked(uis...);

  if (env.loc != NoLocalId && (linked ||
                               pf == PushFlags::MarkLive ||
                               pf == PushFlags::PopOutputs)) {
    addLocGen(env, env.loc);
  }

  switch (pf) {
    case PushFlags::MarkLive:
      markUisLive(env, linked, uis...);
      break;

    case PushFlags::MarkDead:
      commitUis(env, linked, uis...);
      break;

    case PushFlags::MarkUnused: {
      // Our outputs are unused, their consumers are being removed,
      // and we don't care about our inputs. Replace with
      // appropriately unused pops of the inputs.
      auto& ui = combineUis(uis...);
      // use emplace so that the callee can override the action
      ui.actions.emplace(env.id, DceAction::PopInputs);
      commitActions(env, linked, ui.actions);
      if (numPop(env.op)) {
        ignoreInputs(env, linked, {{ env.id, DceAction::Kill }});
      }
      return;
    }

    case PushFlags::PopOutputs:
      popOutputs(env, linked, uis...);
      break;

    case PushFlags::AddElemC: {
      assert(!linked);
      auto& ui = combineUis(uis...);
      ui.usage = Use::AddElemC;
      // For the last AddElemC, we will already have added a Replace
      // action for env.id - so the following emplace will silently
      // fail; for the rest, we just want to kill the AddElemC
      ui.actions.emplace(env.id, DceAction::Kill);
      pop(env, std::move(ui));

      // The key is known; we must drop it if we convert to New*Array.
      pop(env, Use::Not | Use::Linked, DceActionMap {});

      // The value going into the array is a normal use.
      pop(env);
      return;
    }
  }
  pop_inputs(env, numPop(env.op));
}

template<typename F>
auto stack_ops(Env& env, F fun)
  -> decltype(fun(std::declval<UseInfo&>()),void(0)) {
  always_assert(!env.dceState.stack.empty());
  auto ui = std::move(env.dceState.stack.back());
  env.dceState.stack.pop_back();
  FTRACE(2, "      stack_ops({}@{})\n", show(ui.usage), show(ui.actions));
  env.loc = NoLocalId;
  auto const f = fun(ui);
  handle_push(env, f, ui);
}

void stack_ops(Env& env) {
  stack_ops(env, [](const UseInfo&) { return PushFlags::MarkLive; });
}

template<typename F>
auto stack_ops(Env& env, F fun)
  -> decltype(fun(std::declval<UseInfo&>(), std::declval<UseInfo&>()),void(0)) {
  always_assert(env.dceState.stack.size() >= 2);
  auto u1 = std::move(env.dceState.stack.back());
  env.dceState.stack.pop_back();
  auto u2 = std::move(env.dceState.stack.back());
  env.dceState.stack.pop_back();
  FTRACE(2, "      stack_ops({}@{},{}@{})\n",
         show(u1.usage), show(u1.actions), show(u1.usage), show(u2.actions));
  env.loc = NoLocalId;
  auto const f = fun(u1, u2);
  handle_push(env, f, u1, u2);
}

void push_outputs(Env& env, uint32_t numPush) {
  for (auto i = uint32_t{0}; i < numPush; ++i) {
    auto ui = std::move(env.dceState.stack.back());
    env.dceState.stack.pop_back();
    markUisLive(env, isLinked(ui), ui);
  }
}

void pushRemovable(Env& env) {
  stack_ops(env,[] (const UseInfo& ui) {
      return allUnused(ui) ? PushFlags::MarkUnused : PushFlags::MarkLive;
    });
}

void pushRemovableIfNoThrow(Env& env) {
  stack_ops(env,[&] (const UseInfo& ui) {
      return !env.flags.wasPEI && allUnused(ui)
             ? PushFlags::MarkUnused : PushFlags::MarkLive;
    });
}

//////////////////////////////////////////////////////////////////////

/*
 * Note that the instructions with popConds are relying on the consumer of the
 * values they push to check whether lifetime changes can have side-effects.
 *
 * For example, in bytecode like this, assuming $x is an object with a
 * destructor:
 *
 *   CGetL $x
 *   UnsetL $x
 *   // ...
 *   PopC $x // dtor should be here.
 *
 * The PopC will decide it can't be eliminated, which prevents us from
 * eliminating the CGetL.
 */

void dce(Env& env, const bc::PopC&)          { discard(env); }
void dce(Env& env, const bc::PopV&)          { discard(env); }
void dce(Env& env, const bc::PopU&)          { discard(env); }
void dce(Env& env, const bc::PopU2&)         {
  auto ui = std::move(env.dceState.stack.back());
  env.dceState.stack.pop_back();
  if (isLinked(ui)) {
    // this is way too conservative; but hopefully the PopU2 will be
    // killed (it always should be) and we'll do another pass and fix
    // it.
    markUisLive(env, true, ui);
    ui = UseInfo { Use::Used };
  }
  discard(env);
  env.dceState.stack.push_back(std::move(ui));
}
void dce(Env& env, const bc::PopFrame& op) {
  std::vector<UseInfo> uis;
  for (uint32_t i = 0; i < op.arg1; i++) {
    uis.emplace_back(std::move(env.dceState.stack.back()));
    env.dceState.stack.pop_back();

    // As above this is overly conservative but in all likelihood won't matter
    if (isLinked(uis.back())) {
      markUisLive(env, true, uis.back());
      uis.back() = UseInfo { Use::Used };
    }
  }

  // Pop the Uninits from the frame
  pop(env, Use::Not, DceActionMap {});
  pop(env, Use::Not|Use::Linked, DceActionMap {});
  pop(env, Use::Not|Use::Linked, DceActionMap {{ env.id, DceAction::Kill }});

  for (auto it = uis.rbegin(); it != uis.rend(); it++) {
    env.dceState.stack.push_back(std::move(*it));
  }
}

void dce(Env& env, const bc::Int&)           { pushRemovable(env); }
void dce(Env& env, const bc::String&)        { pushRemovable(env); }
void dce(Env& env, const bc::Dict&)          { pushRemovable(env); }
void dce(Env& env, const bc::Vec&)           { pushRemovable(env); }
void dce(Env& env, const bc::Keyset&)        { pushRemovable(env); }
void dce(Env& env, const bc::Double&)        { pushRemovable(env); }
void dce(Env& env, const bc::True&)          { pushRemovable(env); }
void dce(Env& env, const bc::False&)         { pushRemovable(env); }
void dce(Env& env, const bc::Null&)          { pushRemovable(env); }
void dce(Env& env, const bc::NullUninit&)    { pushRemovable(env); }
void dce(Env& env, const bc::File&)          { pushRemovable(env); }
void dce(Env& env, const bc::Dir&)           { pushRemovable(env); }
void dce(Env& env, const bc::FuncCred&)      { pushRemovable(env); }
void dce(Env& env, const bc::NewArray&)      { pushRemovable(env); }
void dce(Env& env, const bc::NewCol&)        { pushRemovable(env); }
void dce(Env& env, const bc::CheckProp&)     { pushRemovable(env); }

void dce(Env& env, const bc::Dup&) {
  // Various cases:
  //   - If the duplicated value is not used, delete the dup and all
  //     its consumers, then
  //     o If the original value is unused pass that on to let the
  //       instruction which pushed it decide whether to kill it
  //     o Otherwise we're done.
  //   - Otherwise, if the original value is unused, kill the dup
  //     and all dependent instructions.
  stack_ops(env, [&] (UseInfo& dup, UseInfo& orig) {
      // Dup pushes a cell that is guaranteed to be not the last reference.
      // So, it can be eliminated if the cell it pushes is used as Use::Not.
      auto const dup_unused = allUnusedIfNotLastRef(dup);
      auto const orig_unused = allUnused(orig) &&
        (!isLinked(dup) || dup_unused);

      if (dup_unused && orig_unused) {
        return PushFlags::MarkUnused;
      }

      if (dup_unused) {
        markUisLive(env, isLinked(orig), orig);
        orig.actions.clear();
        return PushFlags::MarkDead;
      }

      if (orig_unused) {
        markUisLive(env, false, dup);
        dup.actions.clear();
        return PushFlags::MarkDead;
      }

      return PushFlags::MarkLive;
    });
}

void cgetImpl(Env& env, LocalId loc, bool quiet) {
  stack_ops(env, [&] (UseInfo& ui) {
      scheduleGenLoc(env, loc);
      if (allUnused(ui) &&
          (quiet || !readCouldHaveSideEffects(locRaw(env, loc)))) {
        return PushFlags::MarkUnused;
      }
      if (!isLocLive(env, loc) &&
          !setCouldHaveSideEffects(locRaw(env, loc)) &&
          !readCouldHaveSideEffects(locRaw(env, loc)) &&
          !is_volatile_local(env.dceState.ainfo.ctx.func, loc)) {
        // note: PushL does not deal with Uninit, so we need the
        // readCouldHaveSideEffects here, regardless of quiet.
        env.dceState.replaceMap.insert({ env.id, { bc::PushL { loc } } });
        env.dceState.actionMap.emplace(env.id, DceAction::Replace);
      }
      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::CGetL& op) {
  cgetImpl(env, op.loc1, false);
}

void dce(Env& env, const bc::CGetQuietL& op) {
  cgetImpl(env, op.loc1, true);
}

void dce(Env& env, const bc::CUGetL& op) {
  cgetImpl(env, op.loc1, true);
}

void dce(Env& env, const bc::PushL& op) {
  stack_ops(env, [&] (UseInfo& ui) {
      scheduleGenLoc(env, op.loc1);
      if (allUnused(ui)) {
        if (isLocLive(env, op.loc1)) {
          env.dceState.replaceMap.insert(
            { env.id, { bc::UnsetL { op.loc1 } } });
          ui.actions.emplace(env.id, DceAction::Replace);
        }
        return PushFlags::MarkUnused;
      }
      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::CGetL2& op) {
  auto const ty = locRaw(env, op.loc1);

  stack_ops(env, [&] (const UseInfo& u1, const UseInfo& u2) {
      scheduleGenLoc(env, op.loc1);
      if (readCouldHaveSideEffects(ty) || !allUnused(u1, u2)) {
        return PushFlags::MarkLive;
      }
      return PushFlags::MarkUnused;
    });
}

void dce(Env& env, const bc::BareThis& op) {
  stack_ops(env, [&] (UseInfo& ui) {
      if (allUnusedIfNotLastRef(ui) &&
          op.subop1 != BareThisOp::Notice) {
        return PushFlags::MarkUnused;
      }
      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::RetC&)  { pop(env); readDtorLocs(env); }
void dce(Env& env, const bc::RetCSuspended&) { pop(env); readDtorLocs(env); }
void dce(Env& env, const bc::Throw&) { pop(env); readDtorLocs(env); }
void dce(Env& env, const bc::Fatal&) { pop(env); readDtorLocs(env); }
void dce(Env& env, const bc::Exit&)  { stack_ops(env); readDtorLocs(env); }

void dce(Env& env, const bc::IsTypeC& op) {
  stack_ops(env, [&] (UseInfo& ui) {
      if (allUnused(ui) &&
          !is_type_might_raise(type_of_istype(op.subop1), topC(env))) {
        return PushFlags::MarkUnused;
      }
      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::IsTypeL& op) {
  auto const ty = locRaw(env, op.loc1);
  stack_ops(env, [&] (UseInfo& ui) {
      scheduleGenLoc(env, op.loc1);
      if (allUnused(ui) &&
          !readCouldHaveSideEffects(ty) &&
          !is_type_might_raise(type_of_istype(op.subop2), ty)) {
        return PushFlags::MarkUnused;
      }
      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::Array& op) {
  stack_ops(env, [&] (UseInfo& ui) {
      if (allUnusedIfNotLastRef(ui)) return PushFlags::MarkUnused;

      if (ui.usage != Use::AddElemC) return PushFlags::MarkLive;

      assert(!env.dceState.isLocal);

      CompactVector<Bytecode> bcs;
      IterateV(op.arr1, [&] (TypedValue v) {
        bcs.push_back(gen_constant(v));
      });
      env.dceState.replaceMap.emplace(env.id, std::move(bcs));
      ui.actions[env.id] = DceAction::Replace;
      env.dceState.didAddElemOpts  = true;
      return PushFlags::MarkUnused;
    });
}

void dce(Env& env, const bc::NewMixedArray&) {
  stack_ops(env, [&] (const UseInfo& ui) {
      if (ui.usage == Use::AddElemC || allUnused(ui)) {
        env.dceState.didAddElemOpts  = true;
        return PushFlags::MarkUnused;
      }

      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::NewDictArray&) {
  stack_ops(env, [&] (const UseInfo& ui) {
      if (ui.usage == Use::AddElemC || allUnused(ui)) {
        env.dceState.didAddElemOpts  = true;
        return PushFlags::MarkUnused;
      }

      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::NewDArray&) {
  stack_ops(env, [&] (const UseInfo& ui) {
      if (ui.usage == Use::AddElemC || allUnused(ui)) {
        env.dceState.didAddElemOpts  = true;
        return PushFlags::MarkUnused;
      }

      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::AddElemC& /*op*/) {
  stack_ops(env, [&] (UseInfo& ui) {
      // If the set might throw it needs to be kept.
      if (env.flags.wasPEI) {
        return PushFlags::MarkLive;
      }

      if (allUnused(ui)) {
        return PushFlags::MarkUnused;
      }

      if (env.dceState.isLocal) {
        // Converting to New*Array changes the stack layout,
        // which can invalidate the FuncAnalysis; only do it in global
        // dce, since we're going to recompute the FuncAnalysis anyway.
        return PushFlags::MarkLive;
      }

      auto const& arrPost = env.stateAfter.stack.back().type;
      auto const& arrPre = topC(env, 2);
      auto const postSize = arr_size(arrPost);
      if (!postSize || postSize == arr_size(arrPre)) {
        // if postSize is known, and equal to preSize, the AddElemC
        // didn't add an element (duplicate key) so we have to give up.
        // if its not known, we also have nothing to do.
        return PushFlags::MarkLive;
      }
      if (ui.usage == Use::AddElemC) {
        return PushFlags::AddElemC;
      }
      auto const cat = categorize_array(arrPost);
      if (cat.hasValue) {
        if (allUnusedIfNotLastRef(ui)) return PushFlags::MarkUnused;
        auto v = tv(arrPost);
        CompactVector<Bytecode> bcs;
        if (arrPost.subtypeOf(BArrN)) {
          bcs.emplace_back(bc::Array { v->m_data.parr });
        } else {
          assert(arrPost.subtypeOf(BDictN));
          bcs.emplace_back(bc::Dict { v->m_data.parr });
        }
        env.dceState.replaceMap.emplace(env.id, std::move(bcs));
        ui.actions[env.id] = DceAction::PopAndReplace;
        return PushFlags::MarkUnused;
      }

      if (isLinked(ui)) return PushFlags::MarkLive;

      if (arrPost.strictSubtypeOf(TArrN)) {
        CompactVector<Bytecode> bcs;
        if (cat.cat == Type::ArrayCat::Struct &&
            *postSize <= ArrayData::MaxElemsOnStack) {
          if (arrPost.subtypeOf(BPArrN)) {
            bcs.emplace_back(bc::NewStructArray { get_string_keys(arrPost) });
          } else if (arrPost.subtypeOf(BDArrN)) {
            bcs.emplace_back(bc::NewStructDArray { get_string_keys(arrPost) });
          } else {
            return PushFlags::MarkLive;
          }
        } else if (cat.cat == Type::ArrayCat::Packed &&
                   *postSize <= ArrayData::MaxElemsOnStack) {
          if (arrPost.subtypeOf(BPArrN)) {
            bcs.emplace_back(
              bc::NewPackedArray { static_cast<uint32_t>(*postSize) }
            );
          } else if (arrPost.subtypeOf(BVArrN)) {
            bcs.emplace_back(
              bc::NewVArray { static_cast<uint32_t>(*postSize) }
            );
          } else {
            return PushFlags::MarkLive;
          }
        } else {
          return PushFlags::MarkLive;
        }
        env.dceState.replaceMap.emplace(env.id, std::move(bcs));
        ui.actions[env.id] = DceAction::Replace;
        return PushFlags::AddElemC;
      }

      if (arrPost.strictSubtypeOf(TDictN) &&
          cat.cat == Type::ArrayCat::Struct &&
          *postSize <= ArrayData::MaxElemsOnStack) {
        CompactVector<Bytecode> bcs;
        bcs.emplace_back(bc::NewStructDict { get_string_keys(arrPost) });
        env.dceState.replaceMap.emplace(env.id, std::move(bcs));
        ui.actions[env.id] = DceAction::Replace;
        return PushFlags::AddElemC;
      }

      return PushFlags::MarkLive;
    });
}

template<typename Op>
void dceNewArrayLike(Env& env, const Op& op) {
  if (op.numPop() == 1 &&
      !env.flags.wasPEI &&
      allUnusedIfNotLastRef(env.dceState.stack.back())) {
    // Just an optimization for when we have a single element array,
    // but we care about its lifetime. By killing the array, and
    // leaving its element on the stack, the lifetime is unaffected.
    return markDead(env);
  }
  pushRemovableIfNoThrow(env);
}

void dce(Env& env, const bc::NewPackedArray& op)  { dceNewArrayLike(env, op); }
void dce(Env& env, const bc::NewStructArray& op)  { dceNewArrayLike(env, op); }
void dce(Env& env, const bc::NewStructDArray& op) { dceNewArrayLike(env, op); }
void dce(Env& env, const bc::NewStructDict& op)   { dceNewArrayLike(env, op); }
void dce(Env& env, const bc::NewVecArray& op)     { dceNewArrayLike(env, op); }
void dce(Env& env, const bc::NewKeysetArray& op)  { dceNewArrayLike(env, op); }
void dce(Env& env, const bc::NewVArray& op)       { dceNewArrayLike(env, op); }

void dce(Env& env, const bc::NewPair& op)         { dceNewArrayLike(env, op); }
void dce(Env& env, const bc::ColFromArray& op)    { dceNewArrayLike(env, op); }

void dce(Env& env, const bc::PopL& op) {
  auto const effects = setLocCouldHaveSideEffects(env, op.loc1);
  if (!isLocLive(env, op.loc1) && !effects) {
    assert(!locRaw(env, op.loc1).couldBe(BRef));
    discard(env);
    env.dceState.actionMap[env.id] = DceAction::PopInputs;
    return;
  }
  pop(env);
  if (effects || locRaw(env, op.loc1).couldBe(BRef)) {
    addLocGen(env, op.loc1);
  } else {
    addLocKill(env, op.loc1);
  }
}

void dce(Env& env, const bc::InitThisLoc& op) {
  if (!isLocLive(env, op.loc1)) {
    return markDead(env);
  }
}

void dce(Env& env, const bc::SetL& op) {
  auto const effects = setLocCouldHaveSideEffects(env, op.loc1);
  if (!isLocLive(env, op.loc1) && !effects) {
    assert(!locRaw(env, op.loc1).couldBe(BRef));
    return markDead(env);
  }
  stack_ops(env, [&] (UseInfo& ui) {
    if (!allUnusedIfNotLastRef(ui)) return PushFlags::MarkLive;
    // If the stack result of the SetL is unused, we can replace it
    // with a PopL.
    CompactVector<Bytecode> bcs { bc::PopL { op.loc1 } };
    env.dceState.replaceMap.emplace(env.id, std::move(bcs));
    ui.actions[env.id] = DceAction::Replace;
    return PushFlags::MarkDead;
  });
  if (effects || locRaw(env, op.loc1).couldBe(BRef)) {
    addLocGen(env, op.loc1);
  } else {
    addLocKill(env, op.loc1);
  }
}

void dce(Env& env, const bc::UnsetL& op) {
  auto const oldTy   = locRaw(env, op.loc1);
  if (oldTy.subtypeOf(BUninit)) return markDead(env);

  // Unsetting a local bound to a static never has side effects
  // because the static itself has a reference to the value.
  auto const effects = setLocCouldHaveSideEffects(env, op.loc1);

  if (!isLocLive(env, op.loc1) && !effects) return markDead(env);
  if (effects) {
    addLocGen(env, op.loc1);
  } else {
    addLocKill(env, op.loc1);
  }
}

/*
 * IncDecL is a read-modify-write: can be removed if the local isn't
 * live, the set can't have side effects, and no one reads the value
 * it pushes.  If the instruction is not dead, add the local to the
 * set of upward exposed uses.
 */
void dce(Env& env, const bc::IncDecL& op) {
  auto const oldTy   = locRaw(env, op.loc1);
  auto const effects = setLocCouldHaveSideEffects(env, op.loc1) ||
                         readCouldHaveSideEffects(oldTy);
  stack_ops(env, [&] (const UseInfo& ui) {
      scheduleGenLoc(env, op.loc1);
      if (!isLocLive(env, op.loc1) && !effects && allUnused(ui)) {
        return PushFlags::MarkUnused;
      }
      return PushFlags::MarkLive;
    });
}

bool setOpLSideEffects(const bc::SetOpL& op, const Type& lhs, const Type& rhs) {
  auto const lhsOk = lhs.subtypeOfAny(TUninit, TNull, TBool, TInt, TDbl, TStr);
  auto const rhsOk = rhs.subtypeOfAny(TNull, TBool, TInt, TDbl, TStr);
  if (!lhsOk || !rhsOk) return true;

  switch (op.subop2) {
    case SetOpOp::ConcatEqual:
      return false;

    case SetOpOp::PlusEqual:
    case SetOpOp::PlusEqualO:
    case SetOpOp::MinusEqual:
    case SetOpOp::MulEqual:
    case SetOpOp::DivEqual:
    case SetOpOp::ModEqual:
    case SetOpOp::PowEqual:
    case SetOpOp::AndEqual:
    case SetOpOp::OrEqual:
    case SetOpOp::XorEqual:
    case SetOpOp::MinusEqualO:
    case SetOpOp::MulEqualO:
    case SetOpOp::SlEqual:
    case SetOpOp::SrEqual:
      return lhs.subtypeOf(BStr) || rhs.subtypeOf(BStr);
  }
  not_reached();
}

/*
 * SetOpL is like IncDecL, but with the complication that we don't
 * know if we can mark it dead when visiting it, because it is going
 * to pop an input but unlike SetL doesn't push the value it popped.
 * However, since we've checked the input types, it *is* ok to just
 * kill it.
 */
void dce(Env& env, const bc::SetOpL& op) {
  auto const oldTy   = locRaw(env, op.loc1);

  stack_ops(env, [&] (UseInfo& ui) {
      scheduleGenLoc(env, op.loc1);
      if (!isLocLive(env, op.loc1) && allUnused(ui)) {
        if (!setLocCouldHaveSideEffects(env, op.loc1) &&
            !readCouldHaveSideEffects(oldTy) &&
            !setOpLSideEffects(op, oldTy, topC(env))) {
          return PushFlags::MarkUnused;
        }
      }
      return PushFlags::MarkLive;
    });
}

void dce(Env& env, const bc::Add&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::AddNewElemC&)      { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::AddO&)             { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::AKExists&)         { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::ArrayIdx&)         { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::BitAnd&)           { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::BitNot&)           { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::BitOr&)            { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::BitXor&)           { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastArray&)        { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastBool&)         { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastDArray&)       { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastDict&)         { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastDouble&)       { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastInt&)          { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastKeyset&)       { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastObject&)       { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastString&)       { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastVArray&)       { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CastVec&)          { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Cmp&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::CombineAndResolveTypeStruct&) {
  pushRemovableIfNoThrow(env);
}
void dce(Env& env, const bc::Concat&)           { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::ConcatN&)          { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::DblAsBits&)        { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Div&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Eq&)               { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Gt&)               { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Gte&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Idx&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::IsLateBoundCls&)   { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::IsTypeStructC&)    { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Lt&)               { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Lte&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Mod&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Mul&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::MulO&)             { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Neq&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Not&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::NSame&)            { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Pow&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::ReifiedName&)      { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Same&)             { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Shl&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Shr&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Sub&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::SubO&)             { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Xor&)              { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::LateBoundCls&)     { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Self&)             { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::Parent&)           { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::ClassName&)        { pushRemovableIfNoThrow(env); }
void dce(Env& env, const bc::ClassGetC&)        { pushRemovableIfNoThrow(env); }

/*
 * Default implementation is conservative: assume we use all of our
 * inputs, and can't be removed even if our output is unused.
 *
 * We also assume all the locals in the mayReadLocalSet must be
 * added to the live local set, and don't remove anything from it.
 */

template<class Op>
void no_dce(Env& env, const Op& op) {
  addLocGenSet(env, env.flags.mayReadLocalSet);
  push_outputs(env, op.numPush());
  pop_inputs(env, op.numPop());
}

void dce(Env& env, const bc::AliasCls& op) { no_dce(env, op); }
void dce(Env& env, const bc::AssertRATL& op) { no_dce(env, op); }
void dce(Env& env, const bc::AssertRATStk& op) { no_dce(env, op); }
void dce(Env& env, const bc::Await& op) { no_dce(env, op); }
void dce(Env& env, const bc::AwaitAll& op) { no_dce(env, op); }
void dce(Env& env, const bc::BaseGL& op) { no_dce(env, op); }
void dce(Env& env, const bc::BaseH& op) { no_dce(env, op); }
void dce(Env& env, const bc::BaseL& op) { no_dce(env, op); }
void dce(Env& env, const bc::BreakTraceHint& op) { no_dce(env, op); }
void dce(Env& env, const bc::CGetCUNop& op) { no_dce(env, op); }
void dce(Env& env, const bc::CGetG& op) { no_dce(env, op); }
void dce(Env& env, const bc::CGetS& op) { no_dce(env, op); }
void dce(Env& env, const bc::ChainFaults& op) { no_dce(env, op); }
void dce(Env& env, const bc::CheckReifiedGenericMismatch& op) {
  no_dce(env, op);
}
void dce(Env& env, const bc::CheckThis& op) { no_dce(env, op); }
void dce(Env& env, const bc::Clone& op) { no_dce(env, op); }
void dce(Env& env, const bc::ClsCns& op) { no_dce(env, op); }
void dce(Env& env, const bc::ClsCnsD& op) { no_dce(env, op); }
void dce(Env& env, const bc::ClassGetTS& op) { no_dce(env, op); }
void dce(Env& env, const bc::CnsE& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContAssignDelegate& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContCheck& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContCurrent& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContEnter& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContEnterDelegate& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContGetReturn& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContKey& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContRaise& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContUnsetDelegate& op) { no_dce(env, op); }
void dce(Env& env, const bc::ContValid& op) { no_dce(env, op); }
void dce(Env& env, const bc::CreateCl& op) { no_dce(env, op); }
void dce(Env& env, const bc::CreateCont& op) { no_dce(env, op); }
void dce(Env& env, const bc::DefCls& op) { no_dce(env, op); }
void dce(Env& env, const bc::DefClsNop& op) { no_dce(env, op); }
void dce(Env& env, const bc::DefCns& op) { no_dce(env, op); }
void dce(Env& env, const bc::DefRecord& op) { no_dce(env, op); }
void dce(Env& env, const bc::DefTypeAlias& op) { no_dce(env, op); }
void dce(Env& env, const bc::EmptyG& op) { no_dce(env, op); }
void dce(Env& env, const bc::EmptyL& op) { no_dce(env, op); }
void dce(Env& env, const bc::EmptyS& op) { no_dce(env, op); }
void dce(Env& env, const bc::EntryNop& op) { no_dce(env, op); }
void dce(Env& env, const bc::Eval& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCall& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallBuiltin& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallClsMethod& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallClsMethodD& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallClsMethodRD& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallClsMethodS& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallClsMethodSD& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallClsMethodSRD& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallCtor& op) { no_dce(env, op); }
void dce(Env& env, const bc::FPushFunc& op) { no_dce(env, op); }
void dce(Env& env, const bc::FPushFuncD& op) { no_dce(env, op); }
void dce(Env& env, const bc::FPushFuncRD& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallObjMethod& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallObjMethodD& op) { no_dce(env, op); }
void dce(Env& env, const bc::FCallObjMethodRD& op) { no_dce(env, op); }
void dce(Env& env, const bc::GetMemoKeyL& op) { no_dce(env, op); }
void dce(Env& env, const bc::IncDecG& op) { no_dce(env, op); }
void dce(Env& env, const bc::IncDecS& op) { no_dce(env, op); }
void dce(Env& env, const bc::Incl& op) { no_dce(env, op); }
void dce(Env& env, const bc::InclOnce& op) { no_dce(env, op); }
void dce(Env& env, const bc::InitProp& op) { no_dce(env, op); }
void dce(Env& env, const bc::InstanceOf& op) { no_dce(env, op); }
void dce(Env& env, const bc::InstanceOfD& op) { no_dce(env, op); }
void dce(Env& env, const bc::IssetG& op) { no_dce(env, op); }
void dce(Env& env, const bc::IssetL& op) { no_dce(env, op); }
void dce(Env& env, const bc::IssetS& op) { no_dce(env, op); }
void dce(Env& env, const bc::IterBreak& op) { no_dce(env, op); }
void dce(Env& env, const bc::IterFree& op) { no_dce(env, op); }
void dce(Env& env, const bc::IterInit& op) { no_dce(env, op); }
void dce(Env& env, const bc::IterInitK& op) { no_dce(env, op); }
void dce(Env& env, const bc::IterNext& op) { no_dce(env, op); }
void dce(Env& env, const bc::IterNextK& op) { no_dce(env, op); }
void dce(Env& env, const bc::Jmp& op) { no_dce(env, op); }
void dce(Env& env, const bc::JmpNS& op) { no_dce(env, op); }
void dce(Env& env, const bc::JmpNZ& op) { no_dce(env, op); }
void dce(Env& env, const bc::JmpZ& op) { no_dce(env, op); }
void dce(Env& env, const bc::LIterFree& op) { no_dce(env, op); }
void dce(Env& env, const bc::LIterInit& op) { no_dce(env, op); }
void dce(Env& env, const bc::LIterInitK& op) { no_dce(env, op); }
void dce(Env& env, const bc::LIterNext& op) { no_dce(env, op); }
void dce(Env& env, const bc::LIterNextK& op) { no_dce(env, op); }
void dce(Env& env, const bc::MemoGet& op) { no_dce(env, op); }
void dce(Env& env, const bc::MemoGetEager& op) { no_dce(env, op); }
void dce(Env& env, const bc::MemoSet& op) { no_dce(env, op); }
void dce(Env& env, const bc::MemoSetEager& op) { no_dce(env, op); }
void dce(Env& env, const bc::Method& op) { no_dce(env, op); }
void dce(Env& env, const bc::NativeImpl& op) { no_dce(env, op); }
void dce(Env& env, const bc::NewLikeArrayL& op) { no_dce(env, op); }
void dce(Env& env, const bc::NewObj& op) { no_dce(env, op); }
void dce(Env& env, const bc::NewObjR& op) { no_dce(env, op); }
void dce(Env& env, const bc::NewObjD& op) { no_dce(env, op); }
void dce(Env& env, const bc::NewObjRD& op) { no_dce(env, op); }
void dce(Env& env, const bc::NewObjS& op) { no_dce(env, op); }
void dce(Env& env, const bc::LockObj& op) { no_dce(env, op); }
void dce(Env& env, const bc::NewRecord& op) { no_dce(env, op); }
void dce(Env& env, const bc::Nop& op) { no_dce(env, op); }
void dce(Env& env, const bc::OODeclExists& op) { no_dce(env, op); }
void dce(Env& env, const bc::Print& op) { no_dce(env, op); }
void dce(Env& env, const bc::RecordReifiedGeneric& op) { no_dce(env, op); }
void dce(Env& env, const bc::Req& op) { no_dce(env, op); }
void dce(Env& env, const bc::ReqDoc& op) { no_dce(env, op); }
void dce(Env& env, const bc::ReqOnce& op) { no_dce(env, op); }
void dce(Env& env, const bc::ResolveClsMethod& op) { no_dce(env, op); }
void dce(Env& env, const bc::ResolveFunc& op) { no_dce(env, op); }
void dce(Env& env, const bc::ResolveObjMethod& op) { no_dce(env, op); }
void dce(Env& env, const bc::RetM& op) { no_dce(env, op); }
void dce(Env& env, const bc::Select& op) { no_dce(env, op); }
void dce(Env& env, const bc::SetG& op) { no_dce(env, op); }
void dce(Env& env, const bc::SetOpG& op) { no_dce(env, op); }
void dce(Env& env, const bc::SetOpS& op) { no_dce(env, op); }
void dce(Env& env, const bc::SetRangeM& op) { no_dce(env, op); }
void dce(Env& env, const bc::SetS& op) { no_dce(env, op); }
void dce(Env& env, const bc::Silence& op) { no_dce(env, op); }
void dce(Env& env, const bc::SSwitch& op) { no_dce(env, op); }
void dce(Env& env, const bc::Switch& op) { no_dce(env, op); }
void dce(Env& env, const bc::This& op) { no_dce(env, op); }
void dce(Env& env, const bc::ThrowAsTypeStructException& op) {
  no_dce(env, op);
}
void dce(Env& env, const bc::UGetCUNop& op) { no_dce(env, op); }
void dce(Env& env, const bc::UnsetG& op) { no_dce(env, op); }
void dce(Env& env, const bc::VerifyOutType& op) { no_dce(env, op); }
void dce(Env& env, const bc::VerifyParamType& op) { no_dce(env, op); }
void dce(Env& env, const bc::VerifyParamTypeTS& op) { no_dce(env, op); }
void dce(Env& env, const bc::VerifyRetNonNullC& op) { no_dce(env, op); }
void dce(Env& env, const bc::VerifyRetTypeC& op) { no_dce(env, op); }
void dce(Env& env, const bc::VerifyRetTypeTS& op) { no_dce(env, op); }
void dce(Env& env, const bc::VGetL& op) { no_dce(env, op); }
void dce(Env& env, const bc::WHResult& op) { no_dce(env, op); }
void dce(Env& env, const bc::Yield& op) { no_dce(env, op); }
void dce(Env& env, const bc::YieldFromDelegate& op) { no_dce(env, op); }
void dce(Env& env, const bc::YieldK& op) { no_dce(env, op); }

/*
 * The minstr instructions can read a cell from the stack without
 * popping it. They need special handling.
 *
 * In addition, final minstr instructions can drop a number of
 * elements from the stack. Its possible that these elements are dead
 * (eg because an MEC somewhere in the sequence was optimized to MEI
 * or MET), so again we need some special handling.
 */

void minstr_touch(Env& env, int32_t depth) {
  assertx(depth < env.dceState.stack.size());
  // First make sure that the stack element we reference, and any
  // linked ones are marked used.
  use(env.dceState.forcedLiveLocations,
      env.dceState.stack, env.dceState.stack.size() - 1 - depth);
  // If any stack slots at a lower depth get killed, we'll have to
  // adjust our stack index accordingly, so add a MinstrStackFixup
  // action to candidate stack slots.
  // We only track slots up to kMaskSize though - so nothing below
  // that will get killed.
  if (depth > DceAction::kMaskSize) depth = DceAction::kMaskSize;
  while (depth--) {
    auto& ui = env.dceState.stack[env.dceState.stack.size() - 1 - depth];
    if (ui.usage != Use::Used) {
      DEBUG_ONLY auto inserted = ui.actions.emplace(
        env.id, DceAction { DceAction::MinstrStackFixup, 1u << depth }).second;
      assertx(inserted);
    }
  }
}

template<class Op>
void minstr_base(Env& env, const Op& op, int32_t ix) {
  no_dce(env, op);
  minstr_touch(env, ix);
}

template<class Op>
void minstr_dim(Env& env, const Op& op) {
  no_dce(env, op);
  if (op.mkey.mcode == MEC || op.mkey.mcode == MPC) {
    minstr_touch(env, op.mkey.idx);
  }
}

template<class Op>
void minstr_final(Env& env, const Op& op, int32_t ndiscard) {
  addLocGenSet(env, env.flags.mayReadLocalSet);
  push_outputs(env, op.numPush());
  auto const numPop = op.numPop();
  auto const stackRead = op.mkey.mcode == MEC || op.mkey.mcode == MPC ?
    op.mkey.idx : numPop;

  for (auto i = numPop; i--; ) {
    if (i == stackRead || i >= DceAction::kMaskSize || i < numPop - ndiscard) {
      pop(env);
    } else {
      pop(env, {Use::Not, {{env.id, {DceAction::MinstrStackFinal, 1u << i}}}});
    }
  }
}

void dce(Env& env, const bc::BaseC& op)       { minstr_base(env, op, op.arg1); }
void dce(Env& env, const bc::BaseGC& op)      { minstr_base(env, op, op.arg1); }
void dce(Env& env, const bc::BaseSC& op)      {
  minstr_base(env, op, op.arg1);
  minstr_base(env, op, op.arg2);
}

void dce(Env& env, const bc::Dim& op)         { minstr_dim(env, op); }

void dce(Env& env, const bc::QueryM& op) {
  if (!env.flags.wasPEI) {
    assertx(!env.dceState.minstrUI);
    auto ui = env.dceState.stack.back();
    if (!isLinked(ui)) {
      if (allUnused(ui)) {
        addLocGenSet(env, env.flags.mayReadLocalSet);
        ui.actions[env.id] = DceAction::Kill;
        ui.location.id = env.id.idx;
        env.dceState.minstrUI.emplace(std::move(ui));
      } else if (auto const val = tv(env.stateAfter.stack.back().type)) {
        addLocGenSet(env, env.flags.mayReadLocalSet);
        CompactVector<Bytecode> bcs { gen_constant(*val) };
        env.dceState.replaceMap.emplace(env.id, std::move(bcs));
        ui.actions[env.id] = DceAction::Replace;
        ui.location.id = env.id.idx;
        env.dceState.minstrUI.emplace(std::move(ui));
      }
    }
  }
  minstr_final(env, op, op.arg1);
}

void dce(Env& env, const bc::SetM& op)       { minstr_final(env, op, op.arg1); }
void dce(Env& env, const bc::IncDecM& op)    { minstr_final(env, op, op.arg1); }
void dce(Env& env, const bc::SetOpM& op)     { minstr_final(env, op, op.arg1); }
void dce(Env& env, const bc::UnsetM& op)     { minstr_final(env, op, op.arg1); }

void dispatch_dce(Env& env, const Bytecode& op) {
#define O(opcode, ...) case Op::opcode: dce(env, op.opcode); return;
  switch (op.op) { OPCODES }
#undef O
  not_reached();
}

using MaskType = DceAction::MaskType;

void m_adj(uint32_t& depth, MaskType mask) {
  auto i = depth;
  if (i > DceAction::kMaskSize) i = DceAction::kMaskSize;
  while (i--) {
    if ((mask >> i) & 1) depth--;
  }
}

void m_adj(MKey& mkey, MaskType mask) {
  if (mkey.mcode == MEC || mkey.mcode == MPC) {
    uint32_t d = mkey.idx;
    m_adj(d, mask);
    mkey.idx = d;
  }
}

template<typename Op>
void m_adj(uint32_t& ndiscard, Op& op, MaskType mask) {
  auto const adjust = op.numPop() - ndiscard + 1;
  ndiscard += adjust;
  m_adj(ndiscard, mask);
  ndiscard -= adjust;
  m_adj(op.mkey, mask);
}

template <typename Op>
void adjustMinstr(Op& /*op*/, MaskType /*mask*/) {
  always_assert(false);
}

void adjustMinstr(bc::BaseC& op, MaskType m)       { m_adj(op.arg1, m); }
void adjustMinstr(bc::BaseGC& op, MaskType m)      { m_adj(op.arg1, m); }
void adjustMinstr(bc::BaseSC& op, MaskType m)      {
  m_adj(op.arg1, m);
  m_adj(op.arg2, m);
}

void adjustMinstr(bc::Dim& op, MaskType m)         { m_adj(op.mkey, m); }

void adjustMinstr(bc::QueryM& op, MaskType m)  { m_adj(op.arg1, op, m); }
void adjustMinstr(bc::SetM& op, MaskType m)    { m_adj(op.arg1, op, m); }
void adjustMinstr(bc::IncDecM& op, MaskType m) { m_adj(op.arg1, op, m); }
void adjustMinstr(bc::SetOpM& op, MaskType m)  { m_adj(op.arg1, op, m); }
void adjustMinstr(bc::UnsetM& op, MaskType m)  { m_adj(op.arg1, op, m); }

void adjustMinstr(Bytecode& op, MaskType m) {
#define O(opcode, ...) case Op::opcode: adjustMinstr(op.opcode, m); return;
  switch (op.op) { OPCODES }
#undef O
  not_reached();
}


//////////////////////////////////////////////////////////////////////

struct DceOutState {
  DceOutState() = default;
  enum Local {};
  explicit DceOutState(Local) : isLocal(true) {
    locLive.set();
    locLiveExn.set();
  }

  /*
   * The union of the liveIn states of each normal successor for
   * locals and stack slots respectively.
   */
  std::bitset<kMaxTrackedLocals>             locLive;

  /*
   * The union of the liveIn states of each exceptional successor for
   * locals and stack slots respectively.
   */
  std::bitset<kMaxTrackedLocals>             locLiveExn;

  /*
   * The union of the dceStacks from the start of the normal successors.
   */
  folly::Optional<std::vector<UseInfo>>      dceStack;

  /*
   * Whether this is for local_dce
   */
  bool                                        isLocal{false};
};

folly::Optional<DceState>
dce_visit(const Index& index,
          const FuncAnalysis& fa,
          CollectedInfo& collect,
          BlockId bid,
          const State& stateIn,
          const DceOutState& dceOutState) {
  if (!stateIn.initialized) {
    /*
     * Skip unreachable blocks.
     *
     * For DCE analysis it is ok to assume the transfer function is
     * the identity on unreachable blocks (i.e. gen and kill sets are
     * empty).  For optimize, we don't need to bother doing anything
     * to these---another pass is responsible for removing completely
     * unreachable blocks.
     */
    return folly::none;
  }

  auto const states = locally_propagated_states(index, fa, collect,
                                                bid, stateIn);

  auto dceState = DceState{ index, fa };
  dceState.liveLocals = dceOutState.locLive;
  dceState.usedLocals = dceOutState.locLive;
  dceState.isLocal    = dceOutState.isLocal;

  if (dceOutState.dceStack) {
    dceState.stack = *dceOutState.dceStack;
    assert(dceState.stack.size() == states.back().first.stack.size());
  } else {
    dceState.stack.resize(states.back().first.stack.size(),
                          UseInfo { Use::Used });
  }

  auto const blk = fa.ctx.func->blocks[bid].get();
  for (uint32_t idx = blk->hhbcs.size(); idx-- > 0;) {
    auto const& op = blk->hhbcs[idx];

    FTRACE(2, "  == #{} {}\n", idx, show(fa.ctx.func, op));

    auto visit_env = Env {
      dceState,
      op,
      { bid, idx },
      NoLocalId,
      states[idx].first,
      states[idx].second,
      states[idx+1].first,
    };
    auto const handled = [&] {
      if (dceState.minstrUI) {
        if (visit_env.flags.wasPEI) {
          dceState.minstrUI.clear();
          return false;
        }
        if (isMemberDimOp(op.op)) {
          dceState.minstrUI->actions[visit_env.id] = DceAction::Kill;
          // we're almost certainly going to delete this op, but we might not,
          // so we need to record its local uses etc just in case.
          return false;
        }
        if (isMemberBaseOp(op.op)) {
          auto const& final = blk->hhbcs[dceState.minstrUI->location.id];
          if (final.numPop()) {
            CompactVector<Bytecode> bcs;
            for (auto i = 0; i < final.numPop(); i++) {
              use(dceState.forcedLiveLocations,
                  dceState.stack, dceState.stack.size() - 1 - i);
              bcs.push_back(bc::PopC {});
            }
            dceState.replaceMap.emplace(visit_env.id, std::move(bcs));
            dceState.minstrUI->actions[visit_env.id] = DceAction::Replace;
          } else {
            dceState.minstrUI->actions[visit_env.id] = DceAction::Kill;
          }
          commitActions(visit_env, false, dceState.minstrUI->actions);
          dceState.minstrUI.clear();
          return true;
        }
      }
      return false;
    }();
    if (!handled) {
      dispatch_dce(visit_env, op);

      /*
       * When we see a PEI, liveness must take into account the fact
       * that we could take an exception edge here (or'ing in the
       * liveExn sets).
       */
      if (states[idx].second.wasPEI) {
        FTRACE(2, "    <-- exceptions\n");
        dceState.liveLocals |= dceOutState.locLiveExn;
      }

      dceState.usedLocals |= dceState.liveLocals;
    }

    FTRACE(4, "    dce stack: {}\n",
      [&] {
        using namespace folly::gen;
        return from(dceState.stack)
          | map([&] (const UseInfo& ui) { return show(ui); })
          | unsplit<std::string>(" ");
      }()
    );
    FTRACE(4, "    interp stack: {}\n",
      [&] {
        using namespace folly::gen;
        return from(states[idx].first.stack)
          | map([&] (auto const& e) { return show(e.type); })
          | unsplit<std::string>(" ");
      }()
    );
    if (dceState.minstrUI) {
      FTRACE(4, "    minstr ui: {}\n", show(*dceState.minstrUI));
    }

    // We're now at the state before this instruction, so the stack
    // sizes must line up.
    assert(dceState.stack.size() == states[idx].first.stack.size());
  }

  dceState.minstrUI.clear();
  return dceState;
}

struct DceAnalysis {
  std::bitset<kMaxTrackedLocals>      locLiveIn;
  std::vector<UseInfo>                stack;
  LocationIdSet                       forcedLiveLocations;
};

DceAnalysis analyze_dce(const Index& index,
                        const FuncAnalysis& fa,
                        CollectedInfo& collect,
                        BlockId bid,
                        const State& stateIn,
                        const DceOutState& dceOutState) {
  if (auto dceState = dce_visit(index, fa, collect,
                                bid, stateIn, dceOutState)) {
    return DceAnalysis {
      dceState->liveLocals,
      dceState->stack,
      dceState->forcedLiveLocations
    };
  }
  return DceAnalysis {};
}

/*
 * Do the actual updates to the bytecodes.
 */
void dce_perform(php::Func& func,
                 const DceActionMap& actionMap,
                 const DceReplaceMap& replaceMap) {

  using It = BytecodeVec::iterator;
  auto setloc = [] (int32_t srcLoc, It start, int n) {
    while (n--) {
      start++->srcLoc = srcLoc;
    }
  };
  for (auto const& elm : actionMap) {
    auto const& id = elm.first;
    auto const b = func.blocks[id.blk].mutate();
    FTRACE(1, "{} {}\n", show(elm), show(func, b->hhbcs[id.idx]));
    switch (elm.second.action) {
      case DceAction::PopInputs:
        // we want to replace the bytecode with pops of its inputs
        if (auto const numToPop = numPop(b->hhbcs[id.idx])) {
          auto const srcLoc = b->hhbcs[id.idx].srcLoc;
          b->hhbcs.erase(b->hhbcs.begin() + id.idx);
          b->hhbcs.insert(b->hhbcs.begin() + id.idx,
                          numToPop,
                          bc::PopC {});
          setloc(srcLoc, b->hhbcs.begin() + id.idx, numToPop);
          break;
        }
        // Fall through
      case DceAction::Kill:
        if (b->hhbcs.size() == 1) {
          // we don't allow empty blocks
          auto const srcLoc = b->hhbcs[0].srcLoc;
          b->hhbcs[0] = bc::Nop {};
          b->hhbcs[0].srcLoc = srcLoc;
        } else {
          b->hhbcs.erase(b->hhbcs.begin() + id.idx);
        }
        break;
      case DceAction::PopOutputs:
      {
        // we want to pop the things that the bytecode pushed
        auto const numToPop = numPush(b->hhbcs[id.idx]);
        b->hhbcs.insert(b->hhbcs.begin() + id.idx + 1,
                        numToPop,
                        bc::PopC {});
        setloc(b->hhbcs[id.idx].srcLoc,
               b->hhbcs.begin() + id.idx + 1, numToPop);
        break;
      }
      case DceAction::Replace:
      {
        auto it = replaceMap.find(id);
        always_assert(it != end(replaceMap) && !it->second.empty());
        auto const srcLoc = b->hhbcs[id.idx].srcLoc;
        b->hhbcs.erase(b->hhbcs.begin() + id.idx);
        b->hhbcs.insert(b->hhbcs.begin() + id.idx,
                        begin(it->second), end(it->second));
        setloc(srcLoc, b->hhbcs.begin() + id.idx, it->second.size());
        break;
      }
      case DceAction::PopAndReplace:
      {
        auto it = replaceMap.find(id);
        always_assert(it != end(replaceMap) && !it->second.empty());
        auto const srcLoc = b->hhbcs[id.idx].srcLoc;
        auto const numToPop = numPop(b->hhbcs[id.idx]);
        b->hhbcs.erase(b->hhbcs.begin() + id.idx);
        b->hhbcs.insert(b->hhbcs.begin() + id.idx,
                        begin(it->second), end(it->second));
        if (numToPop) {
          b->hhbcs.insert(b->hhbcs.begin() + id.idx,
                          numToPop,
                          bc::PopC {});
        }
        setloc(srcLoc, b->hhbcs.begin() + id.idx, numToPop + it->second.size());
        break;
      }
      case DceAction::MinstrStackFinal:
      case DceAction::MinstrStackFixup:
      {
        adjustMinstr(b->hhbcs[id.idx], elm.second.mask);
        break;
      }
    }
  }
}

struct DceOptResult {
  std::bitset<kMaxTrackedLocals> usedLocals;
  DceActionMap actionMap;
  DceReplaceMap replaceMap;
  bool didAddElemOpts{false};
};

DceOptResult
optimize_dce(const Index& index,
             const FuncAnalysis& fa,
             CollectedInfo& collect,
             BlockId bid,
             const State& stateIn,
             const DceOutState& dceOutState) {
  auto dceState = dce_visit(index, fa, collect, bid, stateIn, dceOutState);

  if (!dceState) {
    return {std::bitset<kMaxTrackedLocals>{}};
  }

  return {
    std::move(dceState->usedLocals),
    std::move(dceState->actionMap),
    std::move(dceState->replaceMap),
    dceState->didAddElemOpts
  };
}

//////////////////////////////////////////////////////////////////////

void remove_unused_locals(Context const ctx,
                          std::bitset<kMaxTrackedLocals> usedLocals) {
  if (!options.RemoveUnusedLocals) return;
  auto const func = ctx.func;

  /*
   * Removing unused locals in closures requires checking which ones
   * are captured variables so we can remove the relevant properties,
   * and then we'd have to mutate the CreateCl callsite, so we don't
   * bother for now.
   *
   * Note: many closure bodies have unused $this local, because of
   * some emitter quirk, so this might be worthwhile.
   */
  if (func->isClosureBody) return;

  // For reified functions, skip the first non-param local
  auto loc = func->locals.begin() + func->params.size() + (int)func->isReified;
  for (; loc != func->locals.end(); ++loc) {
    if (loc->killed) {
      assert(loc->id < kMaxTrackedLocals && !usedLocals.test(loc->id));
      continue;
    }
    if (loc->id < kMaxTrackedLocals && !usedLocals.test(loc->id)) {
      FTRACE(2, "  killing: {}\n", local_string(*func, loc->id));
      const_cast<php::Local&>(*loc).killed = true;
    }
  }
}

//////////////////////////////////////////////////////////////////////

}

void local_dce(const Index& index,
               const FuncAnalysis& ainfo,
               CollectedInfo& collect,
               BlockId bid,
               const State& stateIn) {
  // For local DCE, we have to assume all variables are in the
  // live-out set for the block.
  auto const ret = optimize_dce(index, ainfo, collect, bid, stateIn,
                                DceOutState{DceOutState::Local{}});

  dce_perform(*ainfo.ctx.func, ret.actionMap, ret.replaceMap);
}

//////////////////////////////////////////////////////////////////////

bool global_dce(const Index& index, const FuncAnalysis& ai) {
  auto rpoId = [&] (BlockId blk) {
    return ai.bdata[blk].rpoId;
  };

  auto collect = CollectedInfo {
    index, ai.ctx, nullptr,
    CollectionOpts{}, &ai
  };

  FTRACE(1, "|---- global DCE analyze ({})\n", show(ai.ctx));
  FTRACE(2, "{}", [&] {
    using namespace folly::gen;
    auto i = uint32_t{0};
    return from(ai.ctx.func->locals)
      | mapped(
        [&] (const php::Local& l) {
          return folly::sformat("  {} {}\n",
                                i++, local_string(*ai.ctx.func, l.id));
        })
      | unsplit<std::string>("");
  }());

  /*
   * States for each block, indexed by block id.
   */
  std::vector<DceOutState> blockStates(ai.ctx.func->blocks.size());

  /*
   * Set of block reverse post order ids that still need to be
   * visited.  This is ordered by std::greater, so we'll generally
   * visit blocks before their predecessors.  The algorithm does not
   * depend on this for correctness, but it will reduce iteration, and
   * the optimality of mergeDceStack depends on visiting at least one
   * successor prior to visiting any given block.
   *
   * Every block must be visited at least once, so we throw them all
   * in to start.
   */
  auto incompleteQ = dataflow_worklist<uint32_t,std::less<uint32_t>>(
    ai.rpoBlocks.size()
  );
  for (auto const bid : ai.rpoBlocks) incompleteQ.push(rpoId(bid));

  auto const nonThrowPreds   = computeNonThrowPreds(*ai.ctx.func, ai.rpoBlocks);
  auto const throwPreds      = computeThrowPreds(*ai.ctx.func, ai.rpoBlocks);

  /*
   * Suppose a stack slot isn't used, but it was pushed on two separate
   * paths, one of which could be killed, but the other can't. We have
   * to prevent either, so any time we find a non-used stack slot that
   * can't be killed, we add it to this set (via markUisLive, and the
   * DceAnalysis), and schedule its block for re-analysis.
   */
  LocationIdSet forcedLiveLocations;

  /*
   * Temporary set used to collect locations that were forced live by
   * block merging/linked locations etc.
   */
  LocationIdSet forcedLiveTemp;

  auto checkLive = [&] (std::vector<UseInfo>& uis, uint32_t i,
                        LocationId location) {
    if (forcedLiveLocations.count(location)) return true;
    if (!isLinked(uis[i])) return false;
    assert(i);
    return uis[i - 1].usage == Use::Used;
  };

  auto fixupUseInfo = [&] (std::vector<UseInfo>& uis,
                           BlockId blk,
                           bool isSlot) {
    for (uint32_t i = 0; i < uis.size(); i++) {
      auto& out = uis[i];
      if (out.location.blk == NoBlockId) out.location = { blk, i, isSlot };
      if (checkLive(uis, i, out.location)) {
        use(forcedLiveTemp, uis, i);
      }
    }
  };

  auto mergeUIs = [&] (std::vector<UseInfo>& outBase,
                       const std::vector<UseInfo>& inBase,
                       uint32_t i, BlockId blk, bool isSlot) {
    auto& out = outBase[i];
    auto& in = inBase[i];
    if (out.usage == Use::Used) {
      if (in.usage != Use::Used && nonThrowPreds[blk].size() > 1) {
        // This is to deal with the case where blk has multiple preds,
        // and one of those has multiple succs, one of which does use
        // this stack value.
        while (true) {
          auto& ui = inBase[i];
          auto linked = isLinked(ui);
          if (ui.usage != Use::Used) {
            forcedLiveTemp.insert({blk, i, isSlot});
          }
          if (!linked) break;
          assert(i);
          i--;
        }
      }
      return false;
    }

    if (in.usage == Use::Used || out.usage != in.usage) {
      use(forcedLiveTemp, outBase, i);
      return true;
    }
    auto location = in.location;
    if (location.blk == NoBlockId) location = { blk, i, isSlot };
    if (checkLive(outBase, i, location)) {
      use(forcedLiveTemp, outBase, i);
      return true;
    }

    auto ret = false;
    for (auto const& id : in.actions) {
      ret |= out.actions.insert(id).second;
    }
    assert(out.location.blk != NoBlockId);
    if (out.location < location) {
      // It doesn't matter which one we choose, but it should be
      // independent of the visiting order.
      out.location = location;
      ret = true;
    }
    return ret;
  };

  auto mergeUIVecs = [&] (folly::Optional<std::vector<UseInfo>>& stkOut,
                          const std::vector<UseInfo>& stkIn,
                          BlockId blk, bool isSlot) {
    if (!stkOut) {
      stkOut = stkIn;
      fixupUseInfo(*stkOut, blk, isSlot);
      return true;
    }

    auto ret = false;
    assert(stkOut->size() == stkIn.size());
    for (uint32_t i = 0; i < stkIn.size(); i++) {
      if (mergeUIs(*stkOut, stkIn, i, blk, isSlot)) ret = true;
    }
    return ret;
  };

  /*
   * If we weren't able to take advantage of any unused entries
   * on the dceStack that originated from another block, mark
   * them used to prevent other paths from trying to delete them.
   *
   * Also, merge the entries into our global forcedLiveLocations, to
   * make sure they always get marked Used.
   */
  auto processForcedLive = [&] (const LocationIdSet& forcedLive) {
    for (auto const& id : forcedLive) {
      if (forcedLiveLocations.insert(id).second) {
        // This is slightly inefficient; we know exactly how this will
        // affect the analysis, so we could get away with just
        // reprocessing the affected predecessors of id.blk; but this
        // is much simpler, and less error prone. We can revisit if it
        // turns out to be a performance issue.
        FTRACE(5, "Forcing {} live\n", show(id));
        incompleteQ.push(rpoId(id.blk));
      }
    }
  };

  /*
   * Iterate on live out states until we reach a fixed point.
   *
   * This algorithm treats the exceptional live-out states differently
   * from the live-out states during normal control flow.  The
   * liveOutExn sets only take part in the liveIn computation when the
   * block has exceptional exits.
   */
  while (!incompleteQ.empty()) {
    auto const bid = ai.rpoBlocks[incompleteQ.pop()];

    // skip unreachable blocks
    if (!ai.bdata[bid].stateIn.initialized) continue;

    FTRACE(2, "block #{}\n", bid);

    auto& blockState = blockStates[bid];
    auto const result = analyze_dce(
      index,
      ai,
      collect,
      bid,
      ai.bdata[bid].stateIn,
      blockState
    );

    FTRACE(2, "loc live out  : {}\n"
              "loc out exn   : {}\n"
              "loc live in   : {}\n",
              loc_bits_string(ai.ctx.func, blockState.locLive),
              loc_bits_string(ai.ctx.func, blockState.locLiveExn),
              loc_bits_string(ai.ctx.func, result.locLiveIn));

    processForcedLive(result.forcedLiveLocations);

    auto const isCFPushTaken = [] (const php::Block* blk, BlockId succId) {
      auto const& lastOpc = blk->hhbcs.back();
      switch (lastOpc.op) {
        case Op::MemoGet:
          return lastOpc.MemoGet.target1 == succId;
        case Op::MemoGetEager:
          return lastOpc.MemoGetEager.target1 == succId;
        default:
          return false;
      }
    };

    // Merge the liveIn into the liveOut of each normal predecessor.
    // If the set changes, reschedule that predecessor.
    for (auto const pid : nonThrowPreds[bid]) {
      FTRACE(2, "  -> {}\n", pid);
      auto& pbs = blockStates[pid];
      auto const oldPredLocLive = pbs.locLive;
      pbs.locLive |= result.locLiveIn;
      auto changed = pbs.locLive != oldPredLocLive;

      changed |= [&] {
        auto const pred = ai.ctx.func->blocks[pid].get();
        if (isCFPushTaken(pred, bid)) {
          auto stack = result.stack;
          stack.insert(stack.end(), pred->hhbcs.back().numPush(),
                       UseInfo{Use::Not});
          return mergeUIVecs(pbs.dceStack, stack, bid, false);
        } else {
          return mergeUIVecs(pbs.dceStack, result.stack, bid, false);
        }
      }();
      if (changed) {
        incompleteQ.push(rpoId(pid));
      }
    }

    // Merge the liveIn into the liveOutExn state for each throw predecessor.
    // The liveIn computation also depends on the liveOutExn state, so again
    // reschedule if it changes.
    for (auto const pid : throwPreds[bid]) {
      FTRACE(2, "  => {}\n", pid);
      auto& pbs = blockStates[pid];
      auto const oldPredLocLiveExn = pbs.locLiveExn;
      pbs.locLiveExn |= result.locLiveIn;
      if (pbs.locLiveExn != oldPredLocLiveExn) {
        incompleteQ.push(rpoId(pid));
      }
    }

    while (!forcedLiveTemp.empty()) {
      auto t = std::move(forcedLiveTemp);
      processForcedLive(t);
    }
  }

  /*
   * Now that we're at a fixed point, use the propagated states to
   * remove instructions that don't need to be there.
   */
  FTRACE(1, "|---- global DCE optimize ({})\n", show(ai.ctx));
  std::bitset<kMaxTrackedLocals> usedLocals;
  DceActionMap actionMap;
  DceReplaceMap replaceMap;
  bool didAddElemOpts = false;
  for (auto const bid : ai.rpoBlocks) {
    FTRACE(2, "block #{}\n", bid);
    auto ret = optimize_dce(
      index,
      ai,
      collect,
      bid,
      ai.bdata[bid].stateIn,
      blockStates[bid]
    );
    didAddElemOpts = didAddElemOpts || ret.didAddElemOpts;
    usedLocals |= ret.usedLocals;
    if (ret.actionMap.size()) {
      if (!actionMap.size()) {
        actionMap = std::move(ret.actionMap);
      } else {
        combineActions(actionMap, ret.actionMap);
      }
    }
    if (ret.replaceMap.size()) {
      if (!replaceMap.size()) {
        replaceMap = std::move(ret.replaceMap);
      } else {
        for (auto& elm : ret.replaceMap) {
          replaceMap.insert(std::move(elm));
        }
      }
    }
  }

  dce_perform(*ai.ctx.func, actionMap, replaceMap);

  FTRACE(1, "  used locals: {}\n", loc_bits_string(ai.ctx.func, usedLocals));
  remove_unused_locals(ai.ctx, usedLocals);

  return didAddElemOpts;
}

//////////////////////////////////////////////////////////////////////

}}
