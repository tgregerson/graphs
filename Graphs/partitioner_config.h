#ifndef PARTITIONER_CONFIG_H_
#define PARTITIONER_CONFIG_H_

#include <iostream>
#include <vector>

#include "node.h"

struct PartitionerConfig {
 public:
  PartitionerConfig();

  // Ensures that the configuration data is consistent and valid or exits.
  // If top-level Node 'graph' is non-null, validates against the graph,
  // otherwise ignores graph-specific validation.
  void ValidateOrDie(Node* graph = NULL);

  void PrintPreprocessorOptions(std::ostream& os);
  void PrintKlfmOptions(std::ostream& os);

  // This must have the same number of entries as the weight vectors of the
  // nodes in the graph.
  std::vector<int> device_resource_capacities;
  std::vector<double> device_resource_max_imbalances;
  std::vector<int> device_resource_ratio_weights;

 // ---- Preprocessor config ---- //
  typedef enum {
    kNullPreprocessorPolicy, // Guard value.
    kAllUniversalResource,
    kProportionalToCapacity,
    kLowestCapacityCost,
    kRandomPreprocessor,
  } PreprocessorPolicy;

  typedef enum {
    kNullCapacity, // Guard value.
    // The total capacity of a resource for a target device.
    kTotalCapacity,
    // The capacity of a resource on the target device once the cost of nodes
    // with a single implementation has been subtracted off.
    kFlexibleCapacity,
  } ResourceCapacityType;

  PreprocessorPolicy preprocessor_policy;
  bool use_fixed_random_seed;
  int fixed_random_seed;

  // The ID corresponds to its position in the resource weight vector and its
  // order in the graph weight file. Starts at 0 for the first entry.
  int universal_resource_id;

  ResourceCapacityType preprocessor_capacity_type;

  // ---- KLFM config ---- //
  typedef enum {
    kNullBucketType, // Guard value.
    kGainBucketSingleResource,
    kGainBucketMultiResourceExclusive,
    kGainBucketMultiResourceExclusiveAdaptive,
    kGainBucketMultiResourceMixed,
    kGainBucketMultiResourceMixedAdaptive,
  } GainBucketType;

  typedef enum {
    kNullSelectionPolicy, // Guard value.
    kGbmreSelectionPolicyRandomResource,
    kGbmreSelectionPolicyLargestResourceImbalance,
    kGbmreSelectionPolicyLargestUnconstrainedGain,
    kGbmreSelectionPolicyLargestGain,
    kGbmrmSelectionPolicyRandomResource,
    kGbmrmSelectionPolicyMostUnbalancedResource,
    kGbmrmSelectionPolicyBestGainImbalanceScoreClassic,
    kGbmrmSelectionPolicyBestGainImbalanceScoreWithAffinities,
  } GainBucketSelectionPolicy;

  GainBucketType gain_bucket_type;
  GainBucketSelectionPolicy
      gain_bucket_selection_policy;
  bool use_multilevel_constraint_relaxation;
  bool use_adaptive_node_implementations;
  bool use_ratio_in_imbalance_score;
  bool use_ratio_in_partition_quality;
  bool restrict_supernodes_to_default_implementation;
  int supernode_implementations_cap;
  bool reuse_previous_run_implementations;
  int mutation_rate;
  bool rebalance_on_start_of_pass;
  bool rebalance_on_end_of_run;
  bool rebalance_on_demand;
  int rebalance_on_demand_cap_per_run;
  int rebalance_on_demand_cap_per_pass;

  bool preprocessor_options_set;
  bool klfm_options_set;
  bool postprocessor_options_set;
 private:
  void ValidateOrDiePreprocessorOptions();
  void ValidateOrDieKlfmOptions();
};

#endif /* PARTITIONER_CONFIG_H_ */
