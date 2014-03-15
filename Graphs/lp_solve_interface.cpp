#include "lp_solve_interface.h"

#include <cstdlib>

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
  state_.reset(new LpSolveState(gpstate.ConstructModelRowMode()));
}

void LpSolveInterface::LoadFromChaco(const string& filename) {
  ChacoParser chaco_parser;
  Node graph;
  if (!chaco_parser.Parse(&graph, filename.c_str())) {
    throw LpSolveException("Error parsing CHACO graph from: " + filename);
  }
  GraphParsingState gpstate(&graph, max_imbalance_, verbose_);
  state_.reset(new LpSolveState(gpstate.ConstructModelRowMode()));
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
      full_row_(nullptr, std::free){
  if (graph->internal_nodes().empty()) {
    throw LpSolveException("Provided empty graph.");
  }
  num_resources_ =
    CHECK_NOTNULL(graph->internal_nodes().begin()->second)->num_resources();
  if (num_resources_ <= 0) {
    throw LpSolveException("Weight vector has invalid number of resources");
  }
}

lprec* LpSolveInterface::GraphParsingState::ConstructModel() {
  lprec* model = make_lp(0, 0);
  if (model == nullptr) {
    throw LpSolveException("Error creating empty model.");
  }

  PreAllocateModelMemory(model);

  SetObjectiveFunction(model);
  AddEmptyImbalanceConstraintsToModel(model);
  int num_nodes_added = 0;
  int num_edges_added = 0;
  for (const auto& node_id_ptr_pair : graph_->internal_nodes()) {
    AddNodeToModel(model, *(node_id_ptr_pair.second));
    if (verbose_) {
      if (++num_nodes_added % 10000 == 0) {
        cout << "Added " << num_nodes_added << " nodes." << endl;
        cout << "Current Variables: " << get_Ncolumns(model) << endl;
        cout << "Current Constraints: " << get_Nrows(model) << endl;
      }
    }
  }
  for (const auto& edge_id_ptr_pair : graph_->internal_edges()) {
    AddEdgeToModel(model, *(edge_id_ptr_pair.second));
    if (verbose_) {
      if (++num_edges_added % 1000 == 0) {
        cout << "Added " << num_edges_added << " edges." << endl;
        cout << "Current Variables: " << get_Ncolumns(model) << endl;
        cout << "Current Constraints: " << get_Nrows(model) << endl;
      }
    }
  }
  if (verbose_) {
    cout << "Total Variables: " << get_Ncolumns(model) << endl;
    cout << "Total Constraints: " << get_Nrows(model) << endl;
  }
  return model;
}

lprec* LpSolveInterface::GraphParsingState::ConstructModelRowMode() {
  int num_variables = NumNodeVariablesNeeded() + NumEdgeVariablesNeeded();
  lprec* model = make_lp(0, num_variables);
  if (model == nullptr) {
    throw LpSolveException("Error creating model.");
  }
  PreAllocateModelMemory(model);
  AssignAllVariableIndices();

  set_add_rowmode(model, TRUE);
  SetObjectiveFunction(model);
  AddAllConstraintsToModel(model);
  set_add_rowmode(model, FALSE);

  SetAllVariablesBinary(model);

  if (verbose_) {
    cout << "Total Variables: " << get_Ncolumns(model) << endl;
    cout << "Total Constraints: " << get_Nrows(model) << endl;
  }
  return model;
}

void LpSolveInterface::GraphParsingState::PreAllocateModelMemory(lprec* model) {
  int num_variables = NumNodeVariablesNeeded() + NumEdgeVariablesNeeded();
  int num_constraints = NumNodeConstraintsNeeded() + NumEdgeConstraintsNeeded() +
                        (2 * num_resources_);
  if (verbose_) {
    cout << "Pre-allocating memory for " << num_constraints
         << " constraints and " << num_variables << " variables." << endl;
  }
  if (!resize_lp(model, num_constraints, num_variables)) {
    throw LpSolveException("Failed to pre-allocate memory.");
  }
}

void LpSolveInterface::GraphParsingState::AddEmptyImbalanceConstraintsToModel(
    lprec* model) {
  // Imbalance constraints must be present prior to node constraints, so
  // require they are added first.
  assert (get_Nrows(model) == 0);
  assert (get_Ncolumns(model) == 0);

  // Need 2 * #resources constraints. They are added empty, since they will
  // be populated by adding nodes.
  for (size_t res = 0; res < (2 * num_resources_);
       ++res) {
    if (!add_constraint(model, nullptr, GE, 0.0)) {
      throw LpSolveException("Failed to add imbalance constraints.");
    }
  }
}

void LpSolveInterface::GraphParsingState::AddNodeToModel(
    lprec* model, const Node& node) {
  CHECK_NOTNULL(model);

  int num_personalities = node.WeightVectors().size();
  int num_new_variables = num_partitions_ * num_personalities;
  assert(num_new_variables > 0);

  // Need to add #partitions * #personalities variables (columns).
  // Each variable is a unique <node, partition, personality> 3-tuple with a
  // binary value.
 
  // Need to add 1 constraint equation (row) to limit that only one
  // <node, partition, personality> can be selected per node. This can be done
  // using just one equation because the variables are restricted to binary.
 
  // Add empty row first, then columns, since the new row is all zero in the
  // pre-existing columns.
  if (!add_constraintex(model, 0, nullptr, nullptr, EQ, 1.0)) {
    throw LpSolveException("Failed to add empty row to model.");
  }
  int new_row_index = get_Nrows(model);
  int num_resources = node.num_resources();

  // The new columns should contain a 1 in the new row. Additionally, they must
  // add entries in the weight imbalance constraint rows. There are a pair of
  // constraints per resource, and the constraints start in row 1. The
  // coefficient for the first constraint in the pair is (I_n + 1)*W_n and the
  // coefficient for the second constraint is (I_n - 1)*W_n, where I_n is the
  // maximum allowed imbalance in resource n, expressed as a fraction of the
  // total weight in that resource, and W_n is the weight in that resource of
  // the given personality.

  // This algorithm only works for 2 partitions.
  assert(num_partitions_ == 2);

  // +1 because lp_solve ignores position 0 in its rows.
  int new_variable_index = get_Ncolumns(model) + 1;
  double imbalance_fraction = max_weight_imbalance_fraction_;
  for (int par = 0; par < num_partitions_; ++par) {
    for (int per = 0; per < num_personalities; ++per) {
      vector<pair<int, REAL>> row_num_and_coeff;
      for (int res = 0; res < num_resources; ++res) {
        int row_num_1 = 1 + 2 * res;
        double scaling1 = (par == 0) ? (imbalance_fraction + 1) :
                                       (imbalance_fraction - 1);
        REAL coeff_1 = scaling1 * ((REAL)node.WeightVector(per).at(res));
        row_num_and_coeff.push_back(make_pair(row_num_1, coeff_1));

        int row_num_2 = 2 + 2 * res;
        double scaling2 = (par == 0) ? (imbalance_fraction - 1) :
                                       (imbalance_fraction + 1);
        REAL coeff_2 = scaling2 * ((REAL)node.WeightVector(per).at(res));
        row_num_and_coeff.push_back(make_pair(row_num_2, coeff_2));
      }
      row_num_and_coeff.push_back(make_pair(new_row_index, 1.0));
      int count = row_num_and_coeff.size();
      int row_nums[count];
      REAL coeffs[count];

      for (int i = 0; i < count; ++i) {
        const pair<int, REAL>& rn_c = row_num_and_coeff.at(i);
        row_nums[i] = rn_c.first;
        coeffs[i] = rn_c.second;
      }

      SetNodeVariableIndex(node.id, par, per, new_variable_index);
      if (!add_columnex(model, count, coeffs, row_nums)) {
        throw LpSolveException("Error adding variable.");
      }
      if (!set_binary(model, new_variable_index, TRUE)) {
        throw LpSolveException("Error setting variable to binary.");
      }
      ++new_variable_index;
    }
  }
}

void LpSolveInterface::GraphParsingState::AddEdgeToModel(
    lprec* model, const Edge& edge) {
  CHECK_NOTNULL(model);
  // Adding an edge to the model potentially adds multiple variables and
  // constraints.
  //
  // The number of new variables is 1 + #partitions.
  // One of these variables is a Boolean variable indicating whether the edge
  // crosses a partition boundary. This variable has a coefficient of its
  // edge weight in the objective function, and a coefficient of 1 in each
  // of its constraint equations.
  //
  // The other variables are Boolean variables that indicate whether the edge
  // has any connected nodes in a given partition.
  //
  // The edge constraints are created from Boolean equations in the form:
  //
  // E(n) = P_1(n) && P_2(n) .... && P_#partitions(n)
  //
  // The AND operation can be decomposed into 1 + #partitions linear equations.
  // Shown here for #partitions = 4:
  // E(n) - P_1(n) - P_2(n) - P_3(n) - P_4(n) >= (1 - #partitions)
  // E(n) - P_1(n) <= 0
  // E(n) - P_2(n) <= 0
  // E(n) - P_3(n) <= 0
  // E(n) - P_4(n) <= 0
  //
  // P_k(n) = logical OR of the Boolean node variables in Partition k for nodes
  //          connected to the edge.
  //
  // The number of Boolean node variables in each logical OR is the summation of
  // the number of personalities for all nodes connected to the edge.
  //
  // The OR operation can be decomposed into 1 + #variables linear equations.
  // Shown here for #variables = 3:
  // P_k(n) - V1 - V2 - V3 =< 0
  // P_k(n) - V1 => 0
  // P_k(n) - V2 => 0
  // P_k(n) - V3 => 0
  //
  // A set of OR constraints must be added for each partition.
  
  // Add the new variables
  int new_variable_index = get_Ncolumns(model) + 1;
  
  // Add edge crossing variable
  int edge_crossing_variable_index = new_variable_index;
  REAL objective_coefficient = (REAL)edge.weight;
  int row_num = 0;
  if (!add_columnex(model, 1, &objective_coefficient, &row_num)) {
    throw LpSolveException("Failed to add edge crossing variable.");
  }
  if (!set_binary(model, new_variable_index, TRUE)) {
    throw LpSolveException("Error setting variable to binary.");
  }
  SetEdgeCrossingVariableIndex(edge.id, new_variable_index++);

  // Add Partition connectivity variables
  for (int p = 0; p < num_partitions_; ++p) {
    // These variables have no weight in existing constraints.
    if (!add_columnex(model, 0, nullptr, nullptr)) {
      throw LpSolveException("Failed to add partition connectivity variable.");
    }
    if (!set_binary(model, new_variable_index, TRUE)) {
      throw LpSolveException("Error setting variable to binary.");
    }
    SetEdgePartitionConnectivityVariableIndex(edge.id, p, new_variable_index++);
  }

  // Add the new constraints
  
  // AND constraints
  //
  // E(n) - P_1(n) - P_2(n) - P_3(n) - P_4(n) >= (1 - #partitions)
  // E(n) - P_1(n) <= 0
  // E(n) - P_2(n) <= 0
  // E(n) - P_3(n) <= 0
  // E(n) - P_4(n) <= 0
  REAL and_coeffs1[1 + num_partitions_];
  int and_variable_indices1[1 + num_partitions_];
  
  and_coeffs1[0] = 1.0;
  and_variable_indices1[0] = edge_crossing_variable_index;
  for (int i = 0; i < num_partitions_; ++i) {
    and_coeffs1[i + 1] = -1.0;
    and_variable_indices1[i + 1] =
        GetEdgePartitionConnectivityVariableIndex(edge.id, i);
  }
  if (!add_constraintex(
      model, 1 + num_partitions_, and_coeffs1, and_variable_indices1, GE,
      (REAL)(1 - num_partitions_))) {
    throw LpSolveException("Failed to add AND constraint.");
  }

  for (int i = 0; i < num_partitions_; ++i) {
    int and_variable_indices[2];
    REAL and_coeffs[2];
    and_variable_indices[0] = edge_crossing_variable_index;
    and_variable_indices[1] =
        GetEdgePartitionConnectivityVariableIndex(edge.id, i);
    and_coeffs[0] = 1.0;
    and_coeffs[1] = -1.0;
    if (!add_constraintex(model, 2, and_coeffs,
                          and_variable_indices, LE, 0.0)) {
      throw LpSolveException("Failed to add AND constraint.");
    }
  }

  // OR constraints
  //
  // P_k(n) - V1 - V2 - V3 <= 0
  // P_k(n) - V1 >= 0
  // P_k(n) - V2 >= 0
  // P_k(n) - V3 >= 0
  //
  // Repeat for all partitions.
  for (int part = 0; part < num_partitions_; ++part) {
    int partition_variable_index =
        GetEdgePartitionConnectivityVariableIndex(edge.id, part);
    vector<pair<int, REAL>> eq1_index_coeff_pairs;
    for (int node_id : edge.connection_ids) {
      Node* node = CHECK_NOTNULL(graph_->internal_nodes().at(node_id));
      for (size_t per = 0; per < node->WeightVectors().size(); ++per) {
        int node_variable_index = GetNodeVariableIndex(node_id, part, per);
        REAL or_coeffs[2];
        int or_variable_indices[2];
        or_coeffs[0] = 1.0;
        or_coeffs[1] = -1.0;
        or_variable_indices[0] = partition_variable_index;
        or_variable_indices[1] = node_variable_index;
        if (!add_constraintex(
                model, 2, or_coeffs, or_variable_indices, GE, 0.0)) {
          throw LpSolveException("Failed to add OR constraint.");
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
    if (!add_constraintex(
        model, num_vals, or_coeffs1, or_variable_indices1, LE, 0.0)) {
      throw LpSolveException("Failed to add OR constraint.");
    }
  }
}

void LpSolveInterface::GraphParsingState::SetObjectiveFunction(lprec* model) {
  // Coefficients are set when adding edges. This just ensures fn is minimized.
  set_minim(CHECK_NOTNULL(model));
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
    lprec* model) {
  AddImbalanceConstraintsToModel(model);
  AddAllNodeConstraintsToModel(model);
  AddAllEdgeConstraintsToModel(model);
}

void LpSolveInterface::GraphParsingState::AddAllNodeConstraintsToModel(
    lprec* model) {
  int num_nodes_added = 0;
  for (const pair<int, Node*>& p : graph_->internal_nodes()) {
    AddNodeConstraintsToModel(model, *(p.second));
    if (verbose_) {
      ++num_nodes_added;
      if (num_nodes_added % 10000 == 0) {
        cout << "Added constraints for " << num_nodes_added << " nodes." << endl;
      }
    }
  }
}

void LpSolveInterface::GraphParsingState::AddAllEdgeConstraintsToModel(
    lprec* model) {
  int num_edges_added = 0;
  for (const pair<int, Edge*>& p : graph_->internal_edges()) {
    AddEdgeConstraintsToModel(model, *(p.second));
    if (verbose_) {
      ++num_edges_added;
      if (num_edges_added % 1000 == 0) {
        cout << "Added constraints for " << num_edges_added << " edges." << endl;
      }
    }
  }
}

void LpSolveInterface::GraphParsingState::AddImbalanceConstraintsToModel(
    lprec* model) {
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
          if (res != 0) {
            variable_indices[res].push_back(nid_mapping.second[part][per]);
            coefficients[res].push_back(make_pair(
                (REAL)(weight * (max_weight_imbalance_fraction_ + 1)),
                (REAL)(weight * (max_weight_imbalance_fraction_ - 1))));

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
    if (!add_constraintex(model, num_nonzero, gz_coeffs.get(), indices.get(),
                          GE, 0.0) ||
        !add_constraintex(model, num_nonzero, lz_coeffs.get(), indices.get(),
                          GE, 0.0)) {
      throw LpSolveException("Failed to imbalance constraint to model.");
    }
  }
}

void LpSolveInterface::GraphParsingState::AddNodeConstraintsToModel(
    lprec* model, const Node& node) {
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
  //if (!add_constraintex(model, num_nonzero, coeffs, indices , LE, 1.0)) {
  if (!AddConstraintEx(model, num_nonzero, coeffs, indices, LE, 1.0)) {
    throw LpSolveException("Failed to add empty row to model.");
  }
}

void LpSolveInterface::GraphParsingState::AddEdgeConstraintsToModel(
    lprec* model, const Edge& edge) {
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
  //if (!add_constraintex(
  if (!AddConstraintEx(
      model, 1 + num_partitions_, and_coeffs1, and_variable_indices1, GE,
      (REAL)(1 - num_partitions_))) {
    throw LpSolveException("Failed to add AND constraint.");
  }

  for (int i = 0; i < num_partitions_; ++i) {
    int and_variable_indices[2];
    REAL and_coeffs[2];
    and_variable_indices[0] = edge_crossing_variable_index;
    and_variable_indices[1] =
        GetEdgePartitionConnectivityVariableIndex(edge.id, i);
    and_coeffs[0] = 1.0;
    and_coeffs[1] = -1.0;
    //if (!add_constraintex(model, 2, and_coeffs,
    if (!AddConstraintEx(model, 2, and_coeffs,
                          and_variable_indices, LE, 0.0)) {
      throw LpSolveException("Failed to add AND constraint.");
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
    for (int node_id : edge.connection_ids) {
      Node* node = CHECK_NOTNULL(graph_->internal_nodes().at(node_id));
      for (size_t per = 0; per < node->WeightVectors().size(); ++per) {
        int node_variable_index = GetNodeVariableIndex(node_id, part, per);
        REAL or_coeffs[2];
        int or_variable_indices[2];
        or_coeffs[0] = 1.0;
        or_coeffs[1] = -1.0;
        or_variable_indices[0] = partition_variable_index;
        or_variable_indices[1] = node_variable_index;
        //if (!add_constraintex(
        if (!AddConstraintEx(
                model, 2, or_coeffs, or_variable_indices, GE, 0.0)) {
          throw LpSolveException("Failed to add OR constraint.");
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
    //if (!add_constraintex(
    if (!AddConstraintEx(
        model, num_vals, or_coeffs1, or_variable_indices1, LE, 0.0)) {
      throw LpSolveException("Failed to add OR constraint.");
    }
  }
}

void LpSolveInterface::GraphParsingState::SetAllVariablesBinary(
    lprec* model) {
  for (int i = 1; i < get_Ncolumns(model); ++i) {
    if (!set_binary(model, i, TRUE)) {
      throw LpSolveException("Error setting variable to binary.");
    }
  }
}

int LpSolveInterface::GraphParsingState::NumNodeVariablesNeeded() {
  int total_personalities = 0;
  for (const pair<int, Node*>& p : graph_->internal_nodes()) {
    total_personalities += p.second->num_personalities();
  }
  return total_personalities * num_partitions_;
}

int LpSolveInterface::GraphParsingState::NumEdgeVariablesNeeded() {
  return (1 + num_partitions_) * graph_->internal_edges().size();
}

int LpSolveInterface::GraphParsingState::NumNodeConstraintsNeeded() {
  return graph_->internal_nodes().size();
}

int LpSolveInterface::GraphParsingState::NumEdgeConstraintsNeeded() {
  return (1 + num_partitions_) * graph_->internal_edges().size();
  int total_connections = 0;
  for (const pair<int, Edge*>& p : graph_->internal_edges()) {
    total_connections += p.second->degree();
  }
  int num_and_constraints_needed =
      (1 + num_partitions_) * graph_->internal_edges().size();
  int num_or_constraints_needed =
      total_connections + graph_->internal_edges().size();
  return num_and_constraints_needed + num_or_constraints_needed;
}

char LpSolveInterface::GraphParsingState::AddConstraintEx(
    lprec* model, int count, REAL* coeffs, int* indices, int ctype, REAL rhs) {
  char ret;
  if (full_row_.get() == nullptr) {
    int num_variables = NumNodeVariablesNeeded() + NumEdgeVariablesNeeded();
    full_row_.reset((REAL*)(calloc(sizeof(int), num_variables + 1)));
  }
  REAL* row = full_row_.get();
  for (int i = 0; i < count; ++i) {
    row[indices[i]] = coeffs[i];
  }
  ret = add_constraint(model, row, ctype, rhs);
  // Restore zeroes.
  for (int i = 0; i < count; ++i) {
    row[indices[i]] = 0;
  }
  return ret;
}
