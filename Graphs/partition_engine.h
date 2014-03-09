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
  PartitionSummary() {}
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
  int total_cost;
  double rms_resource_deviation;
  std::vector<double> balance;
  std::vector<int> total_weight;
  std::vector<double> total_resource_ratio;
  std::vector<std::vector<double>> partition_resource_ratios;
  int num_passes_used;
};

#endif /* PARTITION_ENGINE_H_ */
