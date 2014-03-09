#include "lp_solve_interface.h"

#include "chaco_parser.h"
#include "processed_netlist_parser.h"
#include "universal_macros.h"

using namespace std;

void LpSolveInterface::LoadFromNtl(const string& filename) {
  ProcessedNetlistParser netlist_parser;
  netlist_parser.Parse(&graph_, filename.c_str(), nullptr);
  state_.reset(new LpSolveInterface::LpSolveState(ConstructModelFromGraph()));
}

void LpSolveInterface::LoadFromChaco(const string& filename) {
  ChacoParser chaco_parser;
  if (!chaco_parser.Parse(&graph_, filename.c_str())) {
    throw LpSolveException("Error parsing CHACO graph from: " + filename);
  }
  state_.reset(new LpSolveInterface::LpSolveState(ConstructModelFromGraph()));
}

lprec* LpSolveInterface::ConstructModelFromGraph() {
  lprec* model = make_lp(0, 0);
  if (model == nullptr) {
    throw LpSolveException("Error creating empty model.");
  }

  // TODO Implement Objective function.
  AddImbalanceConstraints(model);
  for (const auto& node_id_ptr_pair : graph_.internal_nodes()) {
    AddNodeConstraint(model, *(node_id_ptr_pair.second));
  }
  return nullptr;
}

void LpSolveInterface::AddImbalanceConstraints(lprec* model) {
  // Need 2 * #resources constraints. We assume that these are the first
  // constraints added to the model.
  assert (get_Nrows(model) == 0);
  assert (get_Ncolumns(model) == 0);
  
  for (size_t res = 0; res < (2 * max_weight_imbalance_fraction_.size());
       ++res) {
    if (add_constraint(model, nullptr, GE, 0.0) != 0) { 
      throw LpSolveException("Failed to add imbalance constraints.");
    }
  }
}

void LpSolveInterface::AddNodeConstraint(lprec* model, const Node& node) {
  const int num_partitions = 2;

  pair<int, int> column_index_and_length;
  int starting_variable_index = get_Ncolumns(model) + 1;
  column_index_and_length.first = starting_variable_index;

  int num_personalities = node.WeightVectors().size();
  int num_new_variables = num_partitions * num_personalities;
  assert(num_new_variables > 0);
  column_index_and_length.second = num_new_variables;

  // Need to add #partitions * #personalities variables (columns).
  // Each variable is a unique <node, partition, personality> 3-tuple with a
  // binary value.
 
  // Need to add 1 additional equation (row) to limit that only one
  // <node, partition, personality> can be selected per node. This can be done
  // using just one equation because the variables are restricted to binary.
 
  // Add empty row first, then columns, since the new row is all zero in the
  // pre-existing columns.
  if (add_constraintex(model, 0, nullptr, nullptr, LE, 1.0) != 0) {
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

  for (int par = 0; par < num_partitions; ++par) {
    for (int per = 0; per < num_personalities; ++per) {
      vector<pair<int, REAL>> row_num_and_coeff;
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
      int* row_nums = (int*)malloc(count * sizeof(int));
      REAL* coeffs = (REAL*)malloc(count * sizeof(REAL));

      if (!add_columnex(model, count, coeffs, row_nums)) {
        throw LpSolveException("Error adding variable.");
      }

      if (!set_binary(model, get_Ncolumns(model), TRUE)) {
        throw LpSolveException("Error setting variable to binary.");
      }
    }
  }
}

void LpSolveInterface::AddObjectiveFunction(lprec* model) {
  // TODO implement
}

LpSolveInterface::LpSolveState::LpSolveState() : model_(nullptr, delete_lp) {
  lprec* model = make_lp(0, 0);
  if (model == nullptr) {
    throw LpSolveException("Failed to create LPREC model.");
  }
  model_.reset(model);
}
