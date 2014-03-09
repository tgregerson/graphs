#ifndef DIRECTED_NODE_H_
#define DIRECTED_NODE_H_

#include <cassert>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "edge.h"
#include "node.h"
#include "port.h"

class DirectedEdge;

class DirectedNode : public Node {
 public:
  DirectedNode() {}
  explicit DirectedNode(DirectedNode* src);
  DirectedNode(int node_id, std::string name = "");
  DirectedNode(int node_id, int weight, std::string name = "");
  virtual ~DirectedNode() {}

  // Adds a source port with its external connection id set to 'connected_id'.
  void AddSource(int connected_id);
  // Adds a sink port with its external connection id set to 'connected_id'.
  void AddSink(int connected_id);

  // Forms a set of internal nodes into a single supernode. Returns the ID of
  // the new supernode.
  virtual int ConsolidateSubNodes(
      const std::unordered_set<int>& component_nodes);

  // Locate the source port connected to edge with 'old_id' and change its
  // connection to 'new_id'.
  void SwapSource(int old_id, int new_id);
  void SwapSink(int old_id, int new_id);

  // Print debug information about the node. If 'recursive' is true and the
  // node is a supernode, also print each of the internal ports, edges, and
  // nodes.
  virtual void Print(bool recursive = false) const;

  // Transfers source/sink nodes/ports from 'from_edge' to 'to_edge',
  // excepting 'exclude_id'.
  void TransferEdgeSourcesExcluding(
      DirectedEdge* from_edge, DirectedEdge* to_edge, int exclude_id);
  void TransferEdgeSinksExcluding(
      DirectedEdge* from_edge, DirectedEdge* to_edge, int exclude_id);

 private:
  // Split edges that are not wholly internal to the supernode. Keep the
  // original id on the portion that is internal to the supernode and assign
  // a new id to the external part(s), update the nodes connected to the
  // external part, and connect the split edges to ports on the supernode.
  void SplitSupernodeBoundaryEdgesByType(
      Node* supernode, const std::unordered_set<int>& component_nodes,
      const std::unordered_set<int>& boundary_edges, bool source_edges);

  // Merge the edges that span the boundary of the specifed supernode. The
  // supernode-internal edge IDs will be kept and will replace the corresponding
  // edges external to the supernode. Removed edges are deleted and their IDs
  // free'd.
  virtual void MergeSupernodeBoundaryEdges(Node* supernode);

  void TransferEdgeConnectionsExcludingByType(
      DirectedEdge* from_edge, DirectedEdge* to_edge, int exclude_id,
      bool sources);

	Node::PortMap source_ports_;
	Node::PortMap sink_ports_;
};

#endif /* DIRECTED_NODE_H_ */
