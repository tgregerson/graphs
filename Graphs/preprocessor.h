#ifndef PREPROCESSOR_H_
#define PREPROCESSOR_H_

#include <random>
#include <utility>

#include "node.h"
#include "partitioner_config.h"

class Preprocessor {
 public:
  explicit Preprocessor(const PartitionerConfig& config);

  void ProcessGraph(Node* graph);

 private:
  // This strategy sets the default implementation to the one with the highest
  // weight in the universal resource specified by the PartitionerConfig.
  void RunUniversalResource(Node* graph);
  void RunProportionalToCapacity(Node* graph);
  void RunLowestCapacityCost(Node* graph);
  void RunRandom(Node* graph);

  // Gets the device resource capacity vector, adjusting its values as
  // necessary according to the device resource policy. 'remaining_node_ids'
  // are the IDs of all nodes from 'graph' that contribute to
  // 'effective_resource_capacities'.
  void GetEffectiveResourceCapacities(
      const Node* graph, std::vector<int>* effective_resource_capacities,
      std::vector<int>* remaining_node_ids);

  // Returns the index of the weight vector from 'node' that best matches
  // the use of resource 'res_id'. Returns -1 if no acceptable weight vector
  // found.
  int FindImplementationInResource(Node* node, int res_id);

  // Predicate function used for set ordering.
  static bool frac_comp(const std::pair<int, double>& a,
                        const std::pair<int, double>& b) {
    return a.second > b.second;
  }

  PartitionerConfig config_;
  std::default_random_engine random_engine_;
};

#endif /* PREPROCESSOR_H_ */
