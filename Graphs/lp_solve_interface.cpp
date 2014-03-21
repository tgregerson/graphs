#include "lp_solve_interface.h"

#include <cstdlib>

#include <sstream>

#include "chaco_parser.h"
#include "edge.h"
#include "node.h"
#include "processed_netlist_parser.h"

using namespace std;

void LpSolveInterface::LoadFromMps(const string& filename) {
  if (verbose_) {
    cout << "Reading MPS model from " << filename << "...";
  }
  lprec* model = read_MPS(const_cast<char*>(filename.c_str()), NORMAL);
  if (model == nullptr) {
    throw LpSolveException("Could not read model from " + filename);
  }
  if (verbose_) {
    cout << "Done." << endl;
    cout << "Model contains " << get_Ncolumns(model) << " variables and "
         << get_Nrows(model) << " constraints." << endl;
  }
  state_.reset(new LpSolveState(model));
}

void LpSolveInterface::LoadFromNtl(const string& filename) {
  ProcessedNetlistParser netlist_parser;
  Node graph;
  netlist_parser.Parse(&graph, filename.c_str(), nullptr);
  GraphParsingState gpstate(&graph, max_imbalance_, verbose_);
  state_.reset(new LpSolveState(gpstate.ConstructModel()));
}

void LpSolveInterface::LoadFromChaco(const string& filename) {
  ChacoParser chaco_parser;
  Node graph;
  if (!chaco_parser.Parse(&graph, filename.c_str())) {
    throw LpSolveException("Error parsing CHACO graph from: " + filename);
  }
  GraphParsingState gpstate(&graph, max_imbalance_, verbose_);
  state_.reset(new LpSolveState(gpstate.ConstructModel()));
}

void LpSolveInterface::WriteToLp(const string& filename) const {
  if (verbose_) {
    cout << "Writing LP model to " << filename << endl;
  }
  if (state_.get() == nullptr) {
    throw LpSolveException("Trying to write LP without creating model.");
  }
  lprec* model = CHECK_NOTNULL(state_.get())->model();
  if (!write_lp(model, const_cast<char*>(filename.c_str()))) {
    throw LpSolveException("Could not write file " + filename);
  }
}

void LpSolveInterface::WriteToMps(const string& filename) const {
  if (verbose_) {
    cout << "Writing MPS model to " << filename << endl;
  }
  if (state_.get() == nullptr) {
    throw LpSolveException("Trying to write MPS without creating model.");
  }
  lprec* model = CHECK_NOTNULL(state_.get())->model();
  if (!write_mps(model, const_cast<char*>(filename.c_str()))) {
    throw LpSolveException("Could not write file " + filename);
  }
}

void LpSolveInterface::RunSolver(long timeout_s) {
  lprec* model = CHECK_NOTNULL(state_.get())->model();
  if (timeout_s > 0) {
    set_timeout(model, timeout_s);
  }

  if (verbose_) {
    set_verbose(model, DETAILED);
  }
  int ret = solve(model);

  cout << "Solve exited with return code: " << ret << endl;
  cout << "Objective value: " << get_objective(model) << endl;
}

LpSolveInterface::LpSolveState::LpSolveState() : model_(nullptr, delete_lp) {
  lprec* model = make_lp(0, 0);
  if (model == nullptr) {
    throw LpSolveException("Failed to create LPREC model.");
  }
  model_.reset(model);
}

LpSolveInterface::GraphParsingState::GraphParsingState(
    Node* graph, double max_imbalance, bool verbose)
    : graph_(CHECK_NOTNULL(graph)),
      max_weight_imbalance_fraction_(max_imbalance),
      verbose_(verbose),
      next_variable_index_(1),
      columns_ex_next_row_(1) {
  if (graph->internal_nodes().empty()) {
    throw LpSolveException("Provided empty graph.");
  }
  num_resources_ =
    CHECK_NOTNULL(graph->internal_nodes().begin()->second)->num_resources();
  if (num_resources_ <= 0) {
    throw LpSolveException("Weight vector has invalid number of resources");
  }
}

lprec* LpSolveInterface::GraphParsingState::ConstructModel(bool column_mode) {
  if (column_mode) {
    return ConstructModelColumnMode();
  } else {
    return ConstructModelRowMode();
  }
}

lprec* LpSolveInterface::GraphParsingState::ConstructModelRowMode() {
  cout << "Using Row Mode" << endl;
  int num_variables =
      NumTotalNodeVariablesNeeded() + NumTotalEdgeVariablesNeeded();
  lprec* model = make_lp(0, num_variables);
  if (model == nullptr) {
    throw LpSolveException("Error creating model.");
  }
  PreAllocateModelMemory(model);
  AssignAllVariableIndices();

  set_add_rowmode(model, TRUE);
  SetObjectiveFunction(model);
  AddAllConstraintsToModel(model, false);
  set_add_rowmode(model, FALSE);

  SetAllVariablesBinary(model);
  SetAllVariableNames(model);

  if (verbose_) {
    cout << "Total Variables: " << get_Ncolumns(model) << endl;
    cout << "Total Constraints: " << get_Nrows(model) << endl;
    int node_variables = 0;
    for (const auto& n : node_to_variable_indices_) {
      for (const auto& v : n.second) {
        node_variables += v.size();
      }
    }
    cout << node_variables << " node variables" << endl;
    cout << edge_to_crossing_variable_indices_.size() << " edge crossing variables" << endl;
    int edge_connectivity_variables = 0;
    for (const auto& v : edge_to_partition_connectivity_variable_indices_) {
      edge_connectivity_variables += v.second.size();
    }
    cout << edge_connectivity_variables << " edge connectivity variables" << endl;
  }
  return model;
}

lprec* LpSolveInterface::GraphParsingState::ConstructModelColumnMode() {
  cout << "Using Column Mode" << endl;
  lprec* model = make_lp(0, 0);
  if (model == nullptr) {
    throw LpSolveException("Error creating model.");
  }
  PreAllocateModelMemory(model);
  AssignAllVariableIndices();

  AddAllConstraintsToModel(model, true);
  AddDeferredColumnsEx(model);

  // Must be set after columns are added.
  SetObjectiveFunction(model);

  SetAllVariablesBinary(model);
  SetAllVariableNames(model);

  if (verbose_) {
    cout << "Total Variables: " << get_Ncolumns(model) << endl;
    cout << "Total Constraints: " << get_Nrows(model) << endl;
    int node_variables = 0;
    for (const auto& n : node_to_variable_indices_) {
      for (const auto& v : n.second) {
        node_variables += v.size();
      }
    }
    cout << node_variables << " node variables" << endl;
    cout << edge_to_crossing_variable_indices_.size() << " edge crossing variables" << endl;
    int edge_connectivity_variables = 0;
    for (const auto& v : edge_to_partition_connectivity_variable_indices_) {
      edge_connectivity_variables += v.second.size();
    }
    cout << edge_connectivity_variables << " edge connectivity variables" << endl;
    cout << columns_ex_.size() << " EX columns" << endl;
  }
  return model;
}

void LpSolveInterface::GraphParsingState::PreAllocateModelMemory(lprec* model) {
  int num_variables =
      NumTotalNodeVariablesNeeded() + NumTotalEdgeVariablesNeeded();
  int num_constraints =
      NumTotalNodeConstraintsNeeded() + NumTotalEdgeConstraintsNeeded() +
      (2 * num_resources_);
  if (verbose_) {
    cout << "Pre-allocating memory for " << num_constraints
         << " constraints and " << num_variables << " variables." << endl;
  }
  if (!resize_lp(model, num_constraints, num_variables)) {
    throw LpSolveException("Failed to pre-allocate memory.");
  }
}

void LpSolveInterface::GraphParsingState::SetObjectiveFunction(lprec* model) {
  set_minim(CHECK_NOTNULL(model));
  int num_x_vars = edge_to_crossing_variable_indices_.size();
  unique_ptr<int[]> indices(new int[num_x_vars]);
  unique_ptr<REAL[]> coeffs(new REAL[num_x_vars]);
  int added = 0;
  for (const pair<int, int>& e : edge_to_crossing_variable_indices_) {
    indices[added] = e.second;
    Edge* edge = graph_->internal_edges().at(e.first);
    coeffs[added++] = (REAL)(edge->weight);
  }
  set_obj_fnex(model, num_x_vars, coeffs.get(), indices.get());
}

void LpSolveInterface::GraphParsingState::SetNodeVariableIndex(
    int node_id, int partition_num, int personality_num, int index) {
  assert(partition_num < num_partitions_);
  vector<vector<int>>& entry = node_to_variable_indices_[node_id];
  if (entry.size() <= partition_num) {
    entry.resize(partition_num + 1);
  }
  if (entry[partition_num].size() <= personality_num) {
    entry[partition_num].resize(personality_num + 1, kIndexGuard);
  }
  entry[partition_num][personality_num] = index;
}

void LpSolveInterface::GraphParsingState::SetEdgeCrossingVariableIndex(
    int edge_id, int index) {
  edge_to_crossing_variable_indices_[edge_id] = index;
}

void LpSolveInterface::GraphParsingState::SetEdgePartitionConnectivityVariableIndex(
    int edge_id, int partition_num, int index) {
  vector<int>& entry =
      edge_to_partition_connectivity_variable_indices_[edge_id];
  if (entry.size() <= partition_num) {
    entry.resize(partition_num + 1, kIndexGuard);
  }
  entry[partition_num] = index;
}

void LpSolveInterface::GraphParsingState::AssignAllVariableIndices() {
  AssignAllNodeVariableIndices();
  AssignAllEdgeVariableIndices();
}

void LpSolveInterface::GraphParsingState::AssignAllNodeVariableIndices() {
  for (const pair<int, Node*>& p : graph_->internal_nodes()) {
    AssignNodeVariableIndices(*(p.second));
  }
}

void LpSolveInterface::GraphParsingState::AssignAllEdgeVariableIndices() {
  for (const pair<int, Edge*>& p : graph_->internal_edges()) {
    AssignEdgeVariableIndices(*(p.second));
  }
}

void LpSolveInterface::GraphParsingState::AssignNodeVariableIndices(
    const Node& node) {
  // Need to add #partitions * #personalities variables.
  // Each variable is a unique <node, partition, personality> 3-tuple with a
  // binary value.
  for (int par = 0; par < num_partitions_; ++par) {
    for (int per = 0; per < node.num_personalities(); ++per) {
      SetNodeVariableIndex(node.id, par, per, next_variable_index_++);
    }
  }
}

void LpSolveInterface::GraphParsingState::AssignEdgeVariableIndices(
    const Edge& edge) {
  SetEdgeCrossingVariableIndex(edge.id, next_variable_index_++);
  for (int i = 0; i < num_partitions_; ++i) {
    SetEdgePartitionConnectivityVariableIndex(
        edge.id, i, next_variable_index_++);
  }
}

void LpSolveInterface::GraphParsingState::AddAllConstraintsToModel(
    lprec* model, bool defer_to_columns) {
  AddImbalanceConstraintsToModel(model, defer_to_columns);
  cout << get_Nrows(model) << " constraints after imbalance" << endl;
  AddAllNodeConstraintsToModel(model, defer_to_columns);
  cout << get_Nrows(model) << " constraints after nodes" << endl;
  AddAllEdgeConstraintsToModel(model, defer_to_columns);
  cout << get_Nrows(model) << " constraints after edges" << endl;
}

void LpSolveInterface::GraphParsingState::AddAllNodeConstraintsToModel(
    lprec* model, bool defer_to_columns) {
  int num_nodes_added = 0;
  for (const pair<int, Node*>& p : graph_->internal_nodes()) {
    AddNodeConstraintsToModel(model, *(p.second), defer_to_columns);
    if (verbose_) {
      ++num_nodes_added;
      if (num_nodes_added % kVerboseAddQuanta == 0) {
        cout << "Added constraints for " << num_nodes_added << " nodes." << endl;
      }
    }
  }
}

void LpSolveInterface::GraphParsingState::AddAllEdgeConstraintsToModel(
    lprec* model, bool defer_to_columns) {
  int num_edges_added = 0;
  for (const pair<int, Edge*>& p : graph_->internal_edges()) {
    AddEdgeConstraintsNewToModel(model, *(p.second), defer_to_columns);
    if (verbose_) {
      ++num_edges_added;
      if (num_edges_added % kVerboseAddQuanta == 0) {
        cout << "Added constraints for " << num_edges_added << " edges." << endl;
      }
    }
  }
}

// Discussion below only applies to Bipartitioning.
// Imbalance constraints are in the following form:
// C_1*V_1 + C_2*V_2 + ... + C_N*V_N <= 0
//
// The number of imbalance constraints is 2 * #Resources. There is a pair of
// constraints associated with each resource.
//
// Where the variables, V, are all of the possible node identity variables.
// A node identity variable indicates whether the node with that identity
// exists with a specific personality and in a specific partition. See the
// node constraints for more details.
//
// The coefficients, C, are based on the maximum allowed imbalance in that
// resource between partitions, and the weight of V's personality in that
// resource. Additionally, calculation depends on which partition the
// node identity is associated with. For example, in Partition A, the
// coefficient might be computed as:
//
// C_1 = W_1 * (I + 1)
//
// And for identities in Partition B:
//
// C_2 = W_2 * (I - 1)
//
// Each resource has a pair of constraints. For the second constraint, the
// coefficient calculation are done as the reverse of what is shown above;
// that is, nodes in Partition B use the first equation and in Partition A
// use the second equation.
void LpSolveInterface::GraphParsingState::AddImbalanceConstraintsToModel(
    lprec* model, bool defer_to_columns) {
  // Constraint equations depend on number of partitions, and currently only
  // support 2.
  assert(num_partitions_ == 2);

  vector<vector<pair<REAL, REAL>>> coefficients;
  vector<vector<int>> variable_indices;
  coefficients.resize(num_resources_);
  variable_indices.resize(num_resources_);
  for (const pair<int, vector<vector<int>>>& nid_mapping :
       node_to_variable_indices_) {
    int node_id = nid_mapping.first;
    Node* node = CHECK_NOTNULL(graph_->internal_nodes().at(node_id));
    for (int part = 0; part < num_partitions_; ++part) {
      int num_personalities = node->WeightVectors().size();
      for (int per = 0; per < num_personalities; ++per) {
        const vector<int>& wv =
            graph_->internal_nodes().at(node_id)->WeightVector(per);
        for (int res = 0; res < num_resources_; ++res) {
          int weight = wv[res];
          variable_indices[res].push_back(nid_mapping.second[part][per]);
          // This part needs to change to support k-way partitioning.
          if (part != 0) {
            coefficients[res].push_back(make_pair(
                (REAL)(weight * (max_weight_imbalance_fraction_ + 1)),
                (REAL)(weight * (max_weight_imbalance_fraction_ - 1))));
          } else {
            coefficients[res].push_back(make_pair(
                (REAL)(weight * (max_weight_imbalance_fraction_ - 1)),
                (REAL)(weight * (max_weight_imbalance_fraction_ + 1))));
          }
        }
      }
    }
  }

  for (int res = 0; res < num_resources_; ++res) {
    int num_nonzero = variable_indices[res].size();
    unique_ptr<int, void(*)(void*)> indices(
        (int*)malloc(num_nonzero * sizeof(int)), std::free);
    unique_ptr<REAL, void(*)(void*)> gz_coeffs(
        (REAL*)malloc(num_nonzero * sizeof(REAL)), std::free);
    unique_ptr<REAL, void(*)(void*)> lz_coeffs(
        (REAL*)malloc(num_nonzero * sizeof(REAL)), std::free);
    for (int i = 0; i < num_nonzero; ++i) {
      indices.get()[i] = variable_indices[res][i];
      gz_coeffs.get()[i] = coefficients[res][i].first;
      lz_coeffs.get()[i] = coefficients[res][i].second;
    }
    if (defer_to_columns) {
      AddConstraintEx(model, 0, nullptr, nullptr, GE, 0.0);
      ConstraintExToDeferredColumns(
          num_nonzero, gz_coeffs.get(), indices.get());
      AddConstraintEx(model, 0, nullptr, nullptr, GE, 0.0);
      ConstraintExToDeferredColumns(
          num_nonzero, lz_coeffs.get(), indices.get());
    } else {
      AddConstraintEx(
          model, num_nonzero, gz_coeffs.get(), indices.get(), GE, 0.0);
      AddConstraintEx(
          model, num_nonzero, lz_coeffs.get(), indices.get(), GE, 0.0);
    }
  }
}

// Node constraints are 'set partitioning constraints'. That is, they are
// designed to ensure that exactly one variable in a set is 1 and the others
// are zero.
//
// The set of variables that must be partitioned are the node identity
// variables associated with a given node. Each node has
// #Partitions * #Personalities identity variables, and if an identity
// variable is set, it indicates that the node has that personality and is
// located in that partition. A node must have exactly one personality
// and be in exactly one partition, hence the set partitioning constraint.
//
// To construct the set partitioning constraint, The node constraint must
// set a coefficient of 1 for all of its identity variables, and 0 for
// all other variables in the model. Setting the constraint = 1 and
// using binary bounds on each variable (done later in the algorithm)
// completes the set partitioning constraint.
void LpSolveInterface::GraphParsingState::AddNodeConstraintsToModel(
    lprec* model, const Node& node, bool defer_to_columns) {
  int num_nonzero = num_partitions_ * node.WeightVectors().size();
  int indices[num_nonzero];
  REAL coeffs[num_nonzero];
  int i = 0;
  for (int part = 0; part < num_partitions_; ++part) {
    for (int per = 0; per < node.WeightVectors().size(); ++per) {
      indices[i] = GetNodeVariableIndex(node.id, part, per);
      coeffs[i++] = 1.0;
    }
  }
  if (defer_to_columns) {
    AddConstraintEx(model, 0, nullptr, nullptr , EQ, 1.0);
    ConstraintExToDeferredColumns(num_nonzero, coeffs, indices);
  } else {
    AddConstraintEx(model, num_nonzero, coeffs, indices, EQ, 1.0);
  }
}

// New version uses threshold constraints that leverage the fact that all of
// the variables are binary. This requires fewer constraints.
//
// An edge requires multiple constraints. The purpose of the edge constraints
// is to define an 'edge crossing variable', which is set to 1 if the edge
// crosses partition boundaries or set to 0 if the edge is completely internal
// to a partition.
//
// X1 = (P_A1 + P_B1 + ... + P_K1 >= 2)
//   Note: Since at least one connectivity variable will always be set, we can
//         also use '!= 1' instead of '>= 2'.
//
// Where 'P_A1' is the edge 1's partition connectivity variable for partition A.
// A partition connectivity variable is set to 1 if the edge is connected to
// any nodes located in the specified partition, or 0 if it is not.
//
// P_A1 = (V_A1 + V_A2 + ... + V_AM >= 1)
//
// Where V_A1 .. V_AM are all the nodes in partition P that are connected to
// the edge.
//
// The threshold assignments above can be implemented using two constraints for
// each threshold. In general, for X = (V_1 + V_2 + ... + V_M >= T):
//
// (M + 1 - T)*X - V_1 - V_2 - ... - V_M >= 1 - T
//           T*X - V_1 - V_2 - ... - V_M <= 0
//
// In this case, T is strictly positive, so it is sufficient to use:
//
// M*X - V_1 - V_2 - ... - V_M >= 1 - T
// T*X - V_1 - V_2 - ... - V_M <= 0
void LpSolveInterface::GraphParsingState::AddEdgeConstraintsNewToModel(
    lprec* model, const Edge& edge, bool defer_to_columns) {

  int edge_crossing_variable_index =
      GetEdgeCrossingVariableIndex(edge.id);

  // All of the coefficients for the two equations are the same, except for the
  // first element, so just re-use the same array and set the first element.
  REAL x_coeffs[1 + num_partitions_];
  int x_variable_indices[1 + num_partitions_];

  const int x_thresh = 2;
  x_variable_indices[0] = edge_crossing_variable_index;

  for (int i = 0; i < num_partitions_; ++i) {
    x_coeffs[i + 1] = -1.0;
    x_variable_indices[i + 1] =
        GetEdgePartitionConnectivityVariableIndex(edge.id, i);
  }
  if (defer_to_columns) {
    x_coeffs[0] = (REAL)num_partitions_;
    AddConstraintEx(model, 0, nullptr, nullptr, GE, (REAL)(1 - x_thresh));
    ConstraintExToDeferredColumns(
        1 + num_partitions_, x_coeffs, x_variable_indices);
    x_coeffs[0] = x_thresh;
    AddConstraintEx(model, 0, nullptr, nullptr, LE, 0.0);
    ConstraintExToDeferredColumns(
        1 + num_partitions_, x_coeffs, x_variable_indices);
  } else {
    x_coeffs[0] = (REAL)num_partitions_;
    AddConstraintEx(model, 1 + num_partitions_, x_coeffs, x_variable_indices,
                    GE, (REAL)(1 - x_thresh));
    x_coeffs[0] = x_thresh;
    AddConstraintEx(model, 1 + num_partitions_, x_coeffs, x_variable_indices,
                    LE, 0.0);
  }

  const int p_thresh = 1;
  for (int part = 0; part < num_partitions_; ++part) {
    vector<int> p_indices;
    vector<REAL> p_coeffs;
    p_indices.push_back(
        GetEdgePartitionConnectivityVariableIndex(edge.id, part));
    for (int node_id : edge.connection_ids()) {
      Node* node = CHECK_NOTNULL(graph_->internal_nodes().at(node_id));
      for (size_t per = 0; per < node->WeightVectors().size(); ++per) {
        p_indices.push_back(GetNodeVariableIndex(node_id, part, per));
      }
    }
    p_coeffs.resize(p_indices.size(), -1.0);
    if (defer_to_columns) {
      p_coeffs[0] = p_indices.size() - 1;
      AddConstraintEx(model, 0, nullptr, nullptr, GE, (REAL)(1 - p_thresh));
      ConstraintExToDeferredColumns(p_coeffs, p_indices);
      p_coeffs[0] = p_thresh;
      AddConstraintEx(model, 0, nullptr, nullptr, LE, 0.0);
      ConstraintExToDeferredColumns(p_coeffs, p_indices);
    } else {
      p_coeffs[0] = p_indices.size() - 1;
      AddConstraintEx(model, p_coeffs, p_indices, GE, REAL(1 - p_thresh));
      p_coeffs[0] = p_thresh;
      AddConstraintEx(model, p_coeffs, p_indices, LE, 0.0);
    }
  }
}

// Old version uses OR and AND equations. Keep around while validating new
// method.
// An edge requires multiple constraints. The purpose of the edge constraints
// is to define an 'edge crossing variable', which is set to 1 if the edge
// crosses partition boundaries or set to 0 if the edge is completely internal
// to a partition.
//
// The edge crossing variable, X, is defined by adding a Boolean assignment that
// of the form:
//
// X1 = P_A1 && P_B1  // For 2 partitions
//
// Where 'P_A' is the edge's partition connectivity variable for partition A.
// A partition connectivity variable is set to 1 if the edge is connected to
// any nodes located in the specified partition, or 0 if it is not.
//
// The partition connectivity can be defined by a Boolean assignment using the
// node identity variables (see discussion of node constraints). For instance,
//
// P_A = V_A1 || V_A2 || ... || V_AM
//
// Where V_A1 .. V_AM are all of the node identity variables associated with
// partition A.
//
// Assembling the Boolean assignment can be done as follows:
//
// OR assignment:
//
// To implement the OR assignment requires M + 1 constraints of the form:
//
// P_A - V_A1 - V_A2 - ... - V_AM <= 0
// P_A - V_A1 >= 0
// P_A - V_A2 >= 0
// ...
// P_A - V_AM >= 0
//
// AND assignment:
//
// To implement an AND assignment requires K + 1 constraints of the form:
//
// X1 - P_A1 - P_B1 - ... - P_K1 >= 1 - K
// X1 - P_A1 <= 0
// ...
// X1 - P_K1 <= 0
void LpSolveInterface::GraphParsingState::AddEdgeConstraintsToModel(
    lprec* model, const Edge& edge, bool defer_to_columns) {
  assert (num_partitions_ == 2);

  int edge_crossing_variable_index =
      GetEdgeCrossingVariableIndex(edge.id);
  REAL and_coeffs1[1 + num_partitions_];
  int and_variable_indices1[1 + num_partitions_];

  and_coeffs1[0] = 1.0;
  and_variable_indices1[0] = edge_crossing_variable_index;
  for (int i = 0; i < num_partitions_; ++i) {
    and_coeffs1[i + 1] = -1.0;
    and_variable_indices1[i + 1] =
        GetEdgePartitionConnectivityVariableIndex(edge.id, i);
  }
  if (defer_to_columns) {
    AddConstraintEx(model, 0, nullptr, nullptr, GE, REAL(1 - num_partitions_));
    ConstraintExToDeferredColumns(
        1 + num_partitions_, and_coeffs1, and_variable_indices1);
  } else {
    AddConstraintEx(
        model, 1 + num_partitions_, and_coeffs1, and_variable_indices1, GE,
        (REAL)(1 - num_partitions_));
  }

  for (int i = 0; i < num_partitions_; ++i) {
    int and_variable_indices[2];
    REAL and_coeffs[2];
    and_variable_indices[0] = edge_crossing_variable_index;
    and_variable_indices[1] =
        GetEdgePartitionConnectivityVariableIndex(edge.id, i);
    and_coeffs[0] = 1.0;
    and_coeffs[1] = -1.0;
    if (defer_to_columns) {
      AddConstraintEx(model, 0, nullptr, nullptr, LE, 0.0);
      ConstraintExToDeferredColumns(
          2, and_coeffs, and_variable_indices);
    } else {
      AddConstraintEx(model, 2, and_coeffs, and_variable_indices, LE, 0.0);
    }
  }

  // OR constraints
  //
  // P_k(n) - V1 - V2 - V3 <= 0
  // P_k(n) - V1 >= 0
  // P_k(n) - V2 >= 0
  // P_k(n) - V3 >= 0
  //
  // Repeat for all partition variables.
  for (int part = 0; part < num_partitions_; ++part) {
    int partition_variable_index =
        GetEdgePartitionConnectivityVariableIndex(edge.id, part);
    vector<pair<int, REAL>> eq1_index_coeff_pairs;
    for (int node_id : edge.connection_ids()) {
      Node* node = CHECK_NOTNULL(graph_->internal_nodes().at(node_id));
      for (size_t per = 0; per < node->WeightVectors().size(); ++per) {
        int node_variable_index = GetNodeVariableIndex(node_id, part, per);
        REAL or_coeffs[2];
        int or_variable_indices[2];
        or_coeffs[0] = 1.0;
        or_coeffs[1] = -1.0;
        or_variable_indices[0] = partition_variable_index;
        or_variable_indices[1] = node_variable_index;
        if (defer_to_columns) {
          AddConstraintEx(model, 0, nullptr, nullptr, GE, 0.0);
          ConstraintExToDeferredColumns(
              2, or_coeffs, or_variable_indices);
        } else {
          AddConstraintEx(
              model, 2, or_coeffs, or_variable_indices, GE, 0.0);
        }
        eq1_index_coeff_pairs.push_back(make_pair(node_variable_index, -1.0));
      }
    }
    eq1_index_coeff_pairs.push_back(make_pair(partition_variable_index, 1.0));

    int num_vals = eq1_index_coeff_pairs.size();
    REAL or_coeffs1[num_vals];
    int or_variable_indices1[num_vals];
    for (size_t i = 0; i < eq1_index_coeff_pairs.size(); ++i) {
      or_variable_indices1[i] = eq1_index_coeff_pairs[i].first;
      or_coeffs1[i] = eq1_index_coeff_pairs[i].second;
    }
    if (defer_to_columns) {
      AddConstraintEx(model, 0, nullptr, nullptr, LE, 0.0);
      ConstraintExToDeferredColumns(
          num_vals, or_coeffs1, or_variable_indices1);
    } else {
      AddConstraintEx(
          model, num_vals, or_coeffs1, or_variable_indices1, LE, 0.0);
    }
  }
}

void LpSolveInterface::GraphParsingState::SetAllVariablesBinary(
    lprec* model) {
  for (int i = 1; i <= get_Ncolumns(model); ++i) {
    if (!set_binary(model, i, TRUE)) {
      throw LpSolveException("Error setting variable to binary.");
    }
  }
}

void LpSolveInterface::GraphParsingState::SetAllVariableNames(
    lprec* model) {
  // Node identity variables
  // Example: N66B2
  // (Node 66, in partition B, with personality 2)
  for (const auto& p_i_vv : node_to_variable_indices_) {
    int node_id = p_i_vv.first;
    const vector<vector<int>>& part_per_v = p_i_vv.second;
    for (int part = 0; part < part_per_v.size(); ++part) {
      for (int per = 0; per < part_per_v[part].size(); ++per) {
        assert(part < 2);  // currently only support bipartition.
        int variable_index = part_per_v[part][per];
        stringstream name;
        name << "N" << node_id;
        if (part == 0) {
          name << "A";
        } else {
          name << "B";
        }
        name << per;
        set_col_name(model, variable_index,
                     const_cast<char*>(name.str().c_str()));
      }
    }
  }

  // Edge crossing variables
  // Example: X92
  // (Edge 92)
  for (pair<int, int> p : edge_to_crossing_variable_indices_) {
    stringstream name;
    name << "X" << p.first;
    set_col_name(model, p.second, const_cast<char*>(name.str().c_str()));
  }

  // Edge partition connectivity variables
  // Example: C92A
  // (Connectivity of edge 92 to partition A)
  for (const pair<int, vector<int>>& p :
       edge_to_partition_connectivity_variable_indices_) {
    for (int part = 0; part < p.second.size(); ++part) {
      stringstream name;
      name << "C" << p.first;
      if (part == 0) {
        name << "A";
      } else {
        name << "B";
      }
      set_col_name(model, p.second[part],
                   const_cast<char*>(name.str().c_str()));
    }
  }
}

int LpSolveInterface::GraphParsingState::NumTotalNodeVariablesNeeded() {
  int total_personalities = 0;
  for (const pair<int, Node*>& p : graph_->internal_nodes()) {
    total_personalities += p.second->num_personalities();
  }
  return total_personalities * num_partitions_;
}

int LpSolveInterface::GraphParsingState::NumTotalEdgeVariablesNeeded() {
  return (1 + num_partitions_) * graph_->internal_edges().size();
}

int LpSolveInterface::GraphParsingState::NumTotalNodeConstraintsNeeded() {
  return graph_->internal_nodes().size();
}

int LpSolveInterface::GraphParsingState::NumTotalEdgeConstraintsNeeded() {
  int total_connections = 0;
  for (const pair<int, Edge*>& p : graph_->internal_edges()) {
    total_connections += p.second->degree();
  }
  int num_and_constraints_needed =
      (1 + num_partitions_) * graph_->internal_edges().size();
  int num_or_constraints_needed =
      total_connections + graph_->internal_edges().size();
  return num_partitions_ *
        (num_and_constraints_needed + num_or_constraints_needed);
}

void LpSolveInterface::GraphParsingState::ConstraintExToDeferredColumns(
    int count, const REAL* const constraint_coeffs,
    const int* const constraint_indices) {
  for (int i = 0; i < count; ++i) {
    // OK if this creates the entry or if it already exists.
    vector<pair<int, REAL>>& column_ex = columns_ex_[constraint_indices[i]];
    column_ex.push_back(
        make_pair(columns_ex_next_row_, constraint_coeffs[i]));
  }
  columns_ex_next_row_++;
}

void LpSolveInterface::GraphParsingState::ConstraintExToDeferredColumns(
    const vector<REAL>& constraint_coeffs,
    const vector<int>& constraint_indices) {
  ConstraintExToDeferredColumns(constraint_coeffs.size(),
                                constraint_coeffs.data(),
                                constraint_indices.data());
}

void LpSolveInterface::GraphParsingState::AddDeferredColumnsEx(lprec* model) {
  int num_cols_added = 0;
  for (const pair<int, vector<pair<int, REAL>>>& column_ex_pair : columns_ex_) {
    const vector<pair<int, REAL>>& column_ex = column_ex_pair.second;
    unique_ptr<int[]> indices(new int[column_ex.size()]);
    unique_ptr<REAL[]> coeffs(new REAL[column_ex.size()]);
    for (int i = 0; i < column_ex.size(); ++i) {
      indices[i] = column_ex[i].first;
      coeffs[i] = column_ex[i].second;
    }
    AddColumnEx(model, column_ex.size(), coeffs.get(), indices.get());
    if (verbose_) {
      ++num_cols_added;
      if (num_cols_added % kVerboseAddQuanta == 0) {
        cout << "Added " << num_cols_added << " columns." << endl;
      }
    }
  }
}

char LpSolveInterface::GraphParsingState::AddColumnEx(
    lprec* model, int count, const REAL* const coeffs,
    const int* const indices) {
  char ret = add_columnex(
      model, count, const_cast<REAL*>(coeffs), const_cast<int*>(indices));
  if (!ret) {
    throw LpSolveException("Failed adding column");
  }
  return ret;
}

char LpSolveInterface::GraphParsingState::AddColumnEx(
    lprec* model, const vector<REAL>& coeffs, const vector<int>& indices) {
  return AddColumnEx(model, coeffs.size(), coeffs.data(), indices.data());
}


char LpSolveInterface::GraphParsingState::AddConstraintEx(
    lprec* model, int count, const REAL* const coeffs, const int* const indices,
    int ctype, REAL rhs) {
  char ret = add_constraintex(model, count, const_cast<REAL*>(coeffs),
                              const_cast<int*>(indices), ctype, rhs);
  if (!ret) {
    throw LpSolveException("Failed adding constraint");
  }
  return ret;
}

char LpSolveInterface::GraphParsingState::AddConstraintEx(
    lprec* model, const std::vector<REAL>& coeffs,
    const std::vector<int>& indices, int ctype, REAL rhs) {
  return AddConstraintEx(model, coeffs.size(), coeffs.data(), indices.data(),
                         ctype, rhs);
}
