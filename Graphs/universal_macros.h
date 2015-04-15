#ifndef UNIVERSAL_MACROS_H_
#define UNIVERSAL_MACROS_H_

#include <cassert>

#include <iostream>

// Performance-related options

/* This determines the number of entries from a constrained gain bucket
   that are checked for each KLFM move.

   Increasing the value will always increase algorithm complexity, but may
   lead to better partitions when using non-unitary node weights. The value
   must always be at least 1, and will show no difference when increased
   beyond the number of nodes in the graph (more typically, 50% of the nodes).

   It does not impact the solution quality for unit weight graphs, so should
   be left at 1 for them. */
#ifndef MAX_CONSTRAINED_ENTRY_CHECKS
  #define MAX_CONSTRAINED_ENTRY_CHECKS 1
#endif

#ifndef REBALANCE_PASSES
  #define REBALANCE_PASSES 5
#endif

// Basic guideline for setting DEBUG_LEVEL when logging using DLOG and
// RUN_DEBUG(DEBUG_OPT_TRACE) or VERBOSITY when logging using VLOG and
// RUN_VERBOSE:
// 0: Happens ~once per program execution
// 1: Happens ~once per major algorithm run
// 2: Happens ~once per major algorithm pass
// 3: Happens O(n) times per major algorithm pass

#ifndef VERBOSITY
#define VERBOSITY 1
#endif

// Set to non-zero to enable debugging mode.
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 0
#endif

// Controls verbosity of debug messages. 0 is lowest verbosity.
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1
#endif

// The following are debug options. Setting a value to '0' will disable
// debug commands related to that option.
#ifndef DEBUG_OPT_GENERAL 
#define DEBUG_OPT_GENERAL 1
#endif

#ifndef DEBUG_OPT_TRACE 
#define DEBUG_OPT_TRACE 1
#endif

#ifndef DEBUG_OPT_BALANCE_CHECK 
#define DEBUG_OPT_BALANCE_CHECK 0
#endif

#ifndef DEBUG_OPT_COST_CHECK 
#define DEBUG_OPT_COST_CHECK 0
#endif

#ifndef DEBUG_OPT_TOTAL_WEIGHT_CHECK 
#define DEBUG_OPT_TOTAL_WEIGHT_CHECK 0
#endif

#ifndef DEBUG_OPT_PARTITION_IMBALANCE_EXCEEDED 
#define DEBUG_OPT_PARTITION_IMBALANCE_EXCEEDED 0
#endif

#ifndef DEBUG_OPT_REBALANCE 
#define DEBUG_OPT_REBALANCE 0
#endif

#ifndef DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR 
#define DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR 0
#endif

#ifndef DEBUG_OPT_BUCKET_NODE_SELECT 
#define DEBUG_OPT_BUCKET_NODE_SELECT 0
#endif

#ifndef DEBUG_OPT_BUCKET_NODE_SELECT_RANDOM_RESOURCE 
#define DEBUG_OPT_BUCKET_NODE_SELECT_RANDOM_RESOURCE 0
#endif

#ifndef DEBUG_OPT_BUCKET_NODE_SELECT_GAIN_IMBALANCE_WITH_AFFINITIES 
#define DEBUG_OPT_BUCKET_NODE_SELECT_GAIN_IMBALANCE_WITH_AFFINITIES 0
#endif

#ifndef PROFILE_ENABLED
#define PROFILE_ENABLED 0
#endif

#ifndef PROFILE_ITERATIONS
  #define PROFILE_ITERATIONS 5000
#endif

#ifndef assert_b
  #define assert_b(cond) for ( ; !(cond) ; assert(cond) )
#endif
#ifndef assert_str
  #define assert_str(cond, str) if (!cond) {std::cout << str << std::endl; assert(cond);}
#endif
#ifndef RUN_DEBUG
  #define RUN_DEBUG(debug_option, level) if (DEBUG_ENABLED && debug_option && (DEBUG_LEVEL >= level))
#endif
#ifndef RUN_VERBOSE
  #define RUN_VERBOSE(lev) if (VERBOSITY >= lev)
#endif
#ifndef DLOG
  #define DLOG(debug_option, level) if (DEBUG_ENABLED && debug_option && (DEBUG_LEVEL >= level)) std::cout
#endif
#ifndef VLOG
  #define VLOG(lev) if (VERBOSITY >= lev) std::cout
#endif

#ifndef CNN_INLINE
#define CNN_INLINE
template<typename T> inline T* CHECK_NOTNULL(T* ptr) { assert(ptr != nullptr); return ptr; }
#endif

#endif /* UNIVERSAL_MACROS_H_ */
