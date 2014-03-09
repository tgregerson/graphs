#ifndef EDGE_KLFM_H_
#define EDGE_KLFM_H_

/* This class takes Edge and adds additional members needed for the KLFM
   partitioning algorithm. */

#include "edge.h"

#include <iostream>
#include <set>
#include <unordered_set>
#include <vector>

class EdgeKlfm : public Edge {
 public:
  typedef Edge::NodeIdSet NodeIdSet;
  //typedef std::unordered_set<int> NodeIdSet;
  typedef std::vector<int> NodeIdVector;

  EdgeKlfm(int edge_id, int weight, const std::string& edge_name);
  explicit EdgeKlfm(Edge* edge);
  virtual ~EdgeKlfm() {
    //std::cout << "Deleting Edge " << id << std::endl;
  }

  // Print debug information about the edge.
  virtual void Print() const;

  // Reset KLFM-specific data for a new iteration of the algorithm.
  // 'part_a_connections' and 'part_b_connections' are the IDs of the nodes
  // in initial partition A and initial partition B connected to the edge at
  // the start of the iteration. This method also sets the edge's critical
  // status based on these initial partition connections.
  void KlfmReset(const NodeIdVector& part_a_connections,
                 const NodeIdVector& part_b_connections);

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

  // An edge is critical iff at least one partition has 0 locked nodes and
  // 0-2 unlocked nodes from the edge's connected nodes.
  bool is_critical;
  // An edge is locked non-critical iff both partitions have at least one
  // locked node from the edge's connected nodes.
  bool locked_noncritical;
  // TODO Experiment with making this a vector instead to save memory.
  // Probably most edges will not have high enough fanout that there is really
  // even a performance reason to use set.
  /*
  NodeIdSet part_a_connected_unlocked_nodes;
  NodeIdSet part_b_connected_unlocked_nodes;
  NodeIdSet part_a_connected_locked_nodes;
  NodeIdSet part_b_connected_locked_nodes;
  */
  NodeIdVector part_a_connected_unlocked_nodes;
  NodeIdVector part_b_connected_unlocked_nodes;
  NodeIdVector part_a_connected_locked_nodes;
  NodeIdVector part_b_connected_locked_nodes;

 private:
  void SetInitialCriticality();
};

#endif /* EDGE_KLFM_H_ */
