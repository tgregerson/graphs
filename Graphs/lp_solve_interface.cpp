#include "lp_solve_interface.h"

#include "chaco_parser.h"
#include "edge.h"
#include "node.h"
#include "processed_netlist_parser.h"

using namespace std;

void LpSolveInterface::LoadFromMps(const string& filename) {
  cout << "Reading MPS model from " << filename << "...";
  lprec* model = read_MPS(const_cast<char*>(filename.c_str()), NORMAL);
  if (model == nullptr) {
    throw LpSolveException("Could not read model from " + filename);
  }
  cout << "Done." << endl;
  cout << "Model contains " << get_Ncolumns(model) << " variables and "
       << get_Nrows(model) << " constraints." << endl;
  state_.reset(new LpSolveState(model));
}

void LpSolveInterface::LoadFromNtl(const string& filename) {
  ProcessedNetlistParser netlist_parser;
  Node graph;
  netlist_parser.Parse(&graph, filename.c_str(), nullptr);
  GraphParsingState gpstate(&graph);
  state_.reset(new LpSolveState(gpstate.ConstructModel()));
}

void LpSolveInterface::LoadFromChaco(const string& filename) {
  ChacoParser chaco_parser;
  Node graph;
  if (!chaco_parser.Parse(&graph, filename.c_str())) {
    throw LpSolveException("Error parsing CHACO graph from: " + filename);
  }
  GraphParsingState gpstate(&graph);
  state_.reset(new LpSolveState(gpstate.ConstructModel()));
}

void LpSolveInterface::WriteToLp(const string& filename) const {
  cout << "Writing LP model to " << filename << endl;
  if (state_.get() == nullptr) {
    throw LpSolveException("Trying to write LP without creating model.");
  }
  lprec* model = CHECK_NOTNULL(state_.get())->model();
  if (!write_lp(model, const_cast<char*>(filename.c_str()))) {
    throw LpSolveException("Could not write file " + filename);
  }
}

void LpSolveInterface::WriteToMps(const string& filename) const {
  cout << "Writing MPS model to " << filename << endl;
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
    Node* graph, double max_imbalance)
    : graph_(CHECK_NOTNULL(graph)),
      max_weight_imbalance_fraction_(max_imbalance) {
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
  AddImbalanceConstraintsToModel(model);
  int num_nodes_added = 0;
  int num_edges_added = 0;
  for (const auto& node_id_ptr_pair : graph_->internal_nodes()) {
    AddNodeToModel(model, *(node_id_ptr_pair.second));
    if (++num_nodes_added % 1000 == 0) {
      cout << "Added " << num_nodes_added << " nodes." << endl;
      cout << "Current Variables: " << get_Ncolumns(model) << endl;
      cout << "Current Constraints: " << get_Nrows(model) << endl;
    }
  }
  for (const auto& edge_id_ptr_pair : graph_->internal_edges()) {
    AddEdgeToModel(model, *(edge_id_ptr_pair.second));
    if (++num_edges_added % 1000 == 0) {
      cout << "Added " << num_edges_added << " edges." << endl;
      cout << "Current Variables: " << get_Ncolumns(model) << endl;
      cout << "Current Constraints: " << get_Nrows(model) << endl;
    }
  }
  cout << "Total Variables: " << get_Ncolumns(model) << endl;
  cout << "Total Constraints: " << get_Nrows(model) << endl;
  return model;
}

void LpSolveInterface::GraphParsingState::PreAllocateModelMemory(lprec* model) {
  // Estimate the number of variables and constraints.
  int num_nodes = graph_->internal_nodes().size();
  int num_edges = graph_->internal_edges().size();

  double avg_personalities_estimate = 3;  // Good for chaco, overkill for ntl.
  int node_variable_estimate =
      num_partitions_ * num_nodes * avg_personalities_estimate;

  int edge_variable_estimate =
      num_edges * (1 + num_partitions_);  // Should be exact.

  int variable_estimate = node_variable_estimate + edge_variable_estimate;

  int node_constraint_estimate = num_nodes;  // Should be exact.

  double avg_edge_degree_estimate = 2;  // Good for chaco, under for ntl.
  int edge_constraint_estimate =
      (1 + avg_edge_degree_estimate * avg_personalities_estimate *
       num_partitions_) * num_edges;

  int constraint_estimate = node_constraint_estimate + edge_constraint_estimate;

  cout << "Pre-allocating memory for " << constraint_estimate
       << " constraints and " << variable_estimate << " variables." << endl;

  if (!resize_lp(model, constraint_estimate, variable_estimate)) {
    throw LpSolveException("Failed to pre-allocate memory.");
  }
}

void LpSolveInterface::GraphParsingState::AddImbalanceConstraintsToModel(
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
  for (int par = 0; par < num_partitions_; ++par) {
    for (int per = 0; per < num_personalities; ++per) {
      vector<pair<int, REAL>> row_num_and_coeff;
      for (int res = 0; res < num_resources; ++res) {
        double imbalance_fraction = max_weight_imbalance_fraction_[res];
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

      variable_index_to_id_.insert(make_pair(new_variable_index, node.id));
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
