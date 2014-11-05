#ifndef GAIN_BUCKET_MANAGER_MULTI_RESOURCE_MIXED_H_
#define GAIN_BUCKET_MANAGER_MULTI_RESOURCE_MIXED_H_

#include <cstdlib>
#include <map>
#include <random>
#include <vector>

#include "gain_bucket_entry.h"
#include "gain_bucket_manager.h"
#include "gain_bucket_standard.h"
#include "partitioner_config.h"

class GainBucketManagerMultiResourceMixed : public GainBucketManager {
 public:
  GainBucketManagerMultiResourceMixed(
      const std::vector<double>& max_imbalance_fraction,
      PartitionerConfig::GainBucketSelectionPolicy selection_policy,
      bool adaptive, bool use_ratio, const std::vector<int>& resource_ratios)
    : max_imbalance_fraction_(max_imbalance_fraction),
      selection_policy_(selection_policy), use_adaptive_(adaptive),
      use_ratio_(use_ratio), resource_ratio_weights_(resource_ratios) {
    // TODO Temporarily disable ratio in individual move selection -
    // limit it to rebalance operations.
    //use_ratio_ = false;
    random_engine_.seed(0);
    num_resources_per_node_ = max_imbalance_fraction_.size();   
    // TODO - Make this a parameter of the constructor and incorporate into
    // the partitioner configuration.
    max_bucket_search_depth_ = 3;
    for (size_t i = 0; i < num_resources_per_node_; i++) {
      gain_buckets_a_.push_back(new GainBucketStandard());
      gain_buckets_b_.push_back(new GainBucketStandard());
    }
    gain_bucket_a_master_ = new GainBucketStandard();
    gain_bucket_b_master_ = new GainBucketStandard();
  }

  virtual ~GainBucketManagerMultiResourceMixed() {
    for (size_t i = 0; i < num_resources_per_node_; i++) {
      delete gain_buckets_a_[i];
      delete gain_buckets_b_[i];
    }
    delete gain_bucket_a_master_;
    delete gain_bucket_b_master_;
  }

  // Selects the next node to move from the two gain buckets by returning the
  // entry with the highest gain that does not violate balance contraints.
  virtual GainBucketEntry GetNextGainBucketEntry(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);

  // Adds a node to the gain bucket(s).
  virtual void AddNode(double gain, Node* node, bool in_part_a,
                       const std::vector<int>& total_weight);

  virtual int NumUnlockedNodes() const;

  virtual bool Empty() const;

  virtual void UpdateGains(double gain_modifier,
                           const std::vector<int>& nodes_to_increase_gain, 
                           const std::vector<int>& nodes_to_decrease_gain, 
                           bool moved_from_part_a);

  virtual void UpdateNodeImplementation(Node* node);

  virtual void Print(bool condensed) const;

  virtual void set_selection_policy(
      PartitionerConfig::GainBucketSelectionPolicy selection_policy);
 
 private:
  virtual void AddEntry(
      const GainBucketEntry& entry, int associated_resource, bool in_part_a);
  // It is safe to call this method even if 'node_id' is not present in any of
  // the buckets.
  virtual void RemoveNode(int node_id);
  int DetermineResourceAffinity(const std::vector<int>& weight_vector,
                                const std::vector<int>& total_weight);
  // Violater version returns 0.0 if no resource maxes are exceeded. Otherwise,
  // returns the same value as ImbalancePower.
  double ViolatorImbalancePower(
      const std::vector<int>& balance, const std::vector<int>& total_weight);
  // Setting 'use_violater' uses the Violater version of ImbalancePower.
  double ImbalancePowerIfMoved(const std::vector<int>& node_weight,
      const std::vector<int>& balance, const std::vector<int>& total_weight,
      bool from_part_a, bool use_violator);

  // RatioPower is based on implementation selection rather than which partition
  // the node is in. Therefore, there is no purpose to use these methods if
  // adaptive node implementations are disabled.
  double RatioPowerIfChangedByEntry(const GainBucketEntry& candidate_entry,
                                    const std::vector<int>& total_weight);

  // Changes 'entry' to have the weight vector that produces the lowest
  // ImbalancePower as its current_weight_vector. Returns the lowest
  // ImbalancePower.
  double SetBestWeightVectorByImbalancePower(GainBucketEntry* entry,
      const std::vector<int>& balance, const std::vector<int>& total_weight,
      bool from_part_a, bool use_violator);

  GainBucketEntry GetNextGainBucketEntryRandomResource(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry GetNextGainBucketEntryMostUnbalancedResource(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry GetNextGainBucketEntryBestGainImbalanceScoreClassic(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry GetNextGainBucketEntryBestGainImbalanceScoreWithAffinities(
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight);
  GainBucketEntry SelectBetweenBucketsByImbalancePower(
      GainBucketInterface* bucket_a,
      GainBucketInterface* bucket_b,
      const std::vector<int>& current_balance,
      const std::vector<int>& total_weight,
      int search_depth);
  GainBucketEntry SelectBetweenBucketsByGainImbalancePowerRatio(
      GainBucketInterface* bucket_a,
      GainBucketInterface* bucket_b,
      const std::vector<int>& current_balance,
      int search_depth);
  // Lower is better.
  double ComputeGainImbalanceFn(double gain, double imbalance_power);

  size_t num_resources_per_node_;
  size_t max_bucket_search_depth_;
  // This vector has the same number of entries as there are resources types.
  // Each bucket represents a resource type and may include entries that are
  // weighted toward that resource.
  std::vector<GainBucketInterface*> gain_buckets_a_;
  // The master bucket contains entries for all of the nodes in gain_buckets_a_.
  // It is used to sort entries by gain regardless of their resource affinity.
  GainBucketInterface* gain_bucket_a_master_;
  std::vector<GainBucketInterface*> gain_buckets_b_;
  GainBucketInterface* gain_bucket_b_master_;
  std::vector<double> max_imbalance_fraction_;
  PartitionerConfig::GainBucketSelectionPolicy selection_policy_;
  bool use_adaptive_;
  bool use_ratio_;
  std::vector<int> resource_ratio_weights_;
  std::unordered_multimap<int, int> node_id_to_resource_index_;
  std::default_random_engine random_engine_;
};

#endif // GAIN_BUCKET_MANAGER_MULTI_RESOURCE_MIXED_H
