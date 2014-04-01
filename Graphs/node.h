#ifndef NODE_H_
#define NODE_H_

#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "edge.h"
#include "port.h"

class Node {
 public:
  typedef std::set<int> NodeIdSet;
  typedef std::set<int> EdgeIdSet;
  typedef std::map<int, Node*> NodeMap;
  typedef std::map<int, Edge*> EdgeMap;
  typedef std::map<int, Port> PortMap;

  explicit Node(Node* src);
  Node(int node_id, const std::string& node_name = "");
  Node(int node_id, int weight, const std::string& node_name = "");
  virtual ~Node() {
    for (auto it : internal_nodes_) {
      delete it.second;
    }
    for (auto it : internal_edges_) {
      delete it.second;
    }
  }

  // Adds a port with its external connection id set to 'connected_id'.
  void AddConnection(int connected_id);

  // Removes the port(s) connected with external connection id set to
  // 'connected_id'. Returns the number of ports removed.
  int RemoveConnection(int connected_id);

  void AddWeightVector(const std::vector<int>& wv);

  // Copies all data from 'src'. NOTE: This includes the ID. Care must be
  // taken not to have separate objects with duplicate IDs in the same
  // container, as the assumption is that IDs are unique.
  virtual void CopyFrom(Node* src);

  // Locate the port connected to edge with 'old_id' and change its
  // connection to 'new_id'.
  void SwapPortConnection(int old_id, int new_id);

  // Print debug information about the node. If 'recursive' is true and the
  // node is a supernode, also print each of the internal ports, edges, and
  // nodes.
  void Print(bool recursive = false) const;
  void PrintPorts() const;
  void PrintInternalNodes(bool recursive) const;
  void PrintInternalEdges() const;
  void PrintInternalNodeSelectedWeights() const;

  void AddInternalNode(int id, Node* node) {
    assert(internal_nodes_.count(id) == 0);
    internal_nodes_.insert(std::make_pair(id, node));
  }
  void AddInternalEdge(int id, Edge* edge) {
    assert(internal_edges_.count(id) == 0);
    internal_edges_.insert(std::make_pair(id, edge));
  }
  void AddPort(int id, const Port& port) {
    assert(ports_.find(id) == ports_.end());
    ports_.insert(std::make_pair(id, port));
  }
  // Returns NULL if node with 'id' is not an internal edge.
  Node* GetInternalNode(int id) {
    if (internal_nodes_.count(id) == 0) {
      return nullptr;
    } else {
      return internal_nodes_[id];
    }
  }
  Edge* GetInternalEdge(int id) {
    if (internal_edges_.count(id) == 0) {
      return nullptr;
    } else {
      return internal_edges_[id];
    }
  }

  // Transfers source/sink nodes/ports from 'from_edge' to 'to_edge',
  // excepting 'exclude_id'.
  void TransferEdgeConnectionsExcluding(Edge* from_edge, Edge* to_edge,
                                        int exclude_id);

  // Indicates that the node is a composite of other nodes.
  bool is_supernode() const;

  // Gets the weight from the selected weight vector corresponding to the index.
  int get_simple_weight(int index) {
    assert(!SelectedWeightVector().empty());
    return SelectedWeightVector()[index];
  }

  void SetWeightVectorToMinimizeImbalance(
      std::vector<int>& balance, const std::vector<int>& max_weight_imbalance,
      bool is_positive, bool use_imbalance, bool use_ratio,
      const std::vector<int>& res_ratios, const std::vector<int>& total_weight);

  std::vector<int> SelectedWeightVector() const {
    if (weight_vectors_.empty() && is_supernode()) {
      return TotalInternalSelectedWeight(NULL);
    } else {
      assert(selected_weight_vector_index_ < weight_vectors_.size());
      return weight_vectors_[selected_weight_vector_index_];
    }
  }

  int selected_weight_vector_index() const {
    return selected_weight_vector_index_;
  }

  const std::vector<int>& WeightVector(int index) const {
    assert(index < weight_vectors_.size());
    return weight_vectors_[index];
  }

  const std::vector<std::vector<int>>& WeightVectors() const {
    return weight_vectors_;
  }

  int num_resources() const {
    return SelectedWeightVector().size();
  }

  int num_personalities() const {
    return weight_vectors_.size();
  }

  // 'SetSelectedWeightVector' only changes the weight vector. The previous
  // weight vector index is unchanged.
  void SetSelectedWeightVector(int index);
  // 'SetSelectedWeightVectorWithRollback' changes the weight vector and
  // updates the previous weight vector.
  void SetSelectedWeightVectorWithRollback(int index);
  
  // This can only be used to revert the most recent change. Calling the method
  // more than once between changes will not have any effect.
  void RevertSelectedWeightVector() {
    SetSelectedWeightVector(prev_selected_weight_vector_index_);
  }

  // Populates the weight vectors for a supernode based on its component nodes.
  // 'ratio' represents a desired resource ratio to achieve for -some- of
  // the implementations.
  void PopulateSupernodeWeightVectors(
      const std::vector<int>& ratio, bool restrict_to_default_implementation,
      int max_implementations_per_supernode);

  // Sets the weight vectors of the internal nodes of a supernode to match
  // the selected supernode weight vector. In many cases it is OK for the
  // state of the internal weight vectors to be inconsistent with the supernode
  // weight vector, so this method can be called on demand rather than always
  // being performed when the supernode weight vector changes.
  void SetSupernodeInternalWeightVectors();

  // Debug method that checks whether the selected supernode weight vector
  // matches the sum of the selected weight vectors for the internal nodes;
  void CheckSupernodeWeightVectorOrDie();

  // Debug method that checks the structure of the internal graph and exits if
  // it is not a valid structure.
  void CheckInternalGraphOrDie();

  // Remove all ports from the internal graph.
  void StripPorts();

  PortMap& ports() { return ports_; }
  const PortMap& ports() const { return ports_; }
  NodeMap& internal_nodes() { return internal_nodes_; }
  const NodeMap& internal_nodes() const { return internal_nodes_; }
  EdgeMap& internal_edges() { return internal_edges_; }
  const EdgeMap& internal_edges() const { return internal_edges_; }

  int id;
  std::string name;

  // Used for KLFM algorithm.
  bool is_locked;
 
 protected:
  virtual void SwapPortConnectionByType(int old_id, int new_id,
                                        Port::PortType port_type);

  // If 'indices' is non-NULL, inserts node_id/selected_weight_vector_index
  // pairs, representing the weight vectors that generated the total selected
  // weight.
  std::vector<int> TotalInternalSelectedWeight(
      std::vector<std::pair<int,int>>* indices) const;

  void AddSupernodeWeightVector(const std::vector<int>& wv,
      const std::vector<std::pair<int,int>>& internal_indices);

  EdgeMap internal_edges_;
  NodeMap internal_nodes_;
  PortMap ports_;
  std::vector<std::vector<int>> weight_vectors_;
  // For every entry in 'weight_vectors_', there is a corresponding entry in
  // 'internal_node_weight_vector_indices_' with a matching index. This entry
  // is a vector with a pair entry for each internal node, indicating which of
  // that node's weight vectors is assumed to be selected for the corresponding
  // supernode weight vector. This is necessary to ensure that the internal
  // nodes are set to the correct weight vector when the supernode is destroyed.
  std::vector<std::vector<std::pair<int, int>>>
      internal_node_weight_vector_indices_;
  int selected_weight_vector_index_;
  // Retain the last selected weight vector index.
  int prev_selected_weight_vector_index_;

  bool registered_;
  double latency_;

 private:
};

#endif /* NODE_H_ */
