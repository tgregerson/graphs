#ifndef PARTITION_ENGINE_H_
#define PARTITION_ENGINE_H_

#include <cstdio>
#include <set>
#include <unordered_set>
#include <vector>

#include "edge_klfm.h"

class PartitionSummary;

// Abstract class for partitioning engines.
class PartitionEngine {
 public:
  virtual ~PartitionEngine() {}

  virtual void Execute(std::vector<PartitionSummary>* summaries) = 0;
  virtual void Reset() = 0;
};

class PartitionSummary {
 public:
  ~PartitionSummary() {}

  void Print() {
    for (size_t i = 0; i < partition_node_ids.size(); i++) {
      printf("Partition %lu Nodes:\n", i);
      printf("%ld total nodes\n", partition_node_ids[i].size());
      for (auto it : partition_node_ids[i]) {
        printf("%d ", it);
      }
      printf("\n\n");
    }
  }

  // Sets of the nodes in each partition. For bipartitioning, this should
  // be two sets.
  std::vector<EdgeKlfm::NodeIdSet> partition_node_ids;
  std::set<int> partition_edge_ids;
  std::set<std::string> partition_edge_names;
  double total_cost{0.0};
  double total_entropy{0.0};
  int total_span{0};
  double rms_resource_deviation{0.0};
  std::vector<double> balance;
  std::vector<int> total_weight;
  std::vector<double> total_resource_ratio;
  std::vector<std::vector<double>> partition_resource_ratios;
  int num_passes_used{0};
};

#endif /* PARTITION_ENGINE_H_ */
