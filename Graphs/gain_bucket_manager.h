#ifndef GAIN_BUCKET_MANAGER_H_
#define GAIN_BUCKET_MANAGER_H_

#include <vector>

#include "gain_bucket_entry.h"
#include "gain_bucket_interface.h"
#include "node.h"
#include "partitioner_config.h"
#include "universal_macros.h"

class GainBucketManager {
 public:
  virtual ~GainBucketManager() {};

  // Selects the next node to move from the two gain buckets by returning the
  // entry with the highest gain that does not violate balance contraints.
  virtual GainBucketEntry GetNextGainBucketEntry(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight) = 0;

  // Return the number of unlocked nodes in all buckets controlled by this
  // manager.
  virtual int NumUnlockedNodes() const = 0;

  // Returns true if all buckets controlled by this manager are empty.
  virtual bool Empty() const = 0;

  // Adds a node to the gain bucket(s).
  virtual void AddNode(int gain, Node* node, bool in_part_a,
                       const std::vector<int>& total_weight) = 0;

  // Updates the gains of the nodes in 'nodes_to_increase_gain' and
  // 'nodes_to_decrease_gain' by 'gain_modifier' depending on whether the
  // node that caused the gain update was 'moved_from_part_a'.
  virtual void UpdateGains(int gain_modifier,
                           const std::vector<int>& nodes_to_increase_gain, 
                           const std::vector<int>& nodes_to_decrease_gain, 
                           bool moved_from_part_a) = 0;

  // This method should be called if a node's implementation (selected weight
  // vector) changes externally to the gain bucket manager. The gain bucket
  // manager will update its internal buckets (if applicable) to account for the
  // change in selected weight vector. Does nothing if 'node' is not currently
  // in the gain buckets.
  virtual void UpdateNodeImplementation(Node* node) = 0;

  virtual void Print(bool condensed) const = 0;

  virtual void set_selection_policy(
      PartitionerConfig::GainBucketSelectionPolicy selection_policy) = 0;

 protected:
  virtual std::vector<int> GetMaxImbalance(
      const std::vector<double> frac, const std::vector<int> total_weight) {
    std::vector<int> imb;
    for (size_t i = 0; i < frac.size(); i++) {
      int res_imb = frac[i] * total_weight[i];
      imb.push_back(res_imb > 0 ? res_imb : 1);
    }
    return imb;
  }
};

#endif // GAIN_BUCKET_MANAGER_H
