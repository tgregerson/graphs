#ifndef GAIN_BUCKET_MANAGER_SINGLE_RESOURCE_H_
#define GAIN_BUCKET_MANAGER_SINGLE_RESOURCE_H_

#include <vector>

#include "gain_bucket_entry.h"
#include "gain_bucket_manager.h"
#include "gain_bucket_standard.h"

class GainBucketManagerSingleResource : public GainBucketManager {
 public:
  // 'resource_index' refers to which resource in a node's weight vector is the
  // single resource considered in this gain bucket manager.
  GainBucketManagerSingleResource(int resource_index, double max_imbalance_frac)
    : resource_index_(resource_index),
      max_imbalance_fraction_(max_imbalance_frac),
      gain_bucket_a_(new GainBucketStandard()),
      gain_bucket_b_(new GainBucketStandard()) {}

  virtual ~GainBucketManagerSingleResource() {
    delete gain_bucket_a_;
    delete gain_bucket_b_;
  }

  // Selects the next node to move from the two gain buckets by returning the
  // entry with the highest gain that does not violate balance contraints.
  virtual GainBucketEntry GetNextGainBucketEntry(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);

  virtual int NumUnlockedNodes() const;

  virtual bool Empty() const;

  // Adds a node to the gain bucket(s).
  virtual void AddNode(double gain, Node* node, bool in_part_a,
                       const std::vector<int>& total_weight);

  virtual void UpdateGains(double gain_modifier,
                           const std::vector<int>& nodes_to_increase_gain, 
                           const std::vector<int>& nodes_to_decrease_gain, 
                           bool moved_from_part_a);

  virtual void UpdateNodeImplementation(Node* node);

  virtual void Print(bool condensed) const;

  virtual void set_selection_policy(
      PartitionerConfig::GainBucketSelectionPolicy /* selection_policy */) {
    printf("Single-resource gain bucket manager does not currently support "
           "different selection policies.\n");
    exit(1);
  }

  virtual GainBucketEntry& GbeRefByNodeId(int node_id);

  virtual bool HasNode(int node_id);
 
 private:
  virtual void AddEntry(const GainBucketEntry& entry, bool in_part_a);

  size_t resource_index_;
  double max_imbalance_fraction_;
  GainBucketInterface* gain_bucket_a_;
  GainBucketInterface* gain_bucket_b_;
};

#endif // GAIN_BUCKET_MANAGER_SINGLE_RESOURCE_H
