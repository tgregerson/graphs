#ifndef EDGE_KLFM_H_
#define EDGE_KLFM_H_

/* This class takes Edge and adds additional members needed for the KLFM
   partitioning algorithm. */

#include "edge.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <unordered_set>
#include <map>
#include <vector>

class EdgeKlfm : public Edge {
 public:
  typedef std::vector<int> NodeIdVector;

  EdgeKlfm(int edge_id, const std::string& edge_name);
  explicit EdgeKlfm(Edge* edge);
  virtual ~EdgeKlfm() {
  }

  // Print debug information about the edge.
  virtual void Print() const;

  // Reset KLFM-specific data for a new iteration of the algorithm.
  // 'part_a_connections' and 'part_b_connections' are the IDs of the nodes
  // in initial partition A and initial partition B connected to the edge at
  // the start of the iteration. This method also sets the edge's critical
  // status based on these initial partition connections.
  template<typename T>
  void KlfmReset(const std::pair<T, T>& partitions);

  template<typename T>
  void PopulatePartitionNodeIds(const std::pair<T,T>& partitions);

  bool TouchesPartitionA() const {
    return !(part_a_locked_nodes.empty() &&
             part_a_unlocked_nodes.empty());
  }
  bool TouchesPartitionB() const {
    return !(part_b_locked_nodes.empty() &&
             part_b_unlocked_nodes.empty());
  }
  bool CrossesPartitions() const {
    return TouchesPartitionA() && TouchesPartitionB();
  }
  bool IsCritical() const {
    return is_critical;
  }

  // This method should be called once a node has been selected by the KLFM
  // algorithm for movement, and should be called for all edges that are
  // connected to that node, specified by 'node_id'.
  // This method will append the node IDs of the connected nodes that need to
  // have their gains increased/reduced based on changes to this edge to the
  // corresponding vectors.
  // NOTE: The same node ID may appear in both the increase gain and decrease
  // gain vectors and/or may appear multiple times in a vector. The node's gain
  // should be adjusted for each time it appears.
  // The method also updates the critical status of the edge.
  void MoveNode(int node_id,
                NodeIdVector* nodes_to_increase_gain,
                NodeIdVector* nodes_to_reduce_gain);

  // Returns the value this edge would contribute to the node's gain value.
  double GainContributionToNode(int node_id) const;

 private:
  void SetInitialCriticality();
  bool InGroup(const NodeIdVector& group, int node_id) const;
  NodeIdVector part_a_unlocked_nodes;
  NodeIdVector part_b_unlocked_nodes;
  NodeIdVector part_a_locked_nodes;
  NodeIdVector part_b_locked_nodes;

  // An edge is critical iff at least one partition has 0 locked nodes and
  // 0-2 unlocked nodes from the edge's connected nodes.
  bool is_critical{false};
  // An edge is locked non-critical iff both partitions have at least one
  // locked node from the edge's connected nodes.
  bool locked_noncritical{false};
};

template<typename T>
void EdgeKlfm::PopulatePartitionNodeIds(const std::pair<T,T>& partitions) {
  part_a_locked_nodes.clear();
  part_b_locked_nodes.clear();
  part_a_unlocked_nodes.clear();
  part_b_unlocked_nodes.clear();
  for (int node_id : connection_ids_) {
    if (partitions.first.find(node_id) != partitions.first.end()) {
      part_a_unlocked_nodes.push_back(node_id);
    } else {
      part_b_unlocked_nodes.push_back(node_id);
    }
  }
}

template<typename T>
void EdgeKlfm::KlfmReset(const std::pair<T,T>& partitions) {
  PopulatePartitionNodeIds(partitions);
  locked_noncritical = false;
  SetInitialCriticality();
}

#endif /* EDGE_KLFM_H_ */
