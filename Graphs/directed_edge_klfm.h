#ifndef DIRECTED_EDGE_KLFM_H_
#define DIRECED_EDGE_KLFM_H_

/* This class takes Edge and adds additional members needed for the KLFM
   partitioning algorithm. */

#include "directed_edge.h"
#include "edge_klfm.h"

#include <unordered_set>
#include <vector>

class DirectedEdgeKlfm : public DirectedEdge, public EdgeKlfm {
 public:
  typedef std::unordered_set<int> NodeIdSet;
  typedef std::vector<int> NodeIdVector;

  explicit DirectedEdgeKlfm(DirectedEdge* edge);
  virtual ~DirectedEdgeKlfm() {}

  // Print debug information about the edge.
  virtual void Print();

  // Reset KLFM-specific data for a new iteration of the algorithm.
  // 'part_a_connections' and 'part_b_connections' are the IDs of the nodes
  // in initial partition A and initial partition B connected to the edge at
  // the start of the iteration. This method also sets the edge's critical
  // status based on these initial partition connections.
  void KlfmReset(const NodeIdSet& part_a_connections,
                 const NodeIdSet& part_b_connections);

  // This method should be called once a node has been selected by the KLFM
  // algorithm for movement, and should be called for all edges that are
  // connected to that node, specified by 'node_id'.
  // This method will append the node IDs of the connected nodes that need to
  // have their gains increased/reduced based on changes to this edge to the
  // corresponding vectors.
  // The method also updates the critical status of the edge.
  void MoveNode(int node_id, NodeIdVector* nodes_to_increase_gain,
                NodeIdVector* nodes_to_reduce_gain);

 private:
  void SetInitialCriticality();
};

#endif /* DIRECTED_EDGE_KLFM_H_ */
