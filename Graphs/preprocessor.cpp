#include "preprocessor.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <utility>

#include "universal_macros.h"

using namespace std;

Preprocessor::Preprocessor(const PartitionerConfig& config)
  : config_(config) {
  if (config.use_fixed_random_seed) {
    random_engine_.seed(config.fixed_random_seed);
  } else {
    random_engine_.seed(time(NULL));
  }
}

void Preprocessor::ProcessGraph(Node* graph) {
  switch (config_.preprocessor_policy) {
    case PartitionerConfig::kAllUniversalResource:
      RunUniversalResource(graph);
      break;
    case PartitionerConfig::kProportionalToCapacity:
      RunProportionalToCapacity(graph);
      break;
    case PartitionerConfig::kLowestCapacityCost:
      RunLowestCapacityCost(graph);
      break;
    case PartitionerConfig::kRandomPreprocessor:
      RunRandom(graph);
      break;
    default:
      printf("Unrecognized Preprocessor Policy\n");
      assert(false);
  }
}

void Preprocessor::RunUniversalResource(Node* graph) {
  bool print_missing_implementation_warning = false;
  for (auto node_pair : graph->internal_nodes()) {
    Node* node = node_pair.second;
    auto& wvs = node->WeightVectors();
    int highest_index = 0;
    int highest_value = 0;
    for (size_t i = 0; i < wvs.size(); i++) {
      if (wvs[i][config_.universal_resource_id] > highest_value) {
        highest_index = i;
        highest_value = wvs[i][config_.universal_resource_id];
      }
    }
    if (highest_value == 0) {
      for (int weight : node->WeightVector(highest_index)) {
        if (weight > 0) {
          print_missing_implementation_warning = true;
        }
      }
    }
    node->SetSelectedWeightVector(highest_index);
  }
  if (print_missing_implementation_warning) {
    printf("Warning: Universal Resource preprocessing mode set, but ");
    printf("encountered a node without an implementation using the ");
    printf("universal resource.\n");
  }
}

void Preprocessor::RunProportionalToCapacity(Node* graph) {
  vector<int> effective_resource_capacities;
  vector<int> remaining_node_ids;
  GetEffectiveResourceCapacities(graph, &effective_resource_capacities,
                                 &remaining_node_ids);
  if (remaining_node_ids.empty()) {
    printf("\nWarning: Running proportional-to-capacity weight preprocessing ");
    printf("but graph only has single-implementation nodes.\n");
    return;
  }

  // To make this an O(n) algorithm, we will process each node's weights
  // individually. This means that the order of processing nodes will affect
  // the outcome.
  // Option 1: Randomize order - allows more exploration of the possible
  // solutions if the algorithm is run multiple times, however results is
  // not repeatable.
  // Option 2: Order by resource cost - Process nodes in order of highest
  // to lowest cost.
  // Option 3: Order by number of implementations - Process nodes with the
  // fewest implementations first.

  // Ordered by number of implementations.
  multimap<int,int> num_wvs_to_node_id;
  for (auto node_id : remaining_node_ids) {
    Node* node = graph->internal_nodes().at(node_id);
    const vector<vector<int>>& wvs = node->WeightVectors();
    num_wvs_to_node_id.insert(make_pair(wvs.size(), node_id));
  }
  remaining_node_ids.clear();
  for (auto it : num_wvs_to_node_id) {
    remaining_node_ids.push_back(it.second);
  }

  // Random
  // TODO - Although randomness makes sense for the algorithm in actual use, for
  // benchmarking purposes it is better to ensure it is deterministic.
  //shuffle(remaining_node_ids.begin(), remaining_node_ids.end(), random_engine_);
  vector<int> committed_flexible_weights;
  committed_flexible_weights.insert(committed_flexible_weights.begin(),
                                    effective_resource_capacities.size(), 0);
  RUN_VERBOSE(1) {
    printf("Effective resource capacity for preprocessing: ");
    for (auto res : effective_resource_capacities) {
      printf("%d ", res);
    }
    printf("\n");
  }
  RUN_VERBOSE(1) {
    for (size_t res_id = 0; res_id < effective_resource_capacities.size();
         res_id++) {
      if (effective_resource_capacities[res_id] <= 0) {
        printf("\nWARNING: Resource %lu has non-positive effective capacity %d. "
               "It will be excluded from consideration in preprocessing.\n",
               res_id, effective_resource_capacities[res_id]);
      }
    }
  }

  // TODO: A superior way of doing the calculation would be to calculate the
  // resource balances that would result from each implementation choice, then
  // choose the one that results in the minimum variance or std dev from
  // the average utilization.

  // Each entry consists of an int representing the resource ID (index) and
  // a double representing fraction of that resource's effective capacity that
  // is currently free.
  vector<pair<int, double>> resource_free_capacity_frac;
  vector<int> resources_in_play;
  for (size_t res_id = 0; res_id < effective_resource_capacities.size();
       res_id++) {
    if (effective_resource_capacities[res_id] > 0) {
      // NOTE: If the effective resource capacity is not > 0, the resource
      // will not be used.
      resources_in_play.push_back(res_id);
      resource_free_capacity_frac.push_back(make_pair(res_id, 1.0));
    }
  }
  if (resources_in_play.empty()) {
    printf("\nError: Attempting to use a proportional-to-capacity preprocessing"
           " strategy, however no resources have positive effective capacity. "
           "Preprocessor will terminate.\n");
    return;
  }

  for (auto node_id : remaining_node_ids) {
    Node* node = graph->internal_nodes()[node_id];

    int wt_idx = -1;
    for (auto it = resource_free_capacity_frac.begin();
         it != resource_free_capacity_frac.end(); it++) {
      wt_idx = FindImplementationInResource(node, it->first);
      if (wt_idx >= 0) {
        node->SetSelectedWeightVector(wt_idx);
        vector<int> selected_vector = node->SelectedWeightVector();

        // Careful - This invalidates 'it'. Must set 'it' or break loop.
        resource_free_capacity_frac.clear();

        // Update all the free capacities.
        for (size_t i = 0; i < selected_vector.size(); i++) {
          committed_flexible_weights[i] += selected_vector[i];
        }
        for (auto res_id : resources_in_play) {
          double erc_d = (double)effective_resource_capacities[res_id];
          double cfw_d = (double)committed_flexible_weights[res_id];
          assert(erc_d > 0);
          double res_frac_free = (erc_d - cfw_d) / erc_d;
          resource_free_capacity_frac.push_back(
              make_pair(res_id, res_frac_free));
        }
        sort(resource_free_capacity_frac.begin(),
             resource_free_capacity_frac.end(), &frac_comp);
        break;
      }
    }
    // This supports nodes that have no weight, which we use to emulate
    // buffers.
    if (wt_idx < 0) {
      node->SetSelectedWeightVector(0);
    }
  }
}

void Preprocessor::RunLowestCapacityCost(Node* graph) {
  vector<int> effective_resource_capacities;
  vector<int> remaining_node_ids;
  GetEffectiveResourceCapacities(graph, &effective_resource_capacities,
                                 &remaining_node_ids);
  if (remaining_node_ids.empty()) {
    printf("\nWarning: Running proportional-to-capacity weight preprocessing ");
    printf("but graph only has single-implementation nodes.\n");
    return;
  }

  RUN_VERBOSE(1) {
    printf("Effective resource capacity for preprocessing: ");
    for (auto res : effective_resource_capacities) {
      printf("%d ", res);
    }
    printf("\n");
  }
  RUN_VERBOSE(1) {
    for (size_t res_id = 0; res_id < effective_resource_capacities.size();
         res_id++) {
      if (effective_resource_capacities[res_id] <= 0) {
        printf("\nWARNING: Resource %lu has non-positive effective capacity %d."
               " It will be excluded from consideration in preprocessing.\n",
               res_id, effective_resource_capacities[res_id]);
      }
    }
  }

  vector<int> resources_in_play;
  for (size_t res_id = 0; res_id < effective_resource_capacities.size();
       res_id++) {
    if (effective_resource_capacities[res_id] > 0) {
      // NOTE: If the effective resource capacity is not > 0, the resource
      // will not be used.
      resources_in_play.push_back(res_id);
    }
  }
  if (resources_in_play.empty()) {
    printf("\nError: Attempting to use a lowest-capacity-cost preprocessing"
           " strategy, however no resources have positive effective capacity. "
           "Preprocessor will terminate.\n");
    return;
  }

  for (auto node_id : remaining_node_ids) {
    Node* node = graph->internal_nodes()[node_id];
    vector<pair<int, double>> res_costs;

    for (auto it : resources_in_play) {
      int wt_idx = FindImplementationInResource(node, it);
      if (wt_idx >= 0) {
        double cost = double(node->WeightVector(wt_idx)[it]) /
                      double(effective_resource_capacities[it]);
        res_costs.push_back(make_pair(it, cost));
      }
    }
    assert(!res_costs.empty());
  }
}

void Preprocessor::RunRandom(Node* graph) {
  for (auto node_pair : graph->internal_nodes()) {
    Node* node = node_pair.second;
    int num_vectors = node->WeightVectors().size();
    int rand_index = random_engine_() % num_vectors;
    node->SetSelectedWeightVector(rand_index);
  }
}

void Preprocessor::GetEffectiveResourceCapacities(
    const Node* graph, vector<int>* effective_resource_capacities,
    vector<int>* remaining_node_ids) {
  int num_capacities = config_.device_resource_capacities.size();
  int num_weight_resources =
      graph->internal_nodes().begin()->second->SelectedWeightVector().size();
  if (num_capacities != num_weight_resources) {
    printf("Error: Configuration specifies %d resource capacities.",
           num_capacities);
    printf("Node weights specify %d resources.", num_weight_resources);
    printf("These values must match. Terminating.\n");
    assert(false);
  }
  *effective_resource_capacities = config_.device_resource_capacities;
  switch (config_.preprocessor_capacity_type) {
    case PartitionerConfig::kTotalCapacity:
      // Do not need to adjust capacities.
      for (auto node_pair : graph->internal_nodes()) {
        remaining_node_ids->push_back(node_pair.first);
      }
      break;
    case PartitionerConfig::kFlexibleCapacity:
      // Subtract the weights of any nodes that have only one possible
      // implementation. This may result in negative capacities.
      for (auto node_pair : graph->internal_nodes()) {
        Node* node = node_pair.second;
        const vector<vector<int>>& wvs = node->WeightVectors();
        if (wvs.size() == 1) {
          for (size_t i = 0; i < wvs[0].size(); i++) {
            effective_resource_capacities->at(i) -= wvs[0][i];
          }
        } else {
          remaining_node_ids->push_back(node_pair.first);
        }
      }
      break;
    default:
      printf("Unrecognized Preprocessor Capacity Type\n");
      assert(false);
  }
}

int Preprocessor::FindImplementationInResource(Node* node, int res_id) {
  // TODO: This only works well if each implementation has only one resource
  // type. It could be made more sophisticated to support multiple types per
  // implementation.
  for (size_t wv_id = 0; wv_id < node->WeightVectors().size(); ++wv_id) {
    if (node->WeightVector(wv_id)[res_id] != 0) {
      return wv_id;
    }
  }
  return -1;
}
