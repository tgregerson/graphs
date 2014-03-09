#ifndef ID_MANAGER_H_
#define ID_MANAGER_H_

#include <cassert>
#include <limits>

/* Provides methods for obtaining unique IDs for nodes and edges. */
class IdManager {
 public:
  static int AcquireEdgeId() { 
    // Detect overflow in IDs.
    // TODO If this ever occurs, code a more robust system or convert
    // all IDs to larger datatype.
    assert(next_id != std::numeric_limits<int>::max() - 1);
    assert(next_id > prev_id);
    prev_id = next_id;
    return next_id++;
  }
  static int AcquireNodeId() {
    assert(next_id != std::numeric_limits<int>::max() - 1);
    assert(next_id > prev_id);
    prev_id = next_id;
    return next_id++;
  }
  static void ReleaseEdgeId(int /*id*/) { /* Currently does nothing. */ }
  static void ReleaseNodeId(int /*id*/) { /* Currently does nothing. */ }
  static void Reset(int val) { next_id = val; }

  static const int kReservedTerminalId;

 private:
  static int next_id;
  static int prev_id;
};

#endif /* ID_MANAGER_H_ */
