#include "partitioner_config.h"

#include "universal_macros.h"

using namespace std;

PartitionerConfig::PartitionerConfig()
  : preprocessor_policy(kNullPreprocessorPolicy),
    use_fixed_random_seed(false),
    fixed_random_seed(0),
    universal_resource_id(0),
    preprocessor_capacity_type(kNullCapacity),
    gain_bucket_type(kNullBucketType),
    gain_bucket_selection_policy(kNullSelectionPolicy),
    use_multilevel_constraint_relaxation(false),
    use_adaptive_node_implementations(false),
    use_ratio_in_imbalance_score(false),
    use_ratio_in_partition_quality(false),
    restrict_supernodes_to_default_implementation(false),
    supernode_implementations_cap(0),
    reuse_previous_run_implementations(false),
    mutation_rate(0),
    rebalance_on_start_of_pass(false),
    rebalance_on_end_of_run(false),
    rebalance_on_demand(false),
    rebalance_on_demand_cap_per_run(0),
    rebalance_on_demand_cap_per_pass(0),
    preprocessor_options_set(false),
    klfm_options_set(false),
    postprocessor_options_set(false) {}

void PartitionerConfig::PrintPreprocessorOptions(ostream& os) {
  os << "Preprocessor Options: " << endl;
  os << "Number of Resources: " << device_resource_capacities.size() << endl;
  os << "Resource Capacities: ";
  for (auto it : device_resource_capacities) {
    os << it << " ";
  }
  os << endl;
  os << "Preprocessor Policy: ";
  switch (preprocessor_policy) {
    case kAllUniversalResource:
      os << "Priority Resource";
    break;
    case kProportionalToCapacity:
      switch (preprocessor_capacity_type) {
        case kTotalCapacity:
          os << "Proportional to Total Capacity";
        break;
        case kFlexibleCapacity:
          os << "Proportional to Flexible Capacity";
        break;
        case kNullCapacity:
          assert_b(false) {
            printf("Configuration Error: A policy that requires capacity "
                    "type to be specified was selected, but the capacity "
                    "type was not set.\n");
          }
        break;
        default:
          assert_b(false) {
            printf("Configuration Error: A policy that requires capacity "
                    "type to be specified was selected, but the capacity "
                    "type was unrecognized.\n");
          }
      }
    break;
    case kLowestCapacityCost:
      switch (preprocessor_capacity_type) {
        case kTotalCapacity:
          os << "Lowest Total Capacity Cost";
        break;
        case kFlexibleCapacity:
          os << "Lowest Flexible Capacity Cost";
        break;
        case kNullCapacity:
          assert_b(false) {
            printf("Configuration Error: A policy that requires capacity "
                    "type to be specified was selected, but the capacity "
                    "type was not set.\n");
          }
        break;
        default:
          assert_b(false) {
            printf("Configuration Error: A policy that requires capacity "
                    "type to be specified was selected, but the capacity "
                    "type was unrecognized.\n");
          }
      }
    break;
    case kRandomPreprocessor:
      os << "Random";
    break;
    case kNullPreprocessorPolicy:
      assert_b(false) {
        printf("Some preprocessor-only options are set, but the no "
                "preprocessor policy was set. Terminating.\n");
      }
    break;
    default:
      assert_b(false) {
        printf("Unexpected error: PartitionerConfig thinks preprocessor "
                "options are set, but does not recognize preprocessor "
                "policy. Terminating\n");
      }
  }
  os << endl;
  os << "Use Fixed Seed for Random Operations: "
     << (use_fixed_random_seed ? "true" : "false") << endl;
  if (use_fixed_random_seed) {
    os << "Fixed Seed Value: " << fixed_random_seed << endl;
  }
  os << endl;
}

void PartitionerConfig::PrintKlfmOptions(ostream& os) {
  os << "KLFM Options: " << endl;
  os << "Number of Resources: " << device_resource_capacities.size() << endl;
  os << "Maximum Imbalance: ";
  for (auto it : device_resource_max_imbalances) {
    os << it << " ";
  }
  os << endl;
  os << "Target Ratio: ";
  for (auto it : device_resource_ratio_weights) {
    os << it << " ";
  }
  os << endl;
  os << "Use Ratio in Partition Quality: "
     << (use_ratio_in_partition_quality ? "true" : "false") << endl;
  os << "Use Ratio in Imbalance Score: "
     << (use_ratio_in_imbalance_score ? "true" : "false") << endl;
  os << "Enable Adaptive Node Implementations: "
     << (use_adaptive_node_implementations ? "true" : "false") << endl;
  os << "Use Multi-level Constraint Relaxation: "
     << (use_multilevel_constraint_relaxation ? "true" : "false") << endl;
  os << "Gain Bucket Type: ";
  switch (gain_bucket_type) {
    case kGainBucketSingleResource:
      os << "Single Resource";
    break;
    case kGainBucketMultiResourceExclusive:
    case kGainBucketMultiResourceExclusiveAdaptive:
      os << "Multi Resource Exclusive";
    break;
    case kGainBucketMultiResourceMixed:
    case kGainBucketMultiResourceMixedAdaptive:
      os << "Multi Resource Mixed";
    break;
    default:
      assert_b(false) {
        printf("\nUnrecognized gain bucket type.\n");
      }
  }
  os << endl;
  os << "Gain Bucket Selection Policy: ";
  if (gain_bucket_type == kGainBucketSingleResource) {
    os << "Single Resource - Default KLFM";
  } else {
    switch (gain_bucket_selection_policy) {
      case kGbmreSelectionPolicyRandomResource:
        os << "Random Resource";
      break;
      case kGbmreSelectionPolicyLargestResourceImbalance:
        os << "Largest Resource Imbalance";
      break;
      case kGbmreSelectionPolicyLargestUnconstrainedGain:
        os << "Largest Unconstrained Gain";
      break;
      case kGbmreSelectionPolicyLargestGain:
        os << "Largest Gain";
      break;
      case kGbmrmSelectionPolicyRandomResource:
        os << "Random Resource";
      break;
      case kGbmrmSelectionPolicyMostUnbalancedResource:
        os << "Most Unbalanced Resource";
      break;
      case kGbmrmSelectionPolicyBestGainImbalanceScoreClassic:
        os << "Best Gain Imbalance Score Classic";
      break;
      case kGbmrmSelectionPolicyBestGainImbalanceScoreWithAffinities:
        os << "Best Gain Imbalance Score With Affinities";
      break;
      default:
        assert_b(false) {
          printf("\nUnrecognized gain bucket selection policy.\n");
        }
    }
  }
  os << endl;
  os << "Restrict Supernodes to Default Implementation: "
     << (restrict_supernodes_to_default_implementation ? "true" : "false")
     << endl;
  os << "Supernode Implementations Cap: " << supernode_implementations_cap
     << endl;
  os << "Reuse Previous Run Implementations: "
     << (reuse_previous_run_implementations ? "true" : "false") << endl;
  os << "Enable Mutation: " << (!mutation_rate ? "true" : "false") << endl;
  if (!mutation_rate) {
    os << "Mutation Rate: " << mutation_rate << endl;
  }
  os << "Rebalance on Start of Pass: "
     << (rebalance_on_start_of_pass ? "true" : "false") << endl;
  os << "Rebalance on End of Run: "
     << (rebalance_on_end_of_run ? "true" : "false") << endl;
  os << "Rebalance on Demand: "
     << (rebalance_on_demand ? "true" : "false") << endl;
  if (rebalance_on_demand) {
    os << "Rebalance on Demand Cap per Run: " << rebalance_on_demand_cap_per_run
       << endl;
    os << "Rebalance on Demand Cap per Pass: "
       << rebalance_on_demand_cap_per_pass << endl;
  }
  os << endl;
}

void PartitionerConfig::ValidateOrDie(Node* graph) {
  assert_b(device_resource_capacities.size() ==
           device_resource_max_imbalances.size()) {
    printf("PartitionerConfig specifies a different number of resource "
           "capacities and max resource imbalances. Did you use the DTD to "
           "verify your configuration?\n");
  }
  if (graph) {
    assert(graph->internal_nodes().size() != 0);
    long unsigned int wv_size =
        graph->internal_nodes().begin()->second->SelectedWeightVector().size();
    assert_b(wv_size == device_resource_capacities.size()) {
      printf("Configuration Error: Config file specifies %lu device resources "
             "but graph node weights have %lu entries. These values must "
             "match.\n", device_resource_capacities.size(), wv_size);
    }
  }
  if (preprocessor_options_set) {
    ValidateOrDiePreprocessorOptions();
  }
  if (klfm_options_set) {
    ValidateOrDieKlfmOptions();
  }
}

void PartitionerConfig::ValidateOrDiePreprocessorOptions() {
    switch (preprocessor_policy) {
      case kAllUniversalResource:
        assert_b(universal_resource_id < device_resource_capacities.size() ||
                 universal_resource_id < 0) {
          printf("Configuration Error: The specified Universal Resource ID "
                 "(%d) is outside the valid range based on the number of "
                 "specifed device resources (0~%lu).\n",
                 universal_resource_id, (device_resource_capacities.size()-1));
        }
      break;
      case kProportionalToCapacity:
        switch (preprocessor_capacity_type) {
          case kTotalCapacity:
          break;
          case kFlexibleCapacity:
          break;
          case kNullCapacity:
            assert_b(false) {
              printf("Configuration Error: A policy that requires capacity "
                     "type to be specified was selected, but the capacity "
                     "type was not set.\n");
            }
          break;
          default:
            assert_b(false) {
              printf("Configuration Error: A policy that requires capacity "
                     "type to be specified was selected, but the capacity "
                     "type was unrecognized.\n");
            }
        }
      break;
      case kLowestCapacityCost:
        switch (preprocessor_capacity_type) {
          case kTotalCapacity:
          break;
          case kFlexibleCapacity:
          break;
          case kNullCapacity:
            assert_b(false) {
              printf("Configuration Error: A policy that requires capacity "
                     "type to be specified was selected, but the capacity "
                     "type was not set.\n");
            }
          break;
          default:
            assert_b(false) {
              printf("Configuration Error: A policy that requires capacity "
                     "type to be specified was selected, but the capacity "
                     "type was unrecognized.\n");
            }
        }
      break;
      case kRandomPreprocessor:
      break;
      case kNullPreprocessorPolicy:
        assert_b(false) {
          printf("Some preprocessor-only options are set, but the no "
                 "preprocessor policy was set. Terminating.\n");
        }
      break;
      default:
        assert_b(false) {
          printf("Unexpected error: PartitionerConfig thinks preprocessor "
                 "options are set, but does not recognize preprocessor "
                 "policy. Terminating\n");
        }
    }
}

void PartitionerConfig::ValidateOrDieKlfmOptions() {
  long unsigned int num_device_resources_specified =
      device_resource_capacities.size();
  if (use_multilevel_constraint_relaxation) {
    assert_b(use_adaptive_node_implementations) {
      printf("Use of multilevel constraint relaxation requires that "
             "adaptive node implementations are enabled\n");
    }
  }
  switch (gain_bucket_type) {
    case kGainBucketSingleResource:
      if (num_device_resources_specified != 1) {
        printf("WARNING: Configuration specifies a single-resource gain "
               "bucket, but the device resource specification lists "
               "%lu resources.\n", num_device_resources_specified);
      }
    break;
    case kGainBucketMultiResourceExclusive:
      assert(!use_adaptive_node_implementations);
    // Intentionally no break here.
    case kGainBucketMultiResourceExclusiveAdaptive:
      assert_b(num_device_resources_specified > 1) {
        printf("PartitionerConfig specifies a multi-resource gain bucket, "
               "but only lists %lu device resource(s).\n",
               num_device_resources_specified);
      }
      switch (gain_bucket_selection_policy) {
        case kGbmreSelectionPolicyRandomResource:
        break;
        case kGbmreSelectionPolicyLargestResourceImbalance:
        break;
        case kGbmreSelectionPolicyLargestUnconstrainedGain:
        break;
        case kGbmreSelectionPolicyLargestGain:
        break;
        case kNullSelectionPolicy:
          assert_b(false) {
            printf("Unexpected error: PartitionerConfig thinks KLFM "
                   "is using a multi-resource-exclusive gain bucket, but "
                   "selection policy was not set. Terminating\n");
          }
        break;
        default:
          assert_b(false) {
            printf("Unexpected error: PartitionerConfig thinks KLFM "
                   "is using a multi-resource-exclusive gain bucket, but "
                   "selection policy is unrecognized or incompatible."
                   "Terminating\n");
          }
      }
    break;
    case kGainBucketMultiResourceMixed:
      assert(!use_adaptive_node_implementations);
      // Intentionally no break here.
    case kGainBucketMultiResourceMixedAdaptive:
      switch (gain_bucket_selection_policy) {
        case kGbmrmSelectionPolicyRandomResource:
        break;
        case kGbmrmSelectionPolicyMostUnbalancedResource:
        break;
        case kGbmrmSelectionPolicyBestGainImbalanceScoreClassic:
        break;
        case kGbmrmSelectionPolicyBestGainImbalanceScoreWithAffinities:
        break;
        case kNullSelectionPolicy:
          assert_b(false) {
            printf("Unexpected error: PartitionerConfig thinks KLFM "
                   "is using a mixed-resource gain bucket, but "
                   "selection policy was not set. Terminating\n");
          }
        break;
        default:
          assert_b(false) {
            printf("Unexpected error: PartitionerConfig thinks KLFM "
                   "is using a mixed-resource gain bucket, but "
                   "selection policy is unrecognized or incompatible."
                   "Terminating\n");
          }
      }
    break;
    case kNullBucketType:
      assert_b(false) {
        printf("Unexpected error: PartitionerConfig thinks KLFM "
               "options are set, but the Gain Bucket type is uninitialized."
               " Terminating\n");
      }
    break;
    default:
      assert_b(false) {
        printf("Unexpected error: PartitionerConfig thinks KLFM "
               "options are set, but does not recognize Gain Bucket "
               "type. Terminating\n");
      }
  }
  assert(mutation_rate <= 100 && mutation_rate >= 0);
  if (rebalance_on_demand) {
    assert_b(!rebalance_on_demand || rebalance_on_demand_cap_per_run >=
             rebalance_on_demand_cap_per_pass) {
      printf("Configuration error: Cap on number of on-demand rebalances per "
             "pass (%d) is set higher than the number of rebalances per run "
             "(%d). This doesn't make sense.\n",
             rebalance_on_demand_cap_per_pass, rebalance_on_demand_cap_per_run);
    }
    assert_b(use_adaptive_node_implementations) {
      printf("Configuration error: On-demand rebalancing is enabled, but "
             "requires adaptive node implementations, which are disabled.");
    }
  } else {
    assert_b(!rebalance_on_demand_cap_per_run &&
             !rebalance_on_demand_cap_per_pass) {
      printf("Unexpected error: On-demand rebalance caps are set, but the "
             "feature is disabled.\n");
    }
  }
}
