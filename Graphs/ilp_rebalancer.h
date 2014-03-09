#ifndef ILP_REBALANCER_H_
#define ILP_REBALANCER_H_

void PartitionEngineKlfm::RebalanceImplementations(
    const NodePartitions& current_partition,
    std::vector<int>* partition_imbalance) {
  using namespace std;

  // SCALABILITY! Number of matrix entries will be I*N^2, where
  //    I: avg number of implementations per node
  //    N: number of nodes
  // For N > 10,000 this will probably lead to memory issues.
  // Therefore we may want to try fracturing the partitions into chunks
  // of 10K or less and running on each chunk.

  /* We will construct a set of linear equations as follows:

     1 Constraint for each node.
     This constraint will have binary variables representing each of its
     possible implementations. The constraint will require that exactly one
     implementation is selected per node.

     2 Constraints for each resource type. These will ensure that the
     balance constraint is met. Two constraints are necessary because the
     balance constraint is an absolute value.
     The format of each of these constraints is for the coefficient of each
     variable that represents a node implementation to be set to the weight
     of the related resource in that implementation. The coefficient for nodes
     in partition A is then multiplied by (1 - max_imbalance_fraction) and
     in partition B by (-1 - max_imbalance_fraction) for the first constraint.
     For the second constraint, the multipliers are reversed. Each constraint
     is set as <= 0.

     Objective function: This can be a variety of things.
       Ideas: Sum of scaled weights
              Sum of scaled resource weight differences between partitions.

     Need one variable per implementation.
  */

  // These store all the node IDs from each partition.
  // Move them into a separate vector just to be sure about the order they
  // appear in.
  vector<int> partition_a_nodes, partition_b_nodes;
  for (auto node_id : current_partition.first) {
    partition_a_nodes.push_back(node_id);
  }
  for (auto node_id : current_partition.second) {
    partition_b_nodes.push_back(node_id);
  }

  // These are the implementations of all of the nodes.
  // The outer vector corresponds to one entry per node.
  // The inner vector corresponds to one entry per weight vector (in the node).
  // Each entry is a unique weight vector ID.
  vector<vector<int>> partition_a_node_implementations;
  vector<vector<int>> partition_b_node_implementations;

  // This maps the IDs from the above vectors to the weight vectors
  // they represent.
  map<int,vector<int>> implementation_id_map;

  // Populate implementaion IDs.
  int implementation_id = 0;
  for (auto node_id : partition_a_nodes) {
    vector<int> node_wv_ids;
    Node* node = internal_node_map_.at(node_id);
    for (auto wv : node->WeightVectors()) {
      node_wv_ids.push_back(implementation_id);
      implementation_id_map.insert(make_pair(implementation_id, wv));
      implementation_number++;
    }
    partition_a_node_implementations.push_back(node_wvs);
  }
  for (auto node_id : partition_b_nodes) {
    vector<int> node_wv_ids;
    Node* node = internal_node_map_.at(node_id);
    for (auto wv : node->WeightVectors()) {
      node_wv_ids.push_back(implementation_id);
      implementation_id_map.insert(make_pair(implementation_id, wv));
      implementation_number++;
    }
    partition_b_node_implementations.push_back(node_wvs);
  }

  // Create lp_solver
  int number_of_variables = implementation_id_map.size();
  lprec* lp = make_lp(0, number_of_variables);

  // Set all variables to binary (note lp_solver starts numbers at 1)
  for (int var = 1; var <= number_of_variables; var++) {
    set_binary(lp, var, true);
  }

  // These will be the rows for implementing the resource balance constraints.
  // Two rows per resource.
  vector<vector<double>> balance_constraint_rows;
  balance_constraint_rows.resize(num_resources_per_node_ * 2);
  for (auto& it : balance_constraint_rows) {
    it.resize(number_of_variables, 0.0);
  }

  // Generate the 'choose one implementation' constraints.
  for (auto& node_implementations : partition_a_implementations) {
    // Put a '1' in the variable column for this node's implementations and
    // '0' in all other columns
    vector<double> constraint_row(number_of_variables, 0.0);
    assert(node_implementations.size() > 0);
    for (auto impl_id : node_implementations) {
      constraint_row[impl_id] = 1.0;
      // Add the implementations weighting to the balance constraints.
      auto& wv = implementation_id_map.at(impl_id);
      for (int res = 0; res < num_resourcess_per_node_; res++) {
        if (wv[res]) {
          balance_constraint_rows[(2*res)][impl_id] =
              (1.0 - max_imbalance_fraction_[res])*((double)wv[res]);
          balance_constraint_rows[(2*res + 1)][impl_id] =
              (-1.0 - max_imbalance_fraction_[res])*((double)wv[res]);
        }
      }
    }
    add_constraint(lp, &constraint_row, EQ, 1.0);
  }
  for (auto& node_implementations : partition_b_implementations) {
    vector<double> constraint_row(number_of_variables, 0.0);
    assert(node_implementations.size() > 0);
    for (auto impl_id : node_implementations) {
      constraint_row[impl_id] = 1.0;
      auto& wv = implementation_id_map.at(impl_id);
      for (int res = 0; res < num_resourcess_per_node_; res++) {
        if (wv[res]) {
          // Note that the order is switched for Partition B.
          balance_constraint_rows[(2*res)][impl_id] =
              (-1.0 - max_imbalance_fraction_[res])*((double)wv[res]);
          balance_constraint_rows[(2*res + 1)][impl_id] =
              (1.0 - max_imbalance_fraction_[res])*((double)wv[res]);
        }
      }
    }
    add_constraint(lp, &constraint_row, EQ, 1.0);
  }

  // Add the balance constraint rows.
  for (int i = 0; i < balance_constraint_rows.size(); i++) {
    add_constraint(lp, &(balance_constraint_rows[i]), LE, 0.0);
  }

  // Add the objective function.

}

#endif /* ILP_REBALANCER_H_ */
