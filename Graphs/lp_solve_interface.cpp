#include "lp_solve_interface.h"

#include "chaco_parser.h"
#include "edge.h"
#include "node.h"
#include "processed_netlist_parser.h"

using namespace std;

void LpSolveInterface::LoadFromNtl(const string& filename) {
  ProcessedNetlistParser netlist_parser;
  Node graph;
  netlist_parser.Parse(&graph, filename.c_str(), nullptr);
  GraphParsingState gpstate(&graph);
  state_.reset(new LpSolveInterface::LpSolveState(gpstate.ConstructModel()));
}

void LpSolveInterface::LoadFromChaco(const string& filename) {
  ChacoParser chaco_parser;
  Node graph;
  if (!chaco_parser.Parse(&graph, filename.c_str())) {
    throw LpSolveException("Error parsing CHACO graph from: " + filename);
  }
  GraphParsingState gpstate(&graph);
  state_.reset(new LpSolveInterface::LpSolveState(gpstate.ConstructModel()));
}

LpSolveInterface::LpSolveState::LpSolveState() : model_(nullptr, delete_lp) {
  lprec* model = make_lp(0, 0);
  if (model == nullptr) {
    throw LpSolveException("Failed to create LPREC model.");
  }
  model_.reset(model);
}

lprec* LpSolveInterface::GraphParsingState::ConstructModel() {
  lprec* model = make_lp(0, 0);
  if (model == nullptr) {
    throw LpSolveException("Error creating empty model.");
  }

  SetObjectiveFunction(model);
  AddImbalanceConstraintsToModel(model);
  for (const auto& node_id_ptr_pair : graph_->internal_nodes()) {
    AddNodeToModel(model, *(node_id_ptr_pair.second));
  }
  for (const auto& edge_id_ptr_pair : graph_->internal_edges()) {
    AddEdgeToModel(model, *(edge_id_ptr_pair.second));
  }
  return model;
}

void LpSolveInterface::GraphParsingState::AddImbalanceConstraintsToModel(
    lprec* model) {
  // Imbalance constraints must be present prior to node constraints, so
  // require they are added first.
  assert (get_Nrows(model) == 0);
  assert (get_Ncolumns(model) == 0);

  // Need 2 * #resources constraints. They are added empty, since they will
  // be populated by adding nodes.
  for (size_t res = 0; res < (2 * max_weight_imbalance_fraction_.size());
       ++res) {
    if (add_constraint(model, nullptr, LE, 0.0) != 0) { 
      throw LpSolveException("Failed to add imbalance constraints.");
    }
  }
}

void LpSolveInterface::GraphParsingState::AddNodeToModel(
    lprec* model, const Node& node) {
  const int num_partitions = 2;

  int num_personalities = node.WeightVectors().size();
  int num_new_variables = num_partitions * num_personalities;
  assert(num_new_variables > 0);

  // Need to add #partitions * #personalities variables (columns).
  // Each variable is a unique <node, partition, personality> 3-tuple with a
  // binary value.
 
  // Need to add 1 constraint equation (row) to limit that only one
  // <node, partition, personality> can be selected per node. This can be done
  // using just one equation because the variables are restricted to binary.
 
  // Add empty row first, then columns, since the new row is all zero in the
  // pre-existing columns.
  if (add_constraintex(model, 0, nullptr, nullptr, EQ, 1.0) != 0) {
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

  // +1 because lp_solve ignores position 0 in its rows.
  int new_variable_index = get_Ncolumns(model) + 1;
  for (int par = 0; par < num_partitions; ++par) {
    for (int per = 0; per < num_personalities; ++per) {
      vector<pair<int, REAL>> row_num_and_coeff;
      // First access will create the empty entry.
      id_to_variable_indices_[node.id].push_back(vector<int>());
      for (int res = 0; res < num_resources; ++res) {
        int row_num_1 = 1 + 2 * res;
        REAL coeff_1 = (1.0 + max_weight_imbalance_fraction_[res]) *
                       node.WeightVector(per).at(res);
        row_num_and_coeff.push_back(make_pair(row_num_1, coeff_1));

        int row_num_2 = 2 + 2 * res;
        REAL coeff_2 = (1.0 + max_weight_imbalance_fraction_[res]) *
                       node.WeightVector(per).at(res);
        row_num_and_coeff.push_back(make_pair(row_num_2, coeff_2));

        row_num_and_coeff.push_back(make_pair(new_row_index, 1.0));
      }
      int count = row_num_and_coeff.size();
      int row_nums[count];
      REAL coeffs[count];

      for (int i = 0; i < count; ++i) {
        const pair<int, REAL>& rn_c = row_num_and_coeff[i];
        row_nums[i] = rn_c.first;
        coeffs[i] = rn_c.second;
      }

      variable_index_to_id_.insert(make_pair(new_variable_index, node.id));
      id_to_variable_indices_[node.id][par].push_back(new_variable_index);
      if (!add_columnex(model, count, coeffs, row_nums)) {
        throw LpSolveException("Error adding variable.");
      }
      ++new_variable_index;

      if (!set_binary(model, get_Ncolumns(model), TRUE)) {
        throw LpSolveException("Error setting variable to binary.");
      }
    }
  }
}

void LpSolveInterface::GraphParsingState::AddEdgeToModel(
    lprec* model, const Edge& edge) {
  const int num_partitions = 2;
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
  // E(n) - P_1(n) - P_2(n) - P_3(n) - P_4(n) + (#partitions - 1) => 0
  // E(n) - P_1(n) =< 0
  // E(n) - P_2(n) =< 0
  // E(n) - P_3(n) =< 0
  // E(n) - P_4(n) =< 0
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
  id_to_variable_indices_[edge.id].resize(num_partitions + 1);
  
  // Add edge crossing variable
  int edge_crossing_variable_index = new_variable_index;
  REAL objective_coefficient = (REAL)edge.weight;
  int row_num = 0;
  if (add_columnex(model, 1, &objective_coefficient, &row_num) != 0) {
    throw LpSolveException("Failed to add edge crossing variable.");
  }
  if (!set_binary(model, new_variable_index, TRUE)) {
    throw LpSolveException("Error setting variable to binary.");
  }
  id_to_variable_indices_[edge.id][num_partitions].push_back(
      new_variable_index++);

  // Add Partition connectivity variables
  for (int p = 0; p < num_partitions; ++p) {
    // These variables have no weight in existing constraints.
    if (add_columnex(model, 0, nullptr, nullptr) != 0) {
      throw LpSolveException("Failed to add partition connectivity variable.");
    }
    if (!set_binary(model, new_variable_index, TRUE)) {
      throw LpSolveException("Error setting variable to binary.");
    }
    id_to_variable_indices_[edge.id][p].push_back(new_variable_index++);
  }

  // Add the new constraints
  
  // AND constraints
  //
  // E(n) - P_1(n) - P_2(n) - P_3(n) - P_4(n) + (#partitions - 1) => 0
  // E(n) - P_1(n) =< 0
  // E(n) - P_2(n) =< 0
  // E(n) - P_3(n) =< 0
  // E(n) - P_4(n) =< 0
  REAL and_coeffs1[1 + num_partitions];
  int and_variable_indices1[1 + num_partitions];
  
  and_coeffs1[0] = 1.0;
  and_variable_indices1[0] = id_to_variable_indices_[edge.id][num_partitions][0];
  for (int i = 1; i <= num_partitions; ++i) {
    and_coeffs1[i] = -1.0;
    and_variable_indices1[0] = id_to_variable_indices_[edge.id][i][0];
  }
  if (add_constraintex(model, 1 + num_partitions, and_coeffs1,
                       and_variable_indices1, GE, 0.0) != 0) {
    throw LpSolveException("Failed to add AND constraint.");
  }

  for (int i = 0; i < num_partitions; ++i) {
    int and_variable_indices[2];
    REAL and_coeffs[2];
    and_variable_indices[0] = edge_crossing_variable_index;
    and_variable_indices[1] = id_to_variable_indices_.at(edge.id).at(i).at(0);
    and_coeffs[0] = 1.0;
    and_coeffs[1] = -1.0;
    if (add_constraintex(model, 2, and_coeffs,
                         and_variable_indices, LE, 0.0) != 0) {
      throw LpSolveException("Failed to add AND constraint.");
    }
  }

  // OR constraints
  //
  // P_k(n) - V1 - V2 - V3 =< 0
  // P_k(n) - V1 => 0
  // P_k(n) - V2 => 0
  // P_k(n) - V3 => 0
  //
  // Repeat for all partitions.
  for (int part = 0; part < num_partitions; ++part) {
    int partition_variable_index = id_to_variable_indices_[edge.id][part][0];
    vector<pair<int, REAL>> eq1_index_coeff_pairs;
    eq1_index_coeff_pairs.push_back(make_pair(partition_variable_index, 1.0));
    for (int node_id : edge.connection_ids) {
      Node* node = CHECK_NOTNULL(graph_->internal_nodes().at(node_id));
      for (size_t per = 0; per < node->WeightVectors().size(); ++per) {
        int variable_index =
          id_to_variable_indices_.at(node_id).at(part).at(per);
        REAL or_coeffs[2];
        int or_variable_indices[2];
        or_coeffs[0] = 1.0;
        or_coeffs[1] = -1.0;
        or_variable_indices[0] = partition_variable_index;
        or_variable_indices[1] = variable_index; 
        if (add_constraintex(
                model, 2, or_coeffs, or_variable_indices, GE, 0.0) != 0) {
          throw LpSolveException("Failed to add OR constraint.");
        }
      }
    }
    REAL or_coeffs1[eq1_index_coeff_pairs.size()];
    int or_variable_indices1[eq1_index_coeff_pairs.size()];
    for (size_t i = 0; i < eq1_index_coeff_pairs.size(); ++i) {
      or_variable_indices1[i] = eq1_index_coeff_pairs[i].first;
      or_coeffs1[i] = eq1_index_coeff_pairs[i].second;
    }
    if (add_constraintex(model, 2, or_coeffs1, or_variable_indices1, GE, 0.0) !=
                         0) {
      throw LpSolveException("Failed to add OR constraint.");
    }
  }
}

void LpSolveInterface::GraphParsingState::SetObjectiveFunction(lprec* model) {
  // Coefficients are set when adding edges. This just ensures fn is minimized.
  set_minim(model);
}
