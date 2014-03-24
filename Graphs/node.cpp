#include "node.h"

#include <cstdio>
#include <limits>
#include <random>
#include <sstream>
#include <utility>

#include "id_manager.h"
#include "universal_macros.h"
#include "weight_score.h"

using namespace std;

Node::Node(Node* src) {
  CopyFrom(src);
}

Node::Node(int node_id, const string& node_name)
  : id(node_id), name(node_name), is_locked(false),
    selected_weight_vector_index_(0), prev_selected_weight_vector_index_(0),
    registered_(false), latency_(0.0) {
}

void Node::AddConnection(int connected_id) {
  for (auto& port_pair : ports_) {
    if (port_pair.second.external_edge_id == connected_id) {
      return;
    }
  }
  int port_id = IdManager::AcquireNodeId();
  ports_.insert(make_pair(port_id, Port(port_id, IdManager::kReservedTerminalId,
                                        connected_id, Port::kDontCareType)));
}

int Node::RemoveConnection(int connected_id) {
  vector<int> found_ids;
  for (auto& port_pair : ports_) {
    if (port_pair.second.external_edge_id == connected_id) {
      found_ids.push_back(port_pair.first);
    }
  }
  for (auto port_id : found_ids) {
    ports_.erase(port_id);
  }
  return found_ids.size();
}

void Node::AddWeightVector(const vector<int>& wv) {
  assert(!is_supernode());
  weight_vectors_.push_back(wv);
}


void Node::CopyFrom(Node* src) {
  id = src->id;
  name = src->name;
  is_locked = src->is_locked;
  selected_weight_vector_index_ = src->selected_weight_vector_index_;
  registered_ = src->registered_;
  latency_ = src->latency_;

  for (auto it : src->internal_edges_) {
    Edge* edge = new Edge();
    edge->CopyFrom(it.second);
    internal_edges_.insert(make_pair(it.first, edge));
  }
  for (auto it : src->internal_nodes_) {
    Node* node = new Node(it.second);
    internal_nodes_.insert(make_pair(it.first, node));
  }
  for (auto it : src->ports_) {
    ports_.insert(it);
  }
  for (auto it : src->weight_vectors_) {
    weight_vectors_.push_back(it);
  }
}

void Node::SwapPortConnection(int old_id, int new_id) {
  bool found = false;
  for (auto& port_pair : ports_) {
    Port& port = port_pair.second;
    if (port.external_edge_id == old_id) {
      port.external_edge_id = new_id;
      found = true;
    }
  }
  assert_b(found) {
    printf("Fatal error: Failed to locate port with correct ID while "
           "swapping.");
  }
}

void Node::SwapPortConnectionByType(int old_id, int new_id,
                                    Port::PortType port_type) {
  for (auto& port_pair : ports_) {
    Port& port = port_pair.second;
    if (port.type == port_type && port.external_edge_id == old_id) {
      port.external_edge_id = new_id;
      return;
    }
  }
  printf("Fatal error: Failed to locate port with correct ID while swapping.");
  assert(false);
}


void Node::Print(bool recursive) const {
  if (is_supernode()) {
    printf("SuperNode  ID=%d  Name=%s\n", id, name.c_str()); 
  } else {
    printf("Node  ID=%d  Name=%s\n", id, name.c_str()); 
  }
  printf("Number of weight vectors: %lu\n", weight_vectors_.size());
  printf("Weights:\n");
  for (auto& wv : weight_vectors_) {
    for (auto w : wv) {
      printf("%d ", w);
    }
    printf("\n");
  }
  printf("Edge IDs: ");
  for (auto& it : ports_) {
    printf("%d ", it.second.external_edge_id);
  }
  printf("\n");
  if (is_supernode()) {
    printf("Internal Nodes: ");
    for (auto it : internal_nodes_) {
      printf("%d ", it.first);
    }
    printf("\n");
    printf("Internal Edges: ");
    for (auto it : internal_edges_) {
      printf("%d ", it.first);
    }
    printf("\n");
    if (recursive) {
      printf("\n");
      PrintPorts();
      printf("\n");
      PrintInternalNodes(true);
      printf("\n");
      PrintInternalEdges();
      printf("\n");
    }
  }
}

void Node::PrintPorts() const {
  for (auto& it : ports_) {
    it.second.Print();
    printf("\n");
  }
}

void Node::PrintInternalNodes(bool recursive) const {
  for (auto it : internal_nodes_) {
    it.second->Print(recursive);
    printf("\n");
  }
}

void Node::PrintInternalEdges() const {
  for (auto it : internal_edges_) {
    it.second->Print();
    printf("\n");
  }
}

void Node::PrintInternalNodeSelectedWeights() const {
  vector<int> total_weights;
  for (auto it : internal_nodes_) {
    printf("%d: ", it.second->id);
    const vector<int>& dwv = it.second->SelectedWeightVector();
    for (auto w : dwv) {
      printf("%d ", w);
    }
    printf("\n");
  }
}

bool Node::is_supernode() const {
  return (internal_nodes_.size() || internal_edges_.size());
}

vector<int> Node::TotalInternalSelectedWeight(
    vector<pair<int,int>>* indices) const {
  assert(is_supernode());
  int num_res = internal_nodes_.begin()->second->SelectedWeightVector().size();
  vector<int> total_weights;
  total_weights.insert(total_weights.begin(), num_res, 0);
  for (auto it : internal_nodes_) {
    const vector<int>& dwv = it.second->SelectedWeightVector();
    for (size_t i = 0; i < dwv.size(); i++) {
      total_weights[i] += dwv[i];
    }
    if (indices != NULL) {
      indices->push_back(
          make_pair(it.first, it.second->selected_weight_vector_index()));
    }
  }
  return total_weights;
}

void Node::TransferEdgeConnectionsExcluding(
    Edge* from_edge, Edge* to_edge, int exclude_id) {
  for (auto entity_id : from_edge->connection_ids()) {
    if (entity_id != exclude_id) {
      to_edge->AddConnection(entity_id);
      if (internal_nodes_.find(entity_id) != internal_nodes_.end()) {
        internal_nodes_.at(entity_id)->SwapPortConnection(from_edge->id,
                                                          to_edge->id);
      } else if (ports_.find(entity_id) != ports_.end()) {
        Port& p = ports_.at(entity_id);
        p.internal_edge_id = to_edge->id;
      } else {
        printf("Fatal error: Edge source not found in internal nodes");
        assert(false);
      }
    }
  }
}

void Node::PopulateSupernodeWeightVectors(
    const vector<int>& /* ratio */, bool restrict_to_default_implementation,
    int max_implementations_per_supernode) {
  assert(!internal_nodes_.empty());
  weight_vectors_.clear();
  internal_node_weight_vector_indices_.clear();
  int max_possible_vectors = 1;
  int num_resources_per_vector =
      internal_nodes_.begin()->second->SelectedWeightVector().size();
  for (auto node_pair : internal_nodes_) {
    max_possible_vectors *= node_pair.second->WeightVectors().size();
  }
  // Always set the weight vector constructed from the default weight vectors.
  vector<pair<int,int>> indices;
  vector<int> default_weight_vector =
      TotalInternalSelectedWeight(&indices);
  AddSupernodeWeightVector(default_weight_vector, indices);
  SetSelectedWeightVector(0);
  if (max_possible_vectors == 1 || restrict_to_default_implementation) {
    return;
  } else if (max_possible_vectors <= max_implementations_per_supernode) {
    // Easy case, include all possible implementations.
    vector<vector<int>> permutations;
    permutations.resize(1);
    permutations[0].assign(num_resources_per_vector, 0);
    vector<vector<pair<int,int>>> permutation_indices;
    permutation_indices.resize(1);
    for (auto node_pair : internal_nodes_) {
      vector<vector<int>> next_step; 
      vector<vector<pair<int,int>>> next_step_indices;
      for (size_t p_idx = 0; p_idx < permutations.size(); p_idx++) {
        for (size_t wv_index = 0;
             wv_index < node_pair.second->WeightVectors().size(); wv_index++) {
          vector<int> intermediate_wv = permutations[p_idx];
          const vector<int>& wv_modifier =
              node_pair.second->WeightVector(wv_index);
          for (size_t i = 0; i < wv_modifier.size(); i++) {
            intermediate_wv[i] += wv_modifier[i];
          }
          vector<pair<int,int>> intermediate_indices =
              permutation_indices[p_idx];
          intermediate_indices.push_back(make_pair(node_pair.first, wv_index));
          next_step.push_back(intermediate_wv);
          next_step_indices.push_back(intermediate_indices);
        }
      }
      permutations = next_step;
      permutation_indices = next_step_indices;
    }
    assert(permutations.size() == permutation_indices.size());
    assert(permutations.size() == max_possible_vectors);
    for (size_t p_idx = 0; p_idx < permutations.size(); p_idx++) {
      vector<int>& permutation = permutations[p_idx];
      vector<pair<int,int>>& indices = permutation_indices[p_idx];
      AddSupernodeWeightVector(permutation, indices);
    }
    return;
  } else {
    // Tough (common) case: Have to choose between implementations.
    vector<vector<int>> implementations;
    vector<vector<pair<int,int>>> implementations_indices;

    // Try adding one implementation maximally-weighted toward each reseource.
    for (int res = 0; res < num_resources_per_vector; res++) {
      vector<int> implementation;
      vector<pair<int,int>> implementation_indices;
      implementation.assign(num_resources_per_vector, 0);
      for (auto node_pair : internal_nodes_) {
        int max_index = 0;
        int max_weight = 0;
        for(int wv_index = 0;
            wv_index < node_pair.second->WeightVectors().size(); wv_index++) {
            auto& wv = node_pair.second->WeightVector(wv_index);
          if (wv[res] > max_weight) {
            max_weight = wv[res];
            max_index = wv_index;
          }
        }
        for (int i = 0; i < implementation.size(); i++) {
          implementation[i] += node_pair.second->WeightVector(max_index)[i];
        }
        implementation_indices.push_back(make_pair(node_pair.first, max_index));
      }
      implementations.push_back(implementation);
      implementations_indices.push_back(implementation_indices);
    }
    // Add two ratio-matched implementations.
    // Forward
    vector<int> implementation;
    vector<pair<int,int>> implementation_indices;
    implementation.assign(num_resources_per_vector, 0);
    for (auto node_it = internal_nodes_.begin();
         node_it != internal_nodes_.end(); node_it++) {
      int best_index = -1;
      double max_diff = std::numeric_limits<double>::max();
      for (int wv_index = 0; wv_index < node_it->second->WeightVectors().size();
           wv_index++) {
        auto& wv = node_it->second->WeightVector(wv_index);
        vector<int> temp = implementation;
        for (int i = 0; i < temp.size(); i++) {
          temp[i] += wv[i];
        }
        double total_diff = 0.0;
        for (int res = 0; res < num_resources_per_vector; res++) {
          total_diff += abs(temp[res]);
        }
        if (total_diff < max_diff) {
          best_index = wv_index;
          max_diff = total_diff;
        }
      }
      assert(best_index >= 0);
      for (size_t i = 0; i < implementation.size(); i++) {
        implementation[i] += node_it->second->WeightVector(best_index)[i];
      }
      implementation_indices.push_back(make_pair(node_it->first, best_index));
    }
    implementations.push_back(implementation);
    implementations_indices.push_back(implementation_indices);
    // Backward
    implementation.clear();
    implementation.assign(num_resources_per_vector, 0);
    implementation_indices.clear();
    for (auto node_it = internal_nodes_.rbegin();
         node_it != internal_nodes_.rend(); node_it++) {
      int best_index = -1;
      double max_diff = std::numeric_limits<double>::max();
      for (size_t wv_index = 0; wv_index < node_it->second->WeightVectors().size();
          wv_index++) {
        auto& wv = node_it->second->WeightVector(wv_index);
        vector<int> temp = implementation;
        assert(temp.size() == num_resources_per_vector);
        for (size_t i = 0; i < temp.size(); i++) {
          temp[i] += wv[i];
        }
        double total_diff = 0.0;
        for (int res = 0; res < num_resources_per_vector; res++) {
          total_diff += abs(temp[res]);
        }
        if (total_diff < max_diff) {
          best_index = wv_index;
          max_diff = total_diff;
        }
      }
      assert(best_index >= 0);
      for (size_t i = 0; i < implementation.size(); i++) {
        implementation[i] += node_it->second->WeightVector(best_index)[i];
      }
      implementation_indices.push_back(make_pair(node_it->first, best_index));
    }
    implementations.push_back(implementation);
    implementations_indices.push_back(implementation_indices);
    default_random_engine random_engine;
    random_engine.seed(0);
    while (implementations.size() < max_implementations_per_supernode) {
      // TODO This may produce duplicate implementations, which is wasteful.
      vector<int> implementation(num_resources_per_vector, 0);
      vector<pair<int,int>> implementation_indices;
      for (auto node_pair : internal_nodes_) {
        int selected_node_impl =
            random_engine() % node_pair.second->WeightVectors().size();
        for (int i = 0; i < num_resources_per_vector; i++) {
          implementation[i] +=
              node_pair.second->WeightVector(selected_node_impl).at(i);
        }
        implementation_indices.push_back(
            make_pair(node_pair.second->id, selected_node_impl));;
      }
      implementations.push_back(implementation);
      implementations_indices.push_back(implementation_indices);
    }
    assert(implementations.size() == implementations_indices.size());
    for (int imp_idx = 0; imp_idx < implementations.size(); imp_idx++) {
      AddSupernodeWeightVector(
          implementations[imp_idx], implementations_indices[imp_idx]);
    }
  }
}

void Node::AddSupernodeWeightVector(
    const vector<int>& wv, const vector<pair<int,int>>& internal_indices) {
  assert(is_supernode());
  weight_vectors_.push_back(wv);
  internal_node_weight_vector_indices_.push_back(internal_indices);
  assert(weight_vectors_.size() == internal_node_weight_vector_indices_.size());
}

void Node::CheckSupernodeWeightVectorOrDie() {
  if (!is_supernode()) {
    return;
  }
  SetSupernodeInternalWeightVectors();
  vector<int> supernode_weight_vector = SelectedWeightVector();
  vector<int> computed_weight_vector;
  computed_weight_vector.assign(supernode_weight_vector.size(), 0);
  for (auto it : internal_nodes_) {
    // Recursively check each sub-node.
    it.second->CheckSupernodeWeightVectorOrDie();
    vector<int> node_weight_vector = it.second->SelectedWeightVector();
    for (size_t i = 0; i < node_weight_vector.size(); i++) {
      computed_weight_vector[i] += node_weight_vector[i];
    }
  }
  for (size_t i = 0; i < supernode_weight_vector.size(); i++) {
    assert_b(supernode_weight_vector[i] == computed_weight_vector[i]) {
      cout << "Supernode " << id << " weight vector: ";
      for (int res = 0; res < supernode_weight_vector.size(); res++) {
        cout << supernode_weight_vector[res] << " ";
      }
      cout << "[WV Index: " << selected_weight_vector_index_ << "]" << endl;
      cout << "Internal node seleced weight vectors:" << endl;
      for (auto it : internal_nodes_) {
        vector<int> node_weight_vector = it.second->SelectedWeightVector();
        cout << "Internal Node " << it.first << ": ";
        for (int res = 0; res < node_weight_vector.size(); res++) {
          cout << node_weight_vector[res] << " ";
        }
        cout << "[WV Index: " << it.second->selected_weight_vector_index()
              << " Expected: ";
        for (auto idx_it : internal_node_weight_vector_indices_[selected_weight_vector_index_]) {
          if (idx_it.first == it.first) {
            cout << idx_it.second << "]" << endl;
          }
        }
      }
      cout << endl;
    }
  }
}

void Node::SetSelectedWeightVector(int index) {
  assert(index < weight_vectors_.size());
  selected_weight_vector_index_ = index;
}

void Node::SetSupernodeInternalWeightVectors() {
  if (is_supernode()) {
    // Set the internal node implementations to match the supernode weight
    // vector.
    assert(weight_vectors_.size() ==
           internal_node_weight_vector_indices_.size());
    int index = selected_weight_vector_index_;
    for (auto node_index_pair : internal_node_weight_vector_indices_[index]) {
      internal_nodes_.at(node_index_pair.first)->SetSelectedWeightVector(
          node_index_pair.second);
    }
    RUN_DEBUG(DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR, 0) {
      CheckSupernodeWeightVectorOrDie();
    }
  }
}

void Node::SetSelectedWeightVectorWithRollback(int index) {
  prev_selected_weight_vector_index_ = selected_weight_vector_index_;
  SetSelectedWeightVector(index);
}

// TODO - It probably makes more sense to make this a method in
// PartitionEngineKlfm instead.
void Node::SetWeightVectorToMinimizeImbalance(
    vector<int>& balance, const vector<int>& max_weight_imbalance,
    bool is_positive, bool use_imbalance, bool use_ratio,
    const vector<int>& res_ratios, const vector<int>& total_weight) {
  const auto selected_weight_vector = SelectedWeightVector();
  int best_index = -1;
  double best_imbalance = numeric_limits<double>::max();
  std::vector<int> best_balance;
  for (int i = 0; i < weight_vectors_.size(); i++) {
    const auto& weight_vector = weight_vectors_[i];
    std::vector<int> modified_balance = balance;
    for (int j = 0; j < weight_vector.size(); j++) {
      if (is_positive) {
        modified_balance[j] += (weight_vector[j] - selected_weight_vector[j]);
      } else {
        modified_balance[j] -= (weight_vector[j] - selected_weight_vector[j]);
      }
    }
    double modified_imbalance = 0;
    if (use_imbalance) {
      modified_imbalance += NearViolaterImbalancePower(
        modified_balance, max_weight_imbalance);
    }
    if (use_ratio) {
      modified_imbalance += RatioPowerIfChanged(
          selected_weight_vector, weight_vector, res_ratios, total_weight);
    }
    if (modified_imbalance < best_imbalance) {
      best_index = i;
      best_imbalance = modified_imbalance;
      best_balance = modified_balance;
    }
  }
  assert(best_index >= 0);
  if (best_index != selected_weight_vector_index_) {
    RUN_DEBUG(DEBUG_OPT_REBALANCE, 2) {
      cout << "Changing weight vector from ";
      for (auto i : SelectedWeightVector()) {
        cout << i << " ";
      }
      cout << "to ";
    }
    SetSelectedWeightVector(best_index);
    RUN_DEBUG(DEBUG_OPT_REBALANCE, 2) {
      for (auto i : SelectedWeightVector()) {
        cout << i << " ";
      }
      cout << endl;
      cout << "Changing balance from ";
      for (auto i : balance) {
        cout << i << " ";
      }
      cout << "to ";
    }
    balance = best_balance;
    RUN_DEBUG(DEBUG_OPT_REBALANCE, 2) {
      for (auto i : balance) {
        cout << i << " ";
      }
      cout << endl;
    }
    RUN_DEBUG(DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR, 2) {
      CheckSupernodeWeightVectorOrDie();
    }
  }
}
