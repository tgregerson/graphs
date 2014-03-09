#ifndef GAIN_BUCKET_MANAGER_MULTI_RESOURCE_EXCLUSIVE_H_
#define GAIN_BUCKET_MANAGER_MULTI_RESOURCE_EXCLUSIVE_H_

#include <cstdlib>
#include <map>
#include <random>
#include <vector>

#include "gain_bucket_entry.h"
#include "gain_bucket_manager.h"
#include "gain_bucket_standard.h"
#include "partitioner_config.h"

class GainBucketManagerMultiResourceExclusive : public GainBucketManager {
 public:
  GainBucketManagerMultiResourceExclusive(
      const std::vector<double>& max_imbalance_fraction,
      PartitionerConfig::GainBucketSelectionPolicy selection_policy,
      bool adaptive)
    : max_imbalance_fraction_(max_imbalance_fraction),
      selection_policy_(selection_policy), use_adaptive_(adaptive),
      num_nodes_(0) {
    random_engine_.seed(time(NULL));
    num_resources_per_node_ = max_imbalance_fraction_.size();   
    for (int i = 0; i < num_resources_per_node_; i++) {
      gain_buckets_a_.push_back(new GainBucketStandard());
      gain_buckets_b_.push_back(new GainBucketStandard());
    }
  }

  virtual ~GainBucketManagerMultiResourceExclusive() {
    for (int i = 0; i < num_resources_per_node_; i++) {
      delete gain_buckets_a_[i];
      delete gain_buckets_b_[i];
    }
  }

  // Selects the next node to move from the two gain buckets by returning the
  // entry with the highest gain that does not violate balance contraints.
  virtual GainBucketEntry GetNextGainBucketEntry(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);

  virtual int NumUnlockedNodes() const;

  virtual bool Empty() const;

  // Adds a node to the gain bucket(s).
  virtual void AddNode(int gain, Node* node, bool in_part_a,
                       const std::vector<int>& total_weight);

  virtual void UpdateGains(int gain_modifier,
                           const std::vector<int>& nodes_to_increase_gain, 
                           const std::vector<int>& nodes_to_decrease_gain, 
                           bool moved_from_part_a);

  virtual void UpdateNodeImplementation(Node* node);

  virtual void Print(bool condensed) const;

  virtual void set_selection_policy(
      PartitionerConfig::GainBucketSelectionPolicy selection_policy);
 
 private:
  virtual void AddEntry(const GainBucketEntry& entry, bool in_part_a);
  // It is safe to call this method even if 'node_id' is not present in any of
  // the buckets.
  virtual void RemoveNode(int node_id);

  // Returns true if an entry with 'node_id' is in one of the part_a buckets.
  virtual bool InPartA(int node_id);

  GainBucketEntry GetNextGainBucketEntryRandomResource(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry GetNextGainBucketEntryLargestImbalanceResource(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry GetNextGainBucketEntryLargestUnconstrainedGain(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry GetNextGainBucketEntryLargestGain(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry SelectBetweenBuckets(
      GainBucketInterface* constrained_bucket,
      GainBucketInterface* unconstrained_bucket,
      int resource_index,
      int max_constrained_node_weight);
  int num_resources_per_node_;
  std::vector<GainBucketInterface*> gain_buckets_a_;
  std::vector<GainBucketInterface*> gain_buckets_b_;
  std::vector<double> max_imbalance_fraction_;
  PartitionerConfig::GainBucketSelectionPolicy selection_policy_;
  std::unordered_multimap<int, int> node_id_to_resource_index_;
  bool use_adaptive_;
  int num_nodes_;
  std::default_random_engine random_engine_;
};

#endif // GAIN_BUCKET_MANAGER_MULTI_RESOURCE_EXCLUSIVE_H
