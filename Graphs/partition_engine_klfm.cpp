#include "partition_engine_klfm.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <limits>
#include <queue>
#include <sstream>
#include <utility>

#include "gain_bucket_manager_single_resource.h"
#include "gain_bucket_manager_multi_resource_exclusive.h"
#include "gain_bucket_manager_multi_resource_mixed.h"
#include "id_manager.h"
#include "mps_name_hash.h"
#include "universal_macros.h"
#include "weight_score.h"

using namespace std;

PartitionEngineKlfm::PartitionEngineKlfm(Node* graph,
    PartitionEngineKlfm::Options& options, ostream& os)
  : options_(options), os_(os), balance_exceeded_(false) {

  //random_engine_.seed(time(NULL));
  // Give the same seed each time for consistency between benchmarks. The
  // randomization of initial partition from run-to-run will still be different,
  // however for run 0 of one configuration and run 0 of another, they will be
  // the same.
  random_engine_initial_.seed(0);
  random_engine_rebalance_.seed(0);
  random_engine_mutate_.seed(0);
  random_engine_coarsen_.seed(0);

  graph->CheckInternalGraphOrDie();

  num_resources_per_node_ = options_.num_resources_per_node;
  total_capacity_ = options_.device_resource_capacities;
  assert_b(
      options_.num_resources_per_node ==
      graph->internal_nodes().begin()->second->SelectedWeightVector().size()) {
    printf("Number of resources specified in Partition Engine options does "
           "not match the number of resources in the graph.");
  }
  total_weight_.insert(total_weight_.begin(), num_resources_per_node_, 0);

  // Remove ports.
  // This is done for simplification while developing the KLFM algorithm.
  // Later may want to restore the concept of ports to consider partitioning
  // of bandwidth.
  //
  // TODO: Currently the algorithm doesn't work properly for hypergraphs
  // without stripping ports.
  graph->StripPorts();

  // Populate internal data structures
  for (auto node_pair : graph->internal_nodes()) {
    Node* copied_node = new Node(node_pair.second);
    copied_node->is_locked = false;
    internal_node_map_.insert(make_pair(copied_node->id, copied_node));
  }
  for (auto edge_pair : graph->internal_edges()) {
    assert(!edge_pair.second->name.empty());
    EdgeKlfm* copied_edge = new EdgeKlfm(edge_pair.second);
    assert(edge_pair.second->name == copied_edge->name);
    internal_edge_map_.insert(make_pair(copied_edge->id, copied_edge));
  }

  // Check that all nodes have the correct number of resources in their weight
  // vectors.
  CheckSizeOfWeightVectors();

  RecomputeTotalWeightAndMaxImbalance();

  switch (options_.gain_bucket_type) {
    case PartitionerConfig::kGainBucketSingleResource:
      DLOG(DEBUG_OPT_TRACE, 0) <<
          "Creating SINGLE RESOURCE gain bucket manager." << endl;
      gain_bucket_manager_ = new GainBucketManagerSingleResource(
          0, options_.max_imbalance_fraction[0]);
    break;
    case PartitionerConfig::kGainBucketMultiResourceExclusive:
      DLOG(DEBUG_OPT_TRACE, 0) <<
          "Creating MULTI RESOURCE EXCLUSIVE gain bucket manager." << endl;
      gain_bucket_manager_ = new GainBucketManagerMultiResourceExclusive(
          options_.max_imbalance_fraction,
          options_.gain_bucket_selection_policy,
          false);
    break;
    case PartitionerConfig::kGainBucketMultiResourceExclusiveAdaptive:
      DLOG(DEBUG_OPT_TRACE, 0) <<
          "Creating MULTI RESOURCE EXCLUSIVE ADAPTIVE gain bucket manager." <<
          endl;
      gain_bucket_manager_ = new GainBucketManagerMultiResourceExclusive(
          options_.max_imbalance_fraction,
          options_.gain_bucket_selection_policy,
          true);
    break;
    case PartitionerConfig::kGainBucketMultiResourceMixed:
      DLOG(DEBUG_OPT_TRACE, 0) <<
          "Creating MULTI RESOURCE MIXED gain bucket manager." << endl;
      gain_bucket_manager_ = new GainBucketManagerMultiResourceMixed(
          options_.max_imbalance_fraction,
          options_.gain_bucket_selection_policy,
          false,
          options_.use_ratio_in_imbalance_score,
          options_.resource_ratio_weights);
    break;
    case PartitionerConfig::kGainBucketMultiResourceMixedAdaptive:
      DLOG(DEBUG_OPT_TRACE, 0) <<
          "Creating MULTI RESOURCE MIXED ADAPTIVE gain bucket manager." << endl;
      gain_bucket_manager_ = new GainBucketManagerMultiResourceMixed(
          options_.max_imbalance_fraction,
          options_.gain_bucket_selection_policy,
          true,
          options_.use_ratio_in_imbalance_score,
          options_.resource_ratio_weights);
    break;
    default:
      assert_b(false) {
        printf("\nOptions specify an unsupported gain bucket type.\n");
      }
  }

  // Verify that all nodes fall within the limits of the weight imbalance.
  bool skip = false;
  for (auto node_pair : internal_node_map_) {
    vector<int> node_weight = node_pair.second->SelectedWeightVector();
    for (int i = 0; i < num_resources_per_node_; i++) {
      if (node_weight[i] >= 2 * max_weight_imbalance_[i]) {
        printf("WARNING: Node %s with weight %d exceeded the max weight allowance: %d "
               "in resource %d.\n",
               node_pair.second->name.c_str(), node_weight[i],
               2 * max_weight_imbalance_[i], i);
        printf("Supressing future warnings of this type for this run.\n");
        skip = true;
        break;
      }
    }
    if (skip) {
      break;
    }
  }

}

PartitionEngineKlfm::~PartitionEngineKlfm() {
  for (auto it : internal_node_map_) {
    delete it.second;
  }
  internal_node_map_.clear();
  for (auto it : internal_edge_map_) {
    delete it.second;
  }
  internal_edge_map_.clear();
  delete gain_bucket_manager_;
}

void PartitionEngineKlfm::Execute(vector<PartitionSummary>* summaries) {

  // **KLFM Algorithm**
  // Generate initial partitioning
  // WHILE PASSES LEFT
  // --Unlock all nodes
  // --Compute initial gain of each node
  // --Set edge criticality - Is this necessary, or could it be done on-demand
  //                        - for only edges touching a moved node?
  // --WHILE NODES UNLOCKED
  // ----Get node from gain bucket (highest gain based on weight constraints)
  // ----Move highest gain node and update partition balance.
  // ----Update the gains of all nodes touching critical edges on moved node
  // ----Update criticality of all edges touching moved node
  // ----If total cost is minimized, save current partition
  // --IF No partition saved (initial is still best) THEN TERMINATE
  // --ELSE Set saved partition as new initial partition.
  DLOG(DEBUG_OPT_TRACE, 0) << "Start KLFM Execution." << endl;
  map<int,int> initial_implementations;
  if (!options_.reuse_previous_run_implementations) {
    StoreInitialImplementations(&initial_implementations);
  }

  for (int cur_run = 0; cur_run < options_.num_runs; cur_run++) {
    vector<PartitionSummary> this_run_summaries;
    if (!options_.reuse_previous_run_implementations && cur_run != 0) {
      ResetImplementations(initial_implementations);
      RecomputeTotalWeightAndMaxImbalance();
    }

    if (!options_.initial_sol_base_filename.empty()) {
      NodePartitions pre_run_partitions;
      int temp_cost;
      vector<int> temp_balance;
      GenerateInitialPartition(&pre_run_partitions, &temp_cost, &temp_balance);
      if (options_.sol_scip_format) {
        WriteScipSolAlt(pre_run_partitions, options_.initial_sol_base_filename);
      }
      if (options_.sol_gurobi_format) {
        WriteGurobiMst(pre_run_partitions, options_.initial_sol_base_filename);
      }
      if (options_.export_initial_sol_only) {
        exit(0);
      }
    }

    VLOG(1) << endl << "########### Begin Run " << cur_run + 1 << "/"
            << options_.num_runs << " ###########" << endl;

    ExecuteRun(cur_run, &this_run_summaries);
    for (const auto& it : this_run_summaries) {
      if (options_.enable_print_output) {
        PrintResultFull(it, cur_run);
      }
      summaries->push_back(it);
    }
  }
  if (options_.enable_print_output) {
    SummarizeResults(*summaries);
  }
}

void PartitionEngineKlfm::CheckSizeOfWeightVectors() {
  for (auto node_pair : internal_node_map_) {
    for (int i = 0; i < node_pair.second->WeightVectors().size(); ++i) {
      assert_b(num_resources_per_node_ ==
               node_pair.second->WeightVector(i).size()) {
        os_  << "Found invalidly sized weight vector in node '"
             << node_pair.second->name << "'. Expected "
             << num_resources_per_node_ << " Found "
             << node_pair.second->WeightVector(i).size();
      }
    }
  }
}

void PartitionEngineKlfm::StoreInitialImplementations(
    map<int,int>* initial_implementations) const {
  DLOG(DEBUG_OPT_TRACE, 0) << "Storing initial implementations." << endl;
  initial_implementations->clear();
  for (auto node_pair : internal_node_map_) {
    initial_implementations->insert(make_pair(
        node_pair.first, node_pair.second->selected_weight_vector_index()));
  }
}

void PartitionEngineKlfm::ResetImplementations(
    const map<int,int>& initial_implementations) {
  DLOG(DEBUG_OPT_TRACE, 1) << "Restoring initial implementations." << endl;
  for (auto ii_pair : initial_implementations) {
    auto np_it = internal_node_map_.find(ii_pair.first);
    assert(np_it != internal_node_map_.end());
    np_it->second->SetSelectedWeightVector(ii_pair.second);
  }
}

void PartitionEngineKlfm::ExecuteRun(
    int cur_run, vector<PartitionSummary>* summaries) {
  rebalances_this_run_ = 0;
  NodePartitions coarsened_partition;
  int current_partition_cost;
  // Balance is the difference in weight between the partitions. It is
  // positive if Partition A has a higher weight.
  vector<int> current_partition_balance;

  if (options_.enable_mutation && options_.mutation_rate > 0) {
    DLOG(DEBUG_OPT_TRACE, 1) << "Mutating implementations." << endl;
    MutateImplementations(options_.mutation_rate);
  }

  DLOG(DEBUG_OPT_TRACE, 1) << "Coarsening graph." << endl;
  options_.cap_passes = true;
  options_.max_passes = 30;
  //CoarsenSimple(4);
  //CoarsenMaxNodeDegree(4);
  // TODO Make a parameter for this.
  int pre_coarsen_size = internal_node_map_.size();
  //CoarsenNeighborhoodInterconnection(16, 0);
  CoarsenHierarchalInterconnection(16, 100);
  VLOG(1) << "Coarsened from " << pre_coarsen_size << " to "
          << internal_node_map_.size() << " nodes." << endl;

  GenerateInitialPartition(&coarsened_partition, &current_partition_cost,
      &current_partition_balance);

  if (options_.use_multilevel_constraint_relaxation) {
    for (int i = 1; i < num_resources_per_node_; i++) {
      options_.constrain_balance_by_resource[i] = false;
    }
    RecomputeTotalWeightAndMaxImbalance();
  }

  RUN_DEBUG(DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR, 0) {
    for (auto node_pair : internal_node_map_) {
      node_pair.second->CheckSupernodeWeightVectorOrDie();
    }
  }
  RUN_DEBUG(DEBUG_OPT_COST_CHECK, 0) {
    assert(current_partition_cost == RecomputeCurrentCost());
  }
  RUN_DEBUG(DEBUG_OPT_BALANCE_CHECK, 0) {
    vector<int> rec_balance = RecomputeCurrentBalance(coarsened_partition);
    assert(current_partition_balance == rec_balance);
    rec_balance = RecomputeCurrentBalanceAtBaseLevel(coarsened_partition);
    assert(current_partition_balance == rec_balance);
  }

  VLOG(1) << "Initial Partition Cost: " << current_partition_cost << endl;

  // Execute coarse partitioning.
  int num_passes = RunKlfmAlgorithm(
      cur_run, coarsened_partition, current_partition_cost,
      current_partition_balance);
  
  DLOG(DEBUG_OPT_TRACE, 1) << "De-Coarsening graph." << endl;
  options_.max_passes = 30;
  NodePartitions decoarsened_partition;
  // TODO Factor to helper fn.
  for (auto node_id : coarsened_partition.first) {
    Node* node = internal_node_map_.at(node_id);
    if (node->is_supernode()) {
      RUN_DEBUG(DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR, 0) {
        node->CheckSupernodeWeightVectorOrDie();
      }
      for (auto internal_node_pair : node->internal_nodes()) {
        decoarsened_partition.first.insert(internal_node_pair.first);
      }
    } else {
      decoarsened_partition.first.insert(node_id);
    }
  }
  coarsened_partition.first.clear();
  for (auto node_id : coarsened_partition.second) {
    Node* node = internal_node_map_.at(node_id);
    if (node->is_supernode()) {
      RUN_DEBUG(DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR, 0) {
        node->CheckSupernodeWeightVectorOrDie();
      }
      for (auto internal_node_pair : node->internal_nodes()) {
        decoarsened_partition.second.insert(internal_node_pair.first);
      }
    } else {
      decoarsened_partition.second.insert(node_id);
    }
  }
  coarsened_partition.second.clear();
  DeCoarsen();

  PopulateEdgePartitionConnections(decoarsened_partition);

  RUN_DEBUG(DEBUG_OPT_COST_CHECK, 0) {
    assert(current_partition_cost == RecomputeCurrentCost());
  }
  RUN_DEBUG(DEBUG_OPT_BALANCE_CHECK, 0) {
    vector<int> rec_balance = RecomputeCurrentBalance(decoarsened_partition);
    assert(current_partition_balance == rec_balance);
  }

  if (options_.use_multilevel_constraint_relaxation) {
    for (int i = 1; i < num_resources_per_node_; i++) {
      options_.constrain_balance_by_resource[i] = true;
    }
    RecomputeTotalWeightAndMaxImbalance();
    // Rebalance using ratio first, then balance.
    RebalanceImplementations(
        decoarsened_partition, current_partition_balance, false, true);
    RebalanceImplementations(
        decoarsened_partition, current_partition_balance, true, true);
  }


  num_passes += RunKlfmAlgorithm(
      cur_run, decoarsened_partition, current_partition_cost,
      current_partition_balance);

  if (!options_.final_sol_base_filename.empty()) {
    if (options_.sol_scip_format) {
      WriteScipSolAlt(decoarsened_partition, options_.final_sol_base_filename);
    }
    if (options_.sol_gurobi_format) {
      WriteGurobiMst(decoarsened_partition, options_.final_sol_base_filename);
    }
  }
  RUN_DEBUG(DEBUG_OPT_COST_CHECK, 0) {
    assert(current_partition_cost == RecomputeCurrentCost());
  }
  RUN_DEBUG(DEBUG_OPT_BALANCE_CHECK, 0) {
    assert(current_partition_balance ==
           RecomputeCurrentBalance(decoarsened_partition));
  }

  int num_summaries = 3;

  for (int sum_num = 0; sum_num < num_summaries; sum_num++) {
    if (sum_num == 1) {
      if (ExceedsMaxWeightImbalance(current_partition_balance)) {
        os_ << "Attempting to rebalance violater partition."
            << endl;
        for (int i = 0; i < num_resources_per_node_; i++) {
          options_.constrain_balance_by_resource[i] = true;
        }
        RecomputeTotalWeightAndMaxImbalance();
        // TODO It appears that this operation is adjusting for ratio even
        // though it shouldn't.
        RebalanceImplementations(
            decoarsened_partition, current_partition_balance, true, false);
      } else {
        continue;
      }
    } else if (sum_num == 2) {
      os_ << "Attempting to rebalance finished partitions. (ratio-only)"
          << endl;
      // This is a technique to avoid the problem that resources which have
      // been completely eliminated are unable to be used.
      bool need_mutate = false;
      for (int i = 0; i < num_resources_per_node_; i++) {
        if (total_weight_[i] == 0) {
          need_mutate = true;
        }
      }
      if (need_mutate) {
        os_ << "Mutating implementations." << endl;
        MutateImplementations(100);
      }
      if (!ExceedsMaxWeightImbalance(current_partition_balance)) {
        RebalanceImplementations(
            decoarsened_partition, current_partition_balance, false, true);
      } else {
        RebalanceImplementations(
            decoarsened_partition, current_partition_balance, true, true);
      }
      if (ExceedsMaxWeightImbalance(current_partition_balance)) {
        os_ << "Could not fix balance" << endl;
        // Don't count the ratio adjusted partition if it violates balance
        // constraints.
        continue;
      }
    }
    // TODO This is used more than once. Factor into function.
    vector<double> partition_imbalance;
    for (int tw_i = 0; tw_i < num_resources_per_node_; tw_i++) {
      if (total_weight_[tw_i] != 0) {
        partition_imbalance.push_back(
            ((double)abs(current_partition_balance[tw_i]) /
            (double)total_weight_[tw_i]));
      } else {
        partition_imbalance.push_back(0.0);
      }
    }
    vector<double> graph_ratio(num_resources_per_node_);
    vector<vector<double>> partition_ratios(2);
    vector<int> part_a_weight(num_resources_per_node_);
    vector<int> part_b_weight(num_resources_per_node_);
    int part_a_weight_sum = 0;
    int part_b_weight_sum = 0;
    int total_weight_sum = 0;
    for (int i = 0; i < num_resources_per_node_; i++) {
      part_a_weight[i] = (total_weight_[i] + current_partition_balance[i]) / 2;
      part_b_weight[i] = (total_weight_[i] - current_partition_balance[i]) / 2;
      part_a_weight_sum += part_a_weight[i];
      part_b_weight_sum += part_b_weight[i];
      total_weight_sum += total_weight_[i];
    }
    for (int i = 0; i < num_resources_per_node_; i++) {
      graph_ratio[i] = (double)total_weight_[i] / (double)total_weight_sum;
      partition_ratios[0].push_back(
          (double)part_a_weight[i] / (double)part_a_weight_sum);
      partition_ratios[1].push_back(
          (double)part_b_weight[i] / (double)part_b_weight_sum);
    }
    double ratio_denominator = 0;
    for (size_t i = 0; i < options_.resource_ratio_weights.size(); i++) {
      ratio_denominator += options_.resource_ratio_weights[i];
    }
    assert(ratio_denominator != 0);
    double sum_of_squares_a = 0;
    double sum_of_squares_b = 0;
    for (size_t i = 0; i < options_.resource_ratio_weights.size(); i++) {
      double ratio_mult = 
          double(options_.resource_ratio_weights[i]) / ratio_denominator;
      double part_a_target_weight = part_a_weight_sum * ratio_mult;
      double diff_a = abs(part_a_target_weight - part_a_weight[i]);
      // TODO Would be better if the denominator were the lower of the
      // target weight and the maximum weight in the resource. This is
      // an OK approximation of that.
      double frac_a = diff_a / part_a_weight[i];
      sum_of_squares_a += (frac_a * frac_a);
      double part_b_target_weight = part_b_weight_sum * ratio_mult;
      double diff_b = abs(part_b_target_weight - part_b_weight[i]);
      double frac_b = diff_b / part_b_weight[i];
      sum_of_squares_b += (frac_b * frac_b);
    }
    double rms_a = sqrt(sum_of_squares_a / 3);
    double rms_b = sqrt(sum_of_squares_b / 3);
    double rms_avg = (rms_a + rms_b) / 2;
    
    RUN_VERBOSE(1) {
      printf("Best cost this run: %d\n", current_partition_cost);
      printf("Imbalance: ");
      for (auto it : partition_imbalance) {
        printf("%f ", it);
      }
      printf("\n");
      printf("Resource ratios:");
      for (auto it : graph_ratio) {
        printf("%f ", it);
      }
      printf("\n");
      printf("Graph weight: ");
      for (auto it : total_weight_) {
        printf("%d ", it);
      }
      printf("\n");
      printf("Run length: %d passes\n", num_passes);
    }

    // Populate summary.
    PartitionSummary summary;
    summary.partition_node_ids.push_back(decoarsened_partition.first);
    summary.partition_node_ids.push_back(decoarsened_partition.second);
    GetCutSet(decoarsened_partition, &summary.partition_edge_ids);
    GetCutSetNames(decoarsened_partition, &summary.partition_edge_names);
    summary.partition_resource_ratios = partition_ratios;
    summary.balance = partition_imbalance;
    summary.total_resource_ratio = graph_ratio;
    summary.total_cost = current_partition_cost;
    summary.total_weight = total_weight_;
    summary.rms_resource_deviation = rms_avg;
    summary.num_passes_used = num_passes;
    summaries->push_back(summary);
  }

  DLOG(DEBUG_OPT_TRACE, 1) << "Run complete." << endl;
}

int PartitionEngineKlfm::RunKlfmAlgorithm(
    int cur_run, NodePartitions& current_partition,
    int& current_partition_cost, std::vector<int>& current_partition_balance) {

  // These vectors are not yet used for anything useful.
  vector<int> best_cost_by_pass;
  vector<vector<int>> best_cost_balance_by_pass;

  // Perform specified number of passes.
  int cur_pass = 0;
  while (cur_pass < options_.max_passes || !options_.cap_passes) {
    bool partition_changed;

    RUN_VERBOSE(2) { PrintPassInfo(cur_pass, cur_run); }
    if (PROFILE_ENABLED) RefreshProfilingData(false);

    if (options_.rebalance_on_start_of_pass) {
      RebalanceImplementations(current_partition, current_partition_balance,
                               false, options_.use_ratio_in_imbalance_score);
      RebalanceImplementations(current_partition, current_partition_balance,
                               true, options_.use_ratio_in_imbalance_score);
    }

    ExecutePass(current_partition, current_partition_cost,
        current_partition_balance, partition_changed);

    // Currently don't use this information for anything, but it's a cheap
    // copy. Don't keep a copy of the best partition by pass because that
    // would be expensive.
    best_cost_by_pass.push_back(current_partition_cost);
    best_cost_balance_by_pass.push_back(current_partition_balance);

    RUN_VERBOSE(2) {
      printf("Best cost this pass: %d\n", best_cost_by_pass[cur_pass]);
      printf("Best result found after %d moves.\n", max_at_node_count_);
      printf("Imbalance: ");
      for (int tw_i = 0; tw_i < num_resources_per_node_; tw_i++) {
        if (total_weight_[tw_i] != 0) {
          printf("%f ",
                ((double)abs(current_partition_balance.at(tw_i)) /
                (double)total_weight_[tw_i]));
        } else {
          printf("0 ");
        }
      }
      printf("\n");
      printf("Graph weight: ");
      for (auto it : total_weight_) {
          printf("%d ", it);
      }
      printf("\n");
    }

    DLOG(DEBUG_OPT_TRACE, 2) << "Pass complete." << endl;
    cur_pass++;
    // Cheap way of checking if this partition is identical to the previous
    // one: Either no nodes moved or they all did.
    // TODO Latter is a temporary bug-fix hack, and should be fixed properly
    // ASAP!
    if (!partition_changed || (max_at_node_count_ == 1)) {
        // Done with KLFM.
        printf("No difference from previous pass. Early termination.\n");
        return cur_pass;
    }
  }
  return cur_pass;
}

void PartitionEngineKlfm::ExecutePass(
    NodePartitions& current_partition, int& current_partition_cost,
    std::vector<int>& current_partition_balance, bool& partition_changed) {

    int best_cost = current_partition_cost;
    int pre_best_cost = best_cost;
    vector<int> best_cost_balance = current_partition_balance;
    double best_cost_br_power =
        ImbalancePower(current_partition_balance, max_weight_imbalance_);
    if (options_.use_ratio_in_partition_quality) {
      best_cost_br_power += RatioPower(options_.resource_ratio_weights,
                                       total_weight_);
    }
    rebalances_this_pass_ = 0;
    recompute_best_balance_flag_ = false;

    DLOG(DEBUG_OPT_TRACE, 2) << "Reset pass state." << endl;
    ResetNodeAndEdgeKlfmState(current_partition);

    // This is used to track the moves we have made since the best result for
    // a given pass. At the end of the pass, it is used to roll back
    // to the best result. This is cheaper than copying the best result.
    vector<int> nodes_moved_since_best_result;

    node_count_ = 0;
    max_at_node_count_ = 0;
    // Iterate until every node is locked.
    DLOG(DEBUG_OPT_TRACE, 2) << "Start moving nodes." << endl;
    while (!gain_bucket_manager_->Empty()) {
      node_count_++;
      MakeKlfmMove(current_partition_balance, current_partition_cost,
          best_cost_balance, best_cost, best_cost_br_power,
          current_partition, nodes_moved_since_best_result);
      if ((VERBOSITY >= 2) && (node_count_ % PROFILE_ITERATIONS == 0)) {
          printf("Processed %d nodes\n", node_count_);
      }
      if (PROFILE_ENABLED && (node_count_ % PROFILE_ITERATIONS == 0)) {
        RefreshProfilingData(true);
      }
    }

    if (pre_best_cost == best_cost && (nodes_moved_since_best_result.empty() ||
        nodes_moved_since_best_result.size() ==
            (current_partition.first.size() +
             current_partition.second.size()))) {
      partition_changed = false;
    } else {
      partition_changed = true;
    }

    // Roll-back to the best result of the pass.
    DLOG(DEBUG_OPT_TRACE, 2) << "Roll back to best result." << endl;
    RollBackToBestResultOfPass(nodes_moved_since_best_result,
        current_partition, current_partition_cost, current_partition_balance,
        best_cost, best_cost_balance);
    //assert(!ExceedsMaxWeightImbalance(*current_partition_balance));
}

void PartitionEngineKlfm::PrintPassInfo(int cur_pass, int cur_run) {
  if (options_.cap_passes) {
    printf("\n============Run %d/%d Pass %d/%d============\n\n",
        cur_run + 1, options_.num_runs, cur_pass + 1, options_.max_passes);
  } else {
    printf("\n============Run %d/%d Pass %d============\n\n",
        cur_run + 1, options_.num_runs, cur_pass + 1);
  }
}

void PartitionEngineKlfm::RefreshProfilingData(bool record_previous_results) {
  if (record_previous_results) {
    os_ << "------------------------------------" << endl;
    os_ << "GBE time: " << gbe_time_ << endl;
    os_ << "Set Node Weight Vector and Update Total Weight time: "
         << set_vector_and_update_weights_time_ << endl;
    os_ << "Move Node and Update Balance time: "
         << move_node_and_update_balance_time_ << endl;
    os_ << "MNUB Gain Update time: " << gain_update_time_ << endl;
    os_ << "MNUB GU Move Node: " << move_node_time_ << endl;
    os_ << "MNUB GU Update Gains: " << update_gains_time_ << endl;
    os_ << "Weight Update time: " << weight_update_time_ << endl;
    os_ << "Rebalance time: " << rebalance_time_ << endl;
    os_ << "Cost Update time: " << cost_update_time_ << endl;
    os_ << "Total time: " << total_move_time_ << endl;
    os_ << "Connected Edges: " << num_connected_edges_ << endl;
    os_ << "Connected Critical Edges: " << num_critical_connected_edges_
         << endl;
  }
  gbe_time_ = 0;
  set_vector_and_update_weights_time_ = 0;
  move_node_and_update_balance_time_ = 0;
  gain_update_time_ = 0;
  move_node_time_ = 0;
  update_gains_time_ = 0;
  weight_update_time_ = 0;
  rebalance_time_ = 0;
  cost_update_time_ = 0;
  total_move_time_ = 0;
  num_connected_edges_ = 0;
  num_critical_connected_edges_ = 0;
}

void PartitionEngineKlfm::ResetNodeAndEdgeKlfmState(
    const NodePartitions& partitions) {

  // Unlock all nodes.
  for (auto node_pair : internal_node_map_) {
    node_pair.second->is_locked = false;
  }
  // Reset all edges and set their criticality.
  for (auto edge_pair : internal_edge_map_) {
    edge_pair.second->KlfmReset(partitions);
  }

  // Compute initial gain of each node.
  for (auto node_pair : internal_node_map_) {
    bool in_part_a = (partitions.first.count(node_pair.first) != 0);
    ComputeInitialNodeGainAndUpdateBuckets(node_pair.second, in_part_a);
  }
}

void PartitionEngineKlfm::ComputeInitialNodeGainAndUpdateBuckets(
    Node* node, bool in_part_a) {
  int node_gain = ComputeNodeGain(node, in_part_a);
  gain_bucket_manager_->AddNode(node_gain, node, in_part_a, total_weight_);
}

int PartitionEngineKlfm::ComputeNodeGain(Node* node, bool in_part_a) {
  int node_gain = 0;
  int my_node_id = node->id;
  for (auto& port_pair : node->ports()) {
    Port& port = port_pair.second;
    int connecting_edge_id = port.external_edge_id;
    EdgeKlfm* connecting_edge = internal_edge_map_.at(connecting_edge_id);
    auto& my_partition_unlocked = (in_part_a) ?
        connecting_edge->part_a_connected_unlocked_nodes :
        connecting_edge->part_b_connected_unlocked_nodes;
    auto& my_partition_locked = (in_part_a) ?
        connecting_edge->part_a_connected_locked_nodes :
        connecting_edge->part_b_connected_locked_nodes;
    auto& opposite_partition_unlocked = (in_part_a) ?
        connecting_edge->part_b_connected_unlocked_nodes :
        connecting_edge->part_a_connected_unlocked_nodes;
    auto& opposite_partition_locked = (in_part_a) ?
        connecting_edge->part_b_connected_locked_nodes :
        connecting_edge->part_a_connected_locked_nodes;

    if (find(my_partition_locked.begin(), my_partition_locked.end(), my_node_id) !=
        my_partition_locked.end()) {
      // Node is locked. No gain possible.
      return 0;
    }

    if (my_partition_unlocked.size() == 1 && my_partition_locked.empty()) {
      // This node is the only one of this side of the partition for the
      // edge, so its gain increases, because moving it would eliminate
      // the cut in this edge.
      node_gain += connecting_edge->weight;
    } else if (opposite_partition_unlocked.empty() &&
               opposite_partition_locked.empty()) {
        // This would indicate that there was only one node on an edge, which
        // is an invalid configuration. If this method ever gets called in the
        // inner loop of the algorithm, remove this.
        int nodes_on_this_side = my_partition_unlocked.size() +
                                 my_partition_locked.size();
        assert(nodes_on_this_side != 1);
      // There are no nodes on the opposite partition, so decrease the
      // gain of this node, since moving it will cause the edge to be cut.
      node_gain -= connecting_edge->weight;
    }
  }
  return node_gain;
}

void PartitionEngineKlfm::MakeKlfmMove(
    vector<int>& current_partition_balance,
    int& current_partition_cost,
    vector<int>& best_cost_balance,
    int& best_cost,
    double& best_cost_br_power,
    NodePartitions& current_partition,
    vector<int>& nodes_moved_since_best_result) {
  if (PROFILE_ENABLED) {
    gbe_start_time_ = GetTimeUsec();
    total_move_start_time_ = GetTimeUsec();
  }
  GainBucketEntry entry = gain_bucket_manager_->GetNextGainBucketEntry(
      current_partition_balance, total_weight_);
  if (PROFILE_ENABLED) {
    gbe_time_ += GetTimeUsec() - gbe_start_time_;
    set_vector_and_update_weights_start_time_ = GetTimeUsec();
  }

  const int gain = entry.gain;
  const int node_id_to_move = entry.id;
  const bool from_part_a = current_partition.first.count(node_id_to_move) != 0;
  Node* node_to_move = internal_node_map_.at(node_id_to_move);

  // Account for the gain bucket potentially selecting a different
  // implementation for the node than in the previous pass.
  // Note: Even if the gain bucket is a non-adaptive type, the node's
  // implementation may have been changed by rebalancing or mutation, so this
  // step should be carried out regardless of gain bucket type.
  vector<int> previous_weight_vector = node_to_move->SelectedWeightVector();
  node_to_move->SetSelectedWeightVectorWithRollback(
      entry.current_weight_vector_index);
  UpdateTotalWeightsForImplementationChange(
      previous_weight_vector, node_to_move->SelectedWeightVector());

  VLOG(3) << "Move node ID: " << node_id_to_move << " Gain: " << gain << endl;
  RUN_DEBUG(DEBUG_OPT_PARTITION_IMBALANCE_EXCEEDED, 1) {
    if (balance_exceeded_) {
      printf("In Balance Exceeded Mode. Max imbalance: ");
      for (auto it : max_weight_imbalance_) {
        printf("%d ", it);
      }
      printf("\n");
      printf("Current balance: ");
      for (auto it : current_partition_balance) {
        printf("%d ", it);
      }
      printf("\n");
      printf("Balance change from move: ");
      for (auto it : entry.current_weight_vector()) {
        if (from_part_a) {
          printf("-");
        } else {
          printf("+");
        }
        printf("%d ", it);
      }
      printf("\n");
    }
    if (options_.rebalance_on_demand) {
      if (rebalances_this_run_ >= options_.rebalance_on_demand_cap_per_run &&
          !options_.rebalance_on_demand_cap_per_run) {
        printf("On-Demand Rebalance cap per run exceeded\n");
      } else if (rebalances_this_pass_ >=
                 options_.rebalance_on_demand_cap_per_pass &&
                 !options_.rebalance_on_demand_cap_per_pass) {
        printf("On-Demand Rebalance cap per pass exceeded\n");
      } else {
        printf("On-Demand Rebalance possible\n");
      }
    }
  }

  if (PROFILE_ENABLED) {
    set_vector_and_update_weights_time_ +=
      GetTimeUsec() - set_vector_and_update_weights_start_time_;
    move_node_and_update_balance_start_time_ = GetTimeUsec();
  }

  // Move the node in the node tracking containers.
  MoveNodeAndUpdateBalance(from_part_a, current_partition, node_to_move,
      entry.current_weight_vector(), previous_weight_vector,
      current_partition_balance);

  RUN_DEBUG(DEBUG_OPT_BALANCE_CHECK, 1) {
    vector<int> rec_balance;
    rec_balance = RecomputeCurrentBalance(current_partition);
    for (int i = 0; i < current_partition_balance.size(); i++) {
      assert(rec_balance[i] == current_partition_balance.at(i));
    }
  }

  RUN_DEBUG(DEBUG_OPT_TOTAL_WEIGHT_CHECK, 1) {
    vector<int> cur_total_weight = total_weight_;
    vector<int> cur_max_imbalance = max_weight_imbalance_;
    RecomputeTotalWeightAndMaxImbalance();
    assert(cur_total_weight == total_weight_);
    assert(cur_max_imbalance == max_weight_imbalance_);
  }

  if (PROFILE_ENABLED) {
    move_node_and_update_balance_time_ +=
      GetTimeUsec() - move_node_and_update_balance_start_time_;
    rebalance_start_time_ = GetTimeUsec();
  }

  // Include ExceedsMaxWeightImbalance check here to allow partition weight
  // to exceed max weight imbalance during the iteration but roll back to the
  // best result that fit.
  bool prev_exceeded = balance_exceeded_;
  balance_exceeded_ = ExceedsMaxWeightImbalance(current_partition_balance);

  // Attempt to fix imbalance by adjusting node implementations.
  if (options_.rebalance_on_demand && balance_exceeded_ &&
      (rebalances_this_run_ < options_.rebalance_on_demand_cap_per_run) &&
      (rebalances_this_pass_ < options_.rebalance_on_demand_cap_per_pass)) {
    DLOG(DEBUG_OPT_REBALANCE, 0) << "Rebalance implementations on-demand."
                                 << endl;
    RUN_DEBUG(DEBUG_OPT_REBALANCE, 0) {
      os_ << "Current fraction of max imbalance: ";
      PrintWeightImbalanceFraction(current_partition_balance,
                                   max_weight_imbalance_);
      os_ << endl;
    }
    RebalanceImplementations(current_partition, current_partition_balance,
                             true, options_.use_ratio_in_imbalance_score);
    rebalances_this_run_++;
    rebalances_this_pass_++;
    RUN_DEBUG(DEBUG_OPT_REBALANCE, 0) {
      os_ << "New fraction of max imbalance: ";
      PrintWeightImbalanceFraction(current_partition_balance,
                                   max_weight_imbalance_);
      os_ << endl;
    }
    balance_exceeded_ = ExceedsMaxWeightImbalance(current_partition_balance);
    if (!balance_exceeded_) {
      DLOG(DEBUG_OPT_REBALANCE, 0) << "Rebalance successful." << endl;
    } else {
      DLOG(DEBUG_OPT_REBALANCE, 0) << "Rebalance unsuccessful." << endl;
      recompute_best_balance_flag_ = true;
    }
  }
  RUN_DEBUG(DEBUG_OPT_PARTITION_IMBALANCE_EXCEEDED, 0) {
    if (!prev_exceeded && balance_exceeded_) {
      printf("Balance exceeded starting at move %d\n", node_count_);
    } else if (prev_exceeded && !balance_exceeded_) {
      printf("Returned to balance at move %d\n", node_count_);
    }
  }
  if (PROFILE_ENABLED) {
    rebalance_time_ += GetTimeUsec() - rebalance_start_time_;
    cost_update_start_time_ = GetTimeUsec();
  }

  // Check if the best solution result needs updating.
  current_partition_cost -= gain;
  RUN_DEBUG(DEBUG_OPT_COST_CHECK, 1) {
    int rec_cost = RecomputeCurrentCost();
    assert(current_partition_cost == rec_cost);
  }

  double current_br_power = ImbalancePower(current_partition_balance,
                                           max_weight_imbalance_);
  if (options_.use_ratio_in_partition_quality) {
    current_br_power +=
        RatioPower(options_.resource_ratio_weights, total_weight_);
  }
  // TODO: Better to use < or <= here?
  // < might result in the algorithm terminating earlier than necessary -
  // i.e. if it only finds partitions that tie with the initial partition,
  // it will terminate.
  // <= will require extra logic to avoid a potential infinite loop of moves
  // between partitions that are different but have the same cost.
  if (!balance_exceeded_ &&
      (current_partition_cost < best_cost  ||
       (current_partition_cost == best_cost &&
        current_br_power < best_cost_br_power))) {
    max_at_node_count_ = node_count_;
    best_cost = current_partition_cost;
    best_cost_balance = current_partition_balance;
    best_cost_br_power = current_br_power;
    recompute_best_balance_flag_ = false;
    nodes_moved_since_best_result.clear();
  } else {
    nodes_moved_since_best_result.push_back(node_id_to_move);
  }
  if (PROFILE_ENABLED) {
    cost_update_time_ += GetTimeUsec() - cost_update_start_time_;
    total_move_time_ += GetTimeUsec() - total_move_start_time_;
  }
}

void PartitionEngineKlfm::MoveNodeAndUpdateBalance(
    bool from_part_a, NodePartitions& current_partition, Node* node,
    const vector<int>& weight_vector,
    const vector<int>& prev_weight_vector, std::vector<int>& balance) {
  if (PROFILE_ENABLED) weight_update_start_time_ = GetTimeUsec();
  // Move the node in the node tracking containers.
  if (from_part_a) {
    current_partition.first.erase(node->id);
    current_partition.second.insert(node->id);
    for (int wt_it = 0; wt_it < num_resources_per_node_; wt_it++) {
      balance[wt_it] -= (weight_vector[wt_it] + prev_weight_vector[wt_it]);
    }
  } else {
    current_partition.second.erase(node->id);
    current_partition.first.insert(node->id);
    for (int wt_it = 0; wt_it < num_resources_per_node_; wt_it++) {
      balance[wt_it] += (weight_vector[wt_it] + prev_weight_vector[wt_it]);
    }
  }
  if (PROFILE_ENABLED) {
    weight_update_time_ += GetTimeUsec() - weight_update_start_time_;
  }

  // Move the node on all edges that touch it, update their criticality,
  // and update the gains of the nodes that need it.
  if (PROFILE_ENABLED) gain_update_start_time_ = GetTimeUsec();
  UpdateMovedNodeEdgesAndNodeGains(node, from_part_a);
  if (PROFILE_ENABLED) {
    gain_update_time_ += GetTimeUsec() - gain_update_start_time_;
  }
}

void PartitionEngineKlfm::UpdateMovedNodeEdgesAndNodeGains(
    Node* moved_node, bool from_part_a) {
  for (auto& port_pair : moved_node->ports()) {

    num_connected_edges_++;
    const int connected_edge_id = port_pair.second.external_edge_id;
    EdgeKlfm* connected_edge = internal_edge_map_.at(connected_edge_id);
    if (connected_edge->is_critical) num_critical_connected_edges_++;
    EdgeKlfm::NodeIdVector nodes_to_increase_gain;
    EdgeKlfm::NodeIdVector nodes_to_decrease_gain;
    connected_edge->MoveNode(moved_node->id, &nodes_to_increase_gain,
                             &nodes_to_decrease_gain);

    // Due to the nature of the KLFM algorithm, the nodes that have their
    // gains increased are always in the same partition that the node was
    // moved from and the gains to be decreased are in the partition it
    // was moved to.
    int gain_modifier = connected_edge->weight;
    gain_bucket_manager_->UpdateGains(gain_modifier, nodes_to_increase_gain,
                                      nodes_to_decrease_gain, from_part_a);
  }
}

void PartitionEngineKlfm::RollBackToBestResultOfPass(
    vector<int>& nodes_moved_since_best_result,
    NodePartitions& current_partition,
    int& current_partition_cost, vector<int>& current_partition_balance,
    const int& best_cost, const vector<int>& best_cost_balance) {
  for (auto id : nodes_moved_since_best_result) {
    if (current_partition.first.count(id) != 0) {
      current_partition.first.erase(id);
      current_partition.second.insert(id);
    } else {
      current_partition.second.erase(id);
      current_partition.first.insert(id);
    }
    Node* node = internal_node_map_.at(id);
    vector<int> current_wv = node->SelectedWeightVector();
    node->RevertSelectedWeightVector();
    vector<int> new_wv = node->SelectedWeightVector();
    UpdateTotalWeightsForImplementationChange(current_wv, new_wv);
  }
  nodes_moved_since_best_result.clear();
  current_partition_cost = best_cost;
  // TODO Hack while there are bugs in balance when rolling back.
  if (options_.rebalance_on_demand) {
  //if (recompute_best_balance_flag_) {
    current_partition_balance = RecomputeCurrentBalance(current_partition);
    RecomputeTotalWeightAndMaxImbalance();
  } else {
    current_partition_balance = best_cost_balance;
  }
  RUN_DEBUG(DEBUG_OPT_BALANCE_CHECK, 1) {
    vector<int> rec_balance;
    rec_balance = RecomputeCurrentBalance(current_partition);
    assert(rec_balance == current_partition_balance);
  }
  RUN_DEBUG(DEBUG_OPT_TOTAL_WEIGHT_CHECK, 1) {
    vector<int> cur_total_weight = total_weight_;
    vector<int> cur_max_imbalance = max_weight_imbalance_;
    RecomputeTotalWeightAndMaxImbalance();
    assert(cur_total_weight == total_weight_);
    assert(cur_max_imbalance == max_weight_imbalance_);
  }
  RUN_DEBUG(DEBUG_OPT_COST_CHECK, 1) {
    int rec_cost = RecomputeCurrentCost();
    assert(current_partition_cost == rec_cost);
  }
}

bool PartitionEngineKlfm::PartitionsIdentical(
    const NodePartitions& a, const NodePartitions& b) const {

  // Handle the case where partitions may be identical by the order of first/
  // second is swapped between the two.
  bool swap = false;
  auto a_fb = a.first.begin();
  auto b_fb = b.first.begin();
  if (*a_fb != *b_fb) {
    if (*a_fb == *b.second.begin()) {
      swap = true;
    } else {
      return false;
    }
  }

  auto af_it = a.first.begin();
  auto as_it = a.second.begin();
  auto bf_it = (swap) ? b.second.begin() : b.first.begin();
  auto bs_it = (swap) ? b.first.begin() : b.second.begin();

  while (af_it != a.first.end() && bf_it != b.first.end()) {
    if (*af_it != *bf_it) {
      return false;
    }
    ++af_it;
    ++bf_it;
  }
  if (af_it != a.first.end() && bf_it != b.first.end()) {
    return false;
  }
  while (as_it != a.second.end() && bs_it != b.second.end()) {
    if (*as_it != *bs_it) {
      return false;
    }
    ++as_it;
    ++bs_it;
  }
  if (as_it != a.second.end() && bs_it != b.second.end()) {
    return false;
  }
  return true;
}

void PartitionEngineKlfm::Reset() {
}

void PartitionEngineKlfm::PopulateEdgePartitionConnections(
    const NodePartitions& partition) {
  for (auto edge_pair : internal_edge_map_) {
    EdgeKlfm* edge = edge_pair.second;
    edge->PopulatePartitionNodeIds(partition);
  }
}

void PartitionEngineKlfm::GenerateInitialPartition(
    NodePartitions* partition, int* cost, vector<int>* balance) {
  partition->first.clear();
  partition->second.clear();
  switch(options_.seed_mode) {
    case Options::kSeedModeRandom:
      DLOG(DEBUG_OPT_TRACE, 1) <<
          "Generating initial partition using RANDOM policy." << endl;
      GenerateInitialPartitionRandom(partition, cost, balance);
      break;
    case Options::kSeedModeUserSpecified:
      DLOG(DEBUG_OPT_TRACE, 1) <<
          "Generating initial partition using USER SPECIFIED policy." << endl;
      partition->first = options_.initial_a_nodes;
      partition->second = options_.initial_b_nodes;
      break;
    case Options::kSeedModeSimpleDeterministic:
      DLOG(DEBUG_OPT_TRACE, 1) <<
          "Generating initial partition using SIMPLE DETERMINISTIC policy." <<
          endl;
      assert(false);  // No longer supported.
      break;
    default:
      printf("Invalid seed partition mode set: %d\n", options_.seed_mode);
      exit(0);
  }
  bool exceeded = ExceedsMaxWeightImbalance(*balance);
  if (exceeded) {
    VLOG(1) << "Initial Partition didn't meet balance requirements."
            << "Attempting Rebalance" << endl;
    RebalanceImplementations(*partition, *balance, true, false);
    if (ExceedsMaxWeightImbalance(*balance)) {
      printf("Warning initial partition exceeds weight imbalance maximum!\n");
    }
  }
}

void PartitionEngineKlfm::GenerateInitialPartitionRandom(
    NodePartitions* partition, int* cost, vector<int>* balance) {

  // Randomly assign nodes to partitions, obeying balance constraints.
  vector<int> part_a_current_weight;
  vector<int> part_b_current_weight;
  vector<int> current_balance;
  part_a_current_weight.insert(
      part_a_current_weight.begin(), num_resources_per_node_, 0);
  part_b_current_weight.insert(
      part_b_current_weight.begin(), num_resources_per_node_, 0);
  current_balance.insert(current_balance.begin(), num_resources_per_node_, 0);
  vector<int> node_ids;
  for (auto it : internal_node_map_) {
    node_ids.push_back(it.first);
  }
  shuffle(node_ids.begin(), node_ids.end(), random_engine_initial_);
  
  for (auto it : node_ids) {
    vector<int> node_weights =
        internal_node_map_.at(it)->SelectedWeightVector();
    assert_b(node_weights.size() == num_resources_per_node_) {
      printf("\nDetected an inconsistent number of resource weights per node "
             "implementation. All implementations of all nodes in the graph "
             "must have the same number of resources.\n");
    }
    // This is an imperfect method to guarantee balance, but may be good enough
    // with the fixer method called afterward. It finds the resource that is
    // most out of balance and assigns the node to the partition that decreases
    // that imbalance, prioritizing resources that exceed maximum imbalance.
    // TODO: Come up with something more robust.
    double max_imbalance_frac = 0.0;
    int choose_resource = 0;
    for (int i = 0; i < num_resources_per_node_; i++) {
      if (node_weights[i] != 0) {
        int imbalance = abs(current_balance[i]);
        double imbalance_frac =
            (double)imbalance / (double)max_weight_imbalance_[i];
        if (imbalance_frac >= max_imbalance_frac) {
          max_imbalance_frac = imbalance_frac;
          choose_resource = i;
        }
      }
    }
    if (current_balance[choose_resource] >= 0) {
      partition->second.insert(it);
      for (size_t i = 0; i < node_weights.size(); i++) {
        part_b_current_weight[i] += node_weights[i];
        current_balance[i] -= node_weights[i];
      }
    } else {
      partition->first.insert(it);
      for (size_t i = 0; i < node_weights.size(); i++) {
        part_a_current_weight[i] += node_weights[i];
        current_balance[i] += node_weights[i];
      }
    }
  }
  *balance = current_balance;
  assert(!partition->first.empty() && !partition->second.empty());

  PopulateEdgePartitionConnections(*partition);

  // Get the initial cost.
  *cost = RecomputeCurrentCost();
}

void PartitionEngineKlfm::FixInitialWeightImbalance(
    NodePartitions* partition, vector<int>* part_a_current_weight,
    vector<int>* part_b_current_weight, vector<int>* current_balance) {
  // TODO implement.
  assert_b(false) {
    printf("Function not implemented\n");
  }
}

bool PartitionEngineKlfm::ExceedsMaxWeightImbalance(
    const vector<int>& current_balance) const {
  for (int res_id = 0; res_id < num_resources_per_node_; res_id++) {
    if (options_.constrain_balance_by_resource[res_id] &&
        abs(current_balance[res_id]) > max_weight_imbalance_[res_id]) {
      return true;
    }
  }
  return false;
}

void PartitionEngineKlfm::RecomputeTotalWeightAndMaxImbalance() {
  total_weight_.assign(num_resources_per_node_, 0);
  for (auto node_pair : internal_node_map_) {
    vector<int> node_weight = node_pair.second->SelectedWeightVector();
    for (int i = 0; i < num_resources_per_node_; i++) {
      total_weight_[i] += node_weight[i];
    }
  }

  max_weight_imbalance_.resize(num_resources_per_node_);
  assert(max_weight_imbalance_.size() ==
         options_.constrain_balance_by_resource.size());
  for (int i = 0; i < max_weight_imbalance_.size(); i++) {
    if (options_.constrain_balance_by_resource[i]) {
      max_weight_imbalance_[i] =
          (int)(total_weight_[i] * options_.max_imbalance_fraction[i]);
      if (max_weight_imbalance_[i] <= 0) {
        max_weight_imbalance_[i] = 1;
      }
    } else {
      // Since max_weight_imbalance_ is sometimes multiplied by 2,
      // dividing it here avoids adding overflow checks elsewhere and still
      // accomplishes the same thing.
      max_weight_imbalance_[i] = numeric_limits<int>::max() / 3;
    }
  }
}

int PartitionEngineKlfm::RecomputeCurrentCost() {
  int current_cost = 0;
  for (auto it : internal_edge_map_) {
    const EdgeKlfm* edge = CHECK_NOTNULL(it.second);
    current_cost += ComputeEdgeCost(*edge);
  }
  return current_cost;
}

int PartitionEngineKlfm::ComputeEdgeCost(const EdgeKlfm& edge) {
  if (edge.CrossesPartitions()) {
    return edge.weight;
  } else {
    return 0;
  }
}

void PartitionEngineKlfm::GetCutSet(const NodePartitions& partition,
                                    std::set<int>* cut_set) {
  for (auto it : internal_edge_map_) {
    const Edge* edge = it.second;
    int a_count = 0;
    int b_count = 0;
    for (auto node_id : edge->connection_ids()) {
      if (partition.first.count(node_id) != 0) {
        a_count++;
      } else {
        b_count++;
      }
      if (a_count != 0 && b_count != 0) {
        cut_set->insert(edge->id);
        break;
      }
    }
  }
}

void PartitionEngineKlfm::GetCutSetNames(
    const NodePartitions& partition, std::set<std::string>* cut_set) {
  for (auto it : internal_edge_map_) {
    const Edge* edge = it.second;
    int a_count = 0;
    int b_count = 0;
    for (auto node_id : edge->connection_ids()) {
      if (partition.first.count(node_id) != 0) {
        a_count++;
      } else {
        b_count++;
      }
      if (a_count != 0 && b_count != 0) {
        if (!edge->name.empty()) {
          cut_set->insert(edge->name);
        }
        break;
      }
    }
  }
}

std::vector<int> PartitionEngineKlfm::RecomputeCurrentBalance(
    const NodePartitions& partition) {
  vector<int> balance;
  balance.assign(num_resources_per_node_, 0);
  for (auto node_id : partition.first) {
    Node* node = internal_node_map_.at(node_id);
    for (int res = 0; res < num_resources_per_node_; res++) {
      balance[res] += node->SelectedWeightVector()[res];
    }
  }
  for (auto node_id : partition.second) {
    Node* node = internal_node_map_.at(node_id);
    for (int res = 0; res < num_resources_per_node_; res++) {
      balance[res] -= node->SelectedWeightVector()[res];
    }
  }
  return balance;
}

std::vector<int> PartitionEngineKlfm::RecomputeCurrentBalanceAtBaseLevel(
    const NodePartitions& partition) {
  vector<int> balance;
  balance.assign(num_resources_per_node_, 0);
  map<int, Node*> first, second;
  for (auto node_id : partition.first) {
    first.insert(make_pair(node_id, internal_node_map_.at(node_id)));
  }
  for (auto node_id : partition.second) {
    second.insert(make_pair(node_id, internal_node_map_.at(node_id)));
  }
  bool base_level = false;
  while (!base_level) {
    set<int> to_erase;
    set<pair<int, Node*>> to_add;
    for (auto node_pair : first) {
      if (node_pair.second->is_supernode()) {
        to_erase.insert(node_pair.first);
        for (auto int_pair : node_pair.second->internal_nodes()) {
          to_add.insert(make_pair(int_pair.first, int_pair.second));
        }
      }
    }
    for (auto id : to_erase) {
      first.erase(id);
    }
    for (auto npair : to_add) {
      first.insert(make_pair(npair.first, npair.second));
    }
    if (to_erase.empty()) {
      base_level = true;
    }
  }
  base_level = false;
  while (!base_level) {
    set<int> to_erase;
    set<pair<int, Node*>> to_add;
    for (auto node_pair : second) {
      if (node_pair.second->is_supernode()) {
        to_erase.insert(node_pair.first);
        for (auto int_pair : node_pair.second->internal_nodes()) {
          to_add.insert(make_pair(int_pair.first, int_pair.second));
        }
      }
    }
    for (auto id : to_erase) {
      second.erase(id);
    }
    for (auto npair : to_add) {
      second.insert(make_pair(npair.first, npair.second));
    }
    if (to_erase.empty()) {
      base_level = true;
    }
  }

  for (auto node_pair : first) {
    Node* node = node_pair.second;
    for (int res = 0; res < num_resources_per_node_; res++) {
      balance[res] += node->SelectedWeightVector()[res];
    }
  }
  for (auto node_pair : second) {
    Node* node = node_pair.second;
    for (int res = 0; res < num_resources_per_node_; res++) {
      balance[res] -= node->SelectedWeightVector()[res];
    }
  }
  return balance;
}

void PartitionEngineKlfm::RebalanceImplementations(
    const NodePartitions& current_partition, vector<int>& partition_imbalance,
    bool use_imbalance, bool use_ratio) {
  // TODO : Once a resource's weight has reached zero, that resource cannot
  // be used again, as any change will result in balance exceeded.
  if (!(use_ratio || use_imbalance)) {
    return;
  }
  vector<int> all_ids;
  for (auto node_id : current_partition.first) {
    all_ids.push_back(node_id);
  }
  for (auto node_id : current_partition.second) {
    all_ids.push_back(node_id);
  }
  bool prev_exceeds = ExceedsMaxWeightImbalance(partition_imbalance);
  shuffle(all_ids.begin(), all_ids.end(), random_engine_rebalance_);
  for (int pass = 0; pass < REBALANCE_PASSES; pass++) {
    for (auto node_id : all_ids) {
      vector<int> old_balance = partition_imbalance;

      Node* node = internal_node_map_.at(node_id);
      bool in_part_a =
         current_partition.first.find(node_id) != current_partition.first.end();
      vector<int> prev_wv = node->SelectedWeightVector();
      int prev_wv_index = node->selected_weight_vector_index();
      node->SetWeightVectorToMinimizeImbalance(
          partition_imbalance, max_weight_imbalance_, in_part_a,
          use_imbalance, use_ratio,
          options_.resource_ratio_weights, total_weight_);
      vector<int> new_wv = node->SelectedWeightVector();
      UpdateTotalWeightsForImplementationChange(prev_wv, new_wv);
      bool new_exceeds = ExceedsMaxWeightImbalance(partition_imbalance);
      if (new_exceeds && !prev_exceeds) {
        node->SetSelectedWeightVector(prev_wv_index);
        UpdateTotalWeightsForImplementationChange(new_wv, prev_wv);
        for (int i = 0; i < new_wv.size(); i++) {
          if (in_part_a) {
            partition_imbalance[i] += prev_wv[i] - new_wv[i];
          } else {
            partition_imbalance[i] -= prev_wv[i] - new_wv[i];
          }
        }
        new_exceeds = ExceedsMaxWeightImbalance(partition_imbalance);
        assert(!new_exceeds);
      } else {
        // If this becomes a performance issue, it could be removed.
        gain_bucket_manager_->UpdateNodeImplementation(node);
      }
      prev_exceeds = new_exceeds;

      RUN_DEBUG(DEBUG_OPT_BALANCE_CHECK, 2) {
        vector<int> new_balance = partition_imbalance;
        vector<int> rec_balance = RecomputeCurrentBalance(current_partition);
        for (int i = 0; i < rec_balance.size(); i++) {
          assert(rec_balance[i] == new_balance[i]);
        }
      }
    }
  }
}

void PartitionEngineKlfm::MutateImplementations(int mutation_rate) {
  // TODO If mutation occurs after nodes have been added to the gain bucket,
  // add support for updating the gain bucket with the new node implementations.
  assert(mutation_rate >= 0 && mutation_rate <= 100);
  for (auto node_pair : internal_node_map_) {
    int num_impl = node_pair.second->WeightVectors().size();  
    if (num_impl == 1) {
      continue;
    }
    int mutate_rn = random_engine_mutate_() % 100;
    if (mutation_rate > mutate_rn) {
      int rand_impl = random_engine_mutate_() % num_impl;
      node_pair.second->SetSelectedWeightVector(rand_impl);
    }
  }
  RecomputeTotalWeightAndMaxImbalance();
}

void PartitionEngineKlfm::Options::Print(ostream& os) {
  os << "KLFM Options: " << endl;
  os << "Num Runs: " << num_runs << endl;
  os << "Cap Passes: " << (cap_passes ? "true" : "false") << endl;
  if (cap_passes) {
    os << "Max Passes: " << max_passes << endl;
  }
  os << "Number of Resources: " << num_resources_per_node << endl;
  os << "Maximum Imbalance: ";
  for (auto it : max_imbalance_fraction) {
    os << it << " ";
  }
  os << endl;
  os << "Target Ratio: ";
  for (auto it : resource_ratio_weights) {
    os << it << " ";
  }
  os << endl;
  os << "Use Ratio in Partition Quality: "
     << (use_ratio_in_partition_quality ? "true" : "false") << endl;
  os << "Use Ratio in Imbalance Score: "
     << (use_ratio_in_imbalance_score ? "true" : "false") << endl;
  os << "Enable Adaptive Node Implementations: "
     << (use_adaptive_node_implementations ? "true" : "false") << endl;
  os << "Use Multilevel Constraint Relaxation: "
     << (use_multilevel_constraint_relaxation ? "true" : "false") << endl;
  os << "Gain Bucket Type: ";
  switch (gain_bucket_type) {
    case PartitionerConfig::kGainBucketSingleResource:
      os << "Single Resource";
    break;
    case PartitionerConfig::kGainBucketMultiResourceExclusive:
    case PartitionerConfig::kGainBucketMultiResourceExclusiveAdaptive:
      os << "Multi Resource Exclusive";
    break;
    case PartitionerConfig::kGainBucketMultiResourceMixed:
    case PartitionerConfig::kGainBucketMultiResourceMixedAdaptive:
      os << "Multi Resource Mixed";
    break;
    default:
      assert_b(false) {
        printf("\nUnrecognized gain bucket type.\n");
      }
  }
  os << endl;
  os << "Gain Bucket Selection Policy: ";
  if (gain_bucket_type == PartitionerConfig::kGainBucketSingleResource) {
    os << "Single Resource - Default Policy";
  } else {
    switch (gain_bucket_selection_policy) {
      case PartitionerConfig::kGbmreSelectionPolicyRandomResource:
        os << "Random Resource";
      break;
      case PartitionerConfig::kGbmreSelectionPolicyLargestResourceImbalance:
        os << "Largest Resource Imbalance";
      break;
      case PartitionerConfig::kGbmreSelectionPolicyLargestUnconstrainedGain:
        os << "Largest Unconstrained Gain";
      break;
      case PartitionerConfig::kGbmreSelectionPolicyLargestGain:
        os << "Largest Gain";
      break;
      case PartitionerConfig::kGbmrmSelectionPolicyRandomResource:
        os << "Random Resource";
      break;
      case PartitionerConfig::kGbmrmSelectionPolicyMostUnbalancedResource:
        os << "Most Unbalanced Resource";
      break;
      case PartitionerConfig::kGbmrmSelectionPolicyBestGainImbalanceScoreClassic:
        os << "Best Gain Imbalance Score Classic";
      break;
      case PartitionerConfig::kGbmrmSelectionPolicyBestGainImbalanceScoreWithAffinities:
        os << "Best Gain Imbalance Score With Affinities";
      break;
      default:
        assert_b(false) {
          printf("\nUnrecognized gain bucket selection policy.\n");
        }
    }
  }
  os << endl;
  os << "Restrict Supernodes to Default Implementation: "
     << (restrict_supernodes_to_default_implementation ? "true" : "false")
     << endl;
  os << "Supernode Implementations Cap: " << supernode_implementations_cap
     << endl;
  os << "Reuse Previous Run Implementations: "
     << (reuse_previous_run_implementations ? "true" : "false") << endl;
  os << "Enable Mutation: " << (enable_mutation ? "true" : "false") << endl;
  if (enable_mutation) {
    os << "Mutation Rate: " << mutation_rate << endl;
  }
  os << "Rebalance on Start of Pass: "
     << (rebalance_on_start_of_pass ? "true" : "false") << endl;
  os << "Rebalance on End of Run: "
     << (rebalance_on_end_of_run ? "true" : "false") << endl;
  os << "Rebalance on Demand: "
     << (rebalance_on_demand ? "true" : "false") << endl;
  if (rebalance_on_demand) {
    os << "Rebalance on Demand Cap per Run: " << rebalance_on_demand_cap_per_run
       << endl;
    os << "Rebalance on Demand Cap per Pass: "
       << rebalance_on_demand_cap_per_pass << endl;
  }
  os << endl;
}

void PartitionEngineKlfm::Options::PopulateFromPartitionerConfig(
    const PartitionerConfig& config) {
  num_resources_per_node = config.device_resource_capacities.size();
  gain_bucket_type = config.gain_bucket_type;
  gain_bucket_selection_policy = config.gain_bucket_selection_policy;
  max_imbalance_fraction.clear();
  for (auto frac : config.device_resource_max_imbalances) {
    max_imbalance_fraction.push_back(frac);
  }
  assert(max_imbalance_fraction.size() == num_resources_per_node);
  constrain_balance_by_resource.clear();
  constrain_balance_by_resource.insert(constrain_balance_by_resource.begin(),
      num_resources_per_node, true);
  if (gain_bucket_type == PartitionerConfig::kGainBucketSingleResource) {
    for (int i = 1; i < num_resources_per_node; i++) {
      constrain_balance_by_resource[i] = false;
    }
  } else {
    for (int i = 0; i < num_resources_per_node; i++) {
      if (max_imbalance_fraction[i] >= 0.99) {
        constrain_balance_by_resource[i] = false;
      } else {
        constrain_balance_by_resource[i] = true;
      }
    }
  }
  device_resource_capacities.clear();
  for (auto cap : config.device_resource_capacities) {
    device_resource_capacities.push_back(cap);
  }
  assert(device_resource_capacities.size() == num_resources_per_node);
  resource_ratio_weights.clear();
  for (auto ratio_wt : config.device_resource_ratio_weights) {
    resource_ratio_weights.push_back(ratio_wt);
  }
  assert(resource_ratio_weights.size() == num_resources_per_node);
  use_multilevel_constraint_relaxation =
      config.use_multilevel_constraint_relaxation;
  use_adaptive_node_implementations = config.use_adaptive_node_implementations;
  use_ratio_in_imbalance_score = config.use_ratio_in_imbalance_score;
  use_ratio_in_partition_quality = config.use_ratio_in_partition_quality;
  restrict_supernodes_to_default_implementation =
      config.restrict_supernodes_to_default_implementation;
  supernode_implementations_cap = config.supernode_implementations_cap;
  reuse_previous_run_implementations =
      config.reuse_previous_run_implementations;
  enable_mutation = (config.mutation_rate > 0);
  mutation_rate = config.mutation_rate;
  rebalance_on_start_of_pass = config.rebalance_on_start_of_pass;
  rebalance_on_end_of_run = config.rebalance_on_end_of_run;
  rebalance_on_demand = config.rebalance_on_demand;
  rebalance_on_demand_cap_per_run = config.rebalance_on_demand_cap_per_run;
  rebalance_on_demand_cap_per_pass = config.rebalance_on_demand_cap_per_pass;
}

void PartitionEngineKlfm::CoarsenMaxEdgeDegree(
    int max_nodes_per_supernode) {
  assert_b(false) {
    printf("Function not yet implemented.\n");
  }
}

// TODO Rather than basing this on the number of nodes, it would be better if
// it instead used a maximum supernode weight.
void PartitionEngineKlfm::CoarsenMaxNodeDegree(
    int max_nodes_per_supernode) {
  vector<pair<int,int>> node_id_degree_pairs;
  set<int> valid_node_ids;
  for (auto node_pair : internal_node_map_) {
    int node_degree = node_pair.second->ports().size();
    node_id_degree_pairs.push_back(make_pair(node_pair.first, node_degree));
    valid_node_ids.insert(node_pair.first);
  }
  sort(node_id_degree_pairs.begin(), node_id_degree_pairs.end(),
       &cmp_pair_second_gt);
  // Shuffle ranges with the same degree.
  auto start_it = node_id_degree_pairs.begin();
  while (start_it != node_id_degree_pairs.end()) {
    auto one_past_end_it = start_it;
    while (one_past_end_it != node_id_degree_pairs.end() && 
           start_it->second == one_past_end_it->second) {
      one_past_end_it++;
    }
    shuffle(start_it, one_past_end_it, random_engine_coarsen_);
    start_it = one_past_end_it;
  }

  for (auto id_degree_pair : node_id_degree_pairs) {
    int main_node_id = id_degree_pair.first;
    auto mn_it = valid_node_ids.find(main_node_id);
    if (mn_it != valid_node_ids.end()) {
      valid_node_ids.erase(mn_it);
      Node* node = internal_node_map_.at(main_node_id);
      set<int> nodes_in_supernode;
      nodes_in_supernode.insert(main_node_id);
      int found_nodes = 1;
      for (auto& port_pair : node->ports()) {
        Edge* edge = internal_edge_map_.at(port_pair.second.external_edge_id);
        for (auto cnx_node_id : edge->connection_ids()) {
          auto vn_it = valid_node_ids.find(cnx_node_id);
          if (vn_it != valid_node_ids.end()) {
            valid_node_ids.erase(vn_it);
            nodes_in_supernode.insert(cnx_node_id);
            found_nodes++;
            if (found_nodes == max_nodes_per_supernode) {
              break;
            }
          }
        }
        if (found_nodes == max_nodes_per_supernode) {
          break;
        }
      }
      if (found_nodes != 1) {
        MakeSupernode(
            nodes_in_supernode, &internal_node_map_, &internal_edge_map_, NULL);
      }
      // Note: It might make sense to update the degrees here for nodes that
      // touched more than one of the nodes that went into the supernode,
      // however, because supernode formation does not consolidate edges, the
      // degrees should be unchanged.
    }
  }
}

// TODO - SIGSEGV occurs in gain bucket add when this coarsening method is used.
void PartitionEngineKlfm::CoarsenNeighborhoodInterconnection(
    int max_nodes_per_supernode, int neighbor_limit) {
  assert(neighbor_limit >= 0);
  assert(max_nodes_per_supernode > 0);
  vector<pair<int,int>> node_id_degree_pairs;
  set<int> available_node_ids;
  for (auto node_pair : internal_node_map_) {
    int node_degree = node_pair.second->ports().size();
    node_id_degree_pairs.push_back(make_pair(node_pair.first, node_degree));
    available_node_ids.insert(node_pair.first);
  }
  sort(node_id_degree_pairs.begin(), node_id_degree_pairs.end(),
       &cmp_pair_second_gt);
  // Shuffle ranges with the same degree.
  auto start_it = node_id_degree_pairs.begin();
  while (start_it != node_id_degree_pairs.end()) {
    auto one_past_end_it = start_it;
    while (one_past_end_it != node_id_degree_pairs.end() && 
           start_it->second == one_past_end_it->second) {
      one_past_end_it++;
    }
    shuffle(start_it, one_past_end_it, random_engine_coarsen_);
    start_it = one_past_end_it;
  }

  for (auto id_degree_pair : node_id_degree_pairs) {
    int seed_node_id = id_degree_pair.first;
    auto seed_available_it = available_node_ids.find(seed_node_id);
    if (seed_available_it == available_node_ids.end()) {
      // The seed node has already been used in a different supernode.
      continue;
    }
    // Seed node is not already in a supernode, build a supernode that
    // includes it.
    set<int> nodes_in_supernode;
    nodes_in_supernode.insert(seed_node_id);
    available_node_ids.erase(seed_node_id);
    int supernode_membership_count = 1;
    set<int> neighbor_node_ids;
    Node* seed_node = internal_node_map_.at(seed_node_id);
    for (auto& port_pair : seed_node->ports()) {
      Edge* edge = internal_edge_map_.at(port_pair.second.external_edge_id);
      for (auto neighbor_node_id : edge->connection_ids()) {
        auto vn_it = available_node_ids.find(neighbor_node_id);
        if (vn_it != available_node_ids.end()) {
          available_node_ids.erase(vn_it);
          neighbor_node_ids.insert(neighbor_node_id);
          if (neighbor_limit && neighbor_node_ids.size() >= neighbor_limit) {
            break;
          }
        }
      }
      if (neighbor_limit && neighbor_node_ids.size() >= neighbor_limit) {
        break;
      }
    }
    while (neighbor_node_ids.size() > 0 &&
           supernode_membership_count < max_nodes_per_supernode) {
      // Compute connectivity score for each edge.
      vector<pair<int,int>> node_id_connectivity_pairs;
      for (auto neighbor_id : neighbor_node_ids) {
        Node* neighbor_node = internal_node_map_.at(neighbor_id);
        int supernode_connectivity_weight = 0;
        int supernode_neighbors_connectivity_weight = 0;
        for (auto& port_pair : neighbor_node->ports()) {
          Edge* edge = internal_edge_map_.at(port_pair.second.external_edge_id);
          for (auto connected_node_id : edge->connection_ids()) {
            auto sn_it = nodes_in_supernode.find(connected_node_id);
            if (sn_it != nodes_in_supernode.end()) {
              supernode_connectivity_weight += edge->weight;
            }
            auto nb_it = neighbor_node_ids.find(connected_node_id);
            if (nb_it != neighbor_node_ids.end()) {
              supernode_neighbors_connectivity_weight += edge->weight;
            }
          }
        }
        // The connectivity score will give heavier weight to connections
        // to nodes that are already locked into the supernode, but also
        // consider connectivity to potential members of the supernode.
        int cx_score = 10 * supernode_connectivity_weight +
                       supernode_neighbors_connectivity_weight;
        node_id_connectivity_pairs.push_back(make_pair(neighbor_id, cx_score));
      }
      assert(!node_id_connectivity_pairs.empty());
      // Get the max score;
      auto max_it = node_id_connectivity_pairs.begin();
      for (auto pair_it = node_id_connectivity_pairs.begin();
           pair_it != node_id_connectivity_pairs.end(); pair_it++) {
        if (pair_it->second > max_it->second) {
          max_it = pair_it;
        }
      }
      // Add the node with the max score to the supernode;
      neighbor_node_ids.erase(max_it->first);
      nodes_in_supernode.insert(max_it->first);
      supernode_membership_count++;
      // Add all the neighbors of the new node to the set of neighbors.
      Node* newly_added_node = internal_node_map_.at(max_it->first);
      for (auto& port_pair : newly_added_node->ports()) {
        Edge* edge = internal_edge_map_.at(port_pair.second.external_edge_id);
        for (auto neighbor_node_id : edge->connection_ids()) {
          auto vn_it = available_node_ids.find(neighbor_node_id);
          if (vn_it != available_node_ids.end()) {
            available_node_ids.erase(vn_it);
            neighbor_node_ids.insert(neighbor_node_id);
            if (neighbor_limit && neighbor_node_ids.size() >= neighbor_limit) {
              break;
            }
          }
        }
        if (neighbor_limit && neighbor_node_ids.size() >= neighbor_limit) {
          break;
        }
      }
    }
    if (supernode_membership_count > 1) {
      MakeSupernode(
          nodes_in_supernode, &internal_node_map_, &internal_edge_map_, NULL);
    }
  }
}

void PartitionEngineKlfm::CoarsenHierarchalInterconnection(
    int max_nodes_per_supernode, int neighbor_limit) {
  assert(neighbor_limit >= 0);
  assert(max_nodes_per_supernode > 0);

  map<int,int> node_id_to_current_supernode_index;
  vector<set<int>> supernode_id_sets;

  // The vector acts as a dynamic bitset and is used to improve the run-time
  // of coarsening by replacing several O(log(N)) operations with O(1) ones.
  vector<bool> supernode_indices_is_finalized;
  unordered_set<int> finalized_supernode_indices;
  unordered_set<int> non_finalized_supernode_indices;

  supernode_id_sets.resize(internal_node_map_.size());
  int insert_index = 0;
  for (auto node_pair : internal_node_map_) {
    supernode_id_sets[insert_index].insert(node_pair.first);
    node_id_to_current_supernode_index.insert(
        make_pair(node_pair.first, insert_index));
    insert_index++;
  }
  supernode_indices_is_finalized.assign(supernode_id_sets.size(), false);
  for (int i = 0; i < supernode_id_sets.size(); i++) {
    non_finalized_supernode_indices.insert(i);
  }

  // This loop uses an iterator that scans from beginning to end, possibly
  // multiple times. Care must be taken when the container is modified to
  // ensure the iterator is not made invalid.
  auto scanning_it = non_finalized_supernode_indices.begin();
  while (!non_finalized_supernode_indices.empty()) {
    int sn_index = *scanning_it;
    set<int> viable_neighbor_indices;
    for (auto node_id : supernode_id_sets[sn_index]) {
      Node* seed_node = internal_node_map_.at(node_id);
      for (auto& port_pair : seed_node->ports()) {
        Edge* edge = internal_edge_map_.at(port_pair.second.external_edge_id);
        for (auto neighbor_node_id : edge->connection_ids()) {
          int neighbor_sn_index =
              node_id_to_current_supernode_index.at(neighbor_node_id);
          if (neighbor_sn_index != sn_index) {
            if (!supernode_indices_is_finalized[neighbor_sn_index]) {
              int potential_size = supernode_id_sets[sn_index].size() +
                                   supernode_id_sets[neighbor_sn_index].size();
              if (potential_size <= max_nodes_per_supernode) {
                viable_neighbor_indices.insert(neighbor_sn_index);
                if (neighbor_limit != 0 &&
                    viable_neighbor_indices.size() >= neighbor_limit) {
                  break;
                }
              }
            }
          }
        }
        if (neighbor_limit != 0 &&
            viable_neighbor_indices.size() >= neighbor_limit) {
          break;
        }
      }
    }
    if (viable_neighbor_indices.empty()) {
      finalized_supernode_indices.insert(sn_index);
      scanning_it++;
      non_finalized_supernode_indices.erase(sn_index);
      supernode_indices_is_finalized[sn_index] = true;
      if (scanning_it == non_finalized_supernode_indices.end()) {
        scanning_it = non_finalized_supernode_indices.begin();
      }
    } else {
      vector<pair<int,int>> sn_index_cx_weight_pairs;
      for (auto neighbor_index : viable_neighbor_indices) {
        int cx_score = 0;
        for (auto neighbor_id : supernode_id_sets[neighbor_index]) {
          Node* neighbor_node = internal_node_map_.at(neighbor_id);
          int supernode_connectivity_weight = 0;
          int supernode_neighbors_connectivity_weight = 0;
          for (auto& port_pair : neighbor_node->ports()) {
            Edge* edge =
                internal_edge_map_.at(port_pair.second.external_edge_id);
            for (auto connected_node_id : edge->connection_ids()) {
              int connected_index =
                  node_id_to_current_supernode_index.at(connected_node_id);
              if (connected_index == sn_index) {
                supernode_connectivity_weight += edge->weight;
              } else {
                auto nb_it = viable_neighbor_indices.find(connected_index);
                if (nb_it != viable_neighbor_indices.end()) {
                  supernode_neighbors_connectivity_weight += edge->weight;
                }
              }
            }
          }
          // The connectivity score will give heavier weight to connections
          // to nodes that are already locked into the supernode, but also
          // consider connectivity to potential members of the supernode.
          cx_score += 10 * supernode_connectivity_weight +
                      supernode_neighbors_connectivity_weight;
        }
        int neighbor_size = supernode_id_sets.at(neighbor_index).size();
        if (cx_score > 0) {
          int connection_ratio = neighbor_size / cx_score;
          sn_index_cx_weight_pairs.push_back(
              make_pair(neighbor_index, connection_ratio));
        } else {
          // 24 here is arbitrary. Want to make it finite so selection favors
          // those with smaller neighbor sizes, but large enough that
          // connectivity usually dominates unless the connected supernode
          // is much larger than one of the alternatives.
          sn_index_cx_weight_pairs.push_back(
              make_pair(neighbor_index, 24 * neighbor_size));
        }
      }
      auto min_it = sn_index_cx_weight_pairs.begin();
      for (auto sicwp_it = sn_index_cx_weight_pairs.begin();
           sicwp_it != sn_index_cx_weight_pairs.end(); sicwp_it++) {
        if (sicwp_it->second < min_it->second) {
          min_it = sicwp_it;
        }
      }
      // Take the nodes from min_it and merge them into the seed supernode.
      for (auto to_move_node_id : supernode_id_sets.at(min_it->first)) {
        supernode_id_sets[sn_index].insert(to_move_node_id);
      }
      supernode_indices_is_finalized[min_it->first] = true;
      non_finalized_supernode_indices.erase(min_it->first);
      scanning_it++;
      if (scanning_it == non_finalized_supernode_indices.end()) {
        scanning_it = non_finalized_supernode_indices.begin();
      }
    }
  }

  VLOG(0) << finalized_supernode_indices.size() << " finalized sets." << endl;
  for (auto sn_index : finalized_supernode_indices) {
    set<int>& sn_set = supernode_id_sets[sn_index];
    if (sn_set.size() > 1) {
      MakeSupernode(sn_set, &internal_node_map_, &internal_edge_map_, NULL);
    }
  }
}

void PartitionEngineKlfm::PrintWeightImbalanceFraction(
    const vector<int>& balance, const vector<int>& max_imbalance) {
  assert(balance.size() == max_weight_imbalance_.size());
  for (int i = 0; i < balance.size(); i++) {
    if (i != 0) {
      os_ << " ";
    }
    os_ << (double(abs(balance[i])) / double(max_imbalance[i]));
  }
}

void PartitionEngineKlfm::CoarsenSimple(int num_nodes_per_supernode) {
  set<int> not_yet_processed_node_ids;
  set<int> processed_node_ids;
  for (auto node_pair : internal_node_map_) {
    not_yet_processed_node_ids.insert(node_pair.first);
  }
  while (!not_yet_processed_node_ids.empty()) {
    set<int> component_nodes;
    int node_id = *not_yet_processed_node_ids.begin();
    Node* node = internal_node_map_.at(node_id);
    component_nodes.insert(node_id);
    int found_nodes = 1;
    for (auto& port_pair : node->ports()) {
      int edge_id = port_pair.second.external_edge_id;
      Edge* edge = internal_edge_map_.at(edge_id);
      for (auto cnx_node_id : edge->connection_ids()) {
        if (not_yet_processed_node_ids.count(cnx_node_id) != 0 &&
            component_nodes.count(cnx_node_id) == 0) {
          component_nodes.insert(cnx_node_id);
          found_nodes++;
        }
        if (found_nodes == num_nodes_per_supernode) {
          break;
        }
      }
      if (found_nodes == num_nodes_per_supernode) {
        break;
      }
    }
    for (auto id : component_nodes) {
      not_yet_processed_node_ids.erase(id);
      processed_node_ids.insert(id);
    }
    int supernode_id = MakeSupernode(
        component_nodes, &internal_node_map_, &internal_edge_map_, NULL);
    processed_node_ids.insert(supernode_id);
  }
}

bool PartitionEngineKlfm::DeCoarsen() {
  // De-coarsening will alter internal_node_map_, therefore we cannot directly
  // iterate on it.
  vector<int> unprocessed_node_ids;
  for (auto it : internal_node_map_) {
    unprocessed_node_ids.push_back(it.first);
  }
  bool found_supernode = false;
  for (auto node_id : unprocessed_node_ids) {
    found_supernode |= ExpandSupernode(
        node_id, &internal_node_map_, &internal_edge_map_, NULL);
  }
  return found_supernode;
}

int PartitionEngineKlfm::MakeSupernode(const NodeIdSet& component_nodes,
    KlfmNodeMap* node_map, KlfmEdgeMap* edge_map, PortMap* port_map) {

  int num_nodes = component_nodes.size();
  assert(num_nodes != 0);
  if (num_nodes == 1) {
    // No need to make a supernode if only one node.
    return *component_nodes.begin();
  }

  int supernode_id = IdManager::AcquireNodeId();
  Node* supernode = new Node(supernode_id);
  ostringstream oss;

  EdgeIdSet touching_edges;
  for (auto node_id : component_nodes) {
    // Make a set of all edges that touch the component nodes.
    Node* node = node_map->at(node_id);
    for (auto& port_pair : node->ports()) {
      touching_edges.insert(port_pair.second.external_edge_id);
    }
  }
  assert(!touching_edges.empty());

  // Check if edges touching each component node are wholly internal to the
  // supernode (i.e. do they only connect other component nodes)
  EdgeIdSet supernode_internal_edges;
  EdgeIdSet supernode_external_edges;
  bool is_wholly_internal;
  for (auto edge_id : touching_edges) {
    is_wholly_internal = true;
    Edge* edge = edge_map->at(edge_id);
    for (auto cnx_id : edge->connection_ids()) {
      if (component_nodes.count(cnx_id) == 0) {
        supernode_external_edges.insert(edge_id);
        is_wholly_internal = false;
        break;
      }
    }
    if (is_wholly_internal) {
      supernode_internal_edges.insert(edge_id);
    }
  }

  // Cut the edges that span the boundaries of the supernode, making a new
  // edge with a new id to take its place in this node and moving the old
  // edge into the supernode.
  SplitSupernodeBoundaryEdges(supernode, component_nodes,
      supernode_external_edges, node_map, edge_map, port_map);

  // Move wholly internal edges from this node to the supernode.
  for (auto sn_int_edge_id : supernode_internal_edges) {
    Edge* sn_int_edge = edge_map->at(sn_int_edge_id);
    supernode->internal_edges().insert(make_pair(sn_int_edge_id, sn_int_edge));
    edge_map->erase(sn_int_edge_id);
  }

  // Move all component nodes to the supernode.
  for (auto c_node_id : component_nodes) {
    supernode->internal_nodes().insert(
        make_pair(c_node_id, node_map->at(c_node_id)));
    node_map->erase(c_node_id);
  }

  // Set the Supernode's weight vectors.
  vector<int> default_weight_vector = supernode->SelectedWeightVector();
  supernode->PopulateSupernodeWeightVectors(
      total_weight_, options_.restrict_supernodes_to_default_implementation,
      options_.supernode_implementations_cap);
  vector<int> newly_selected_weight_vector = supernode->SelectedWeightVector();
  UpdateTotalWeightsForImplementationChange(
      default_weight_vector, newly_selected_weight_vector);
  assert(!supernode->WeightVectors().empty());

  // Supernode is finished. Insert it as one of the internal nodes.
  node_map->insert(make_pair(supernode_id, supernode));

  // Debug.
  assert(supernode->is_supernode());
  //assert(!supernode->ports().empty());
  return supernode_id;
}

void PartitionEngineKlfm::UpdateTotalWeightsForImplementationChange(
    const std::vector<int>& old_weight_vector,
    const std::vector<int>& new_weight_vector) {
  assert(old_weight_vector.size() == new_weight_vector.size());
  for (int i = 0; i < old_weight_vector.size(); i++) {
    total_weight_[i] += (new_weight_vector[i] - old_weight_vector[i]);
    assert(total_weight_[i] >= 0);
    if (options_.constrain_balance_by_resource[i]) {
      max_weight_imbalance_[i] =
          (int)(total_weight_[i] * options_.max_imbalance_fraction[i]);
      if (max_weight_imbalance_[i] == 0) {
        max_weight_imbalance_[i] = 1;
      }
    }
  }
}

void PartitionEngineKlfm::SplitSupernodeBoundaryEdges(
    Node* supernode, const NodeIdSet& component_nodes,
    const EdgeIdSet& boundary_edges, KlfmNodeMap* node_map,
    KlfmEdgeMap* edge_map, PortMap* port_map) {
  int supernode_id = supernode->id;

  for (auto edge_id : boundary_edges) {
    // Move edge to supernode.
    Edge* edge = edge_map->at(edge_id);
    supernode->internal_edges().insert(make_pair(edge_id, edge));
    edge_map->erase(edge_id);

    // Make new edge.
    int new_edge_id = IdManager::AcquireEdgeId();
    EdgeKlfm* new_boundary_edge = new EdgeKlfm(
        new_edge_id, edge->weight, edge->GenerateSplitEdgeName(new_edge_id));
        //new_edge_id, edge->weight, "");
    edge_map->insert(make_pair(new_edge_id, new_boundary_edge));
    new_boundary_edge->AddConnection(supernode_id); 

    // Create port inside supernode and update connection IDs.

    // This vector contains the IDs of Nodes/Ports that will need to be updated
    // to reflect the change in the ID of the edge that connects to them.
    vector<int> entities_to_update;

    // Transfer all nodes/ports not in the supernode from the old edge to the
    // new.
    vector<int> entities_to_remove_from_edge;

    for (auto entity_id : edge->connection_ids()) {
      // If the node/port is not one of the components that make up
      // the new supernode, the edge connection to it must be split.
      if (component_nodes.count(entity_id) == 0) {
        new_boundary_edge->AddConnection(entity_id);
        entities_to_update.push_back(entity_id);
        entities_to_remove_from_edge.push_back(entity_id);
      }
    }

    // This must not be done in the previous loop to avoid invalidating
    // iterators.
    for (auto removed_entity_id : entities_to_remove_from_edge) {
      edge->RemoveConnection(removed_entity_id);
    }

    // Add supernode port as source for the part of the edge that is inside
    // the supernode.
    int new_port_id = IdManager::AcquireNodeId();
    ostringstream port_oss;
    port_oss << "Supernode_" << supernode_id << "_Port_" << new_port_id;
    edge->AddConnection(new_port_id);
    supernode->ports().insert(make_pair(new_port_id,
        Port(new_port_id, edge_id, new_edge_id, Port::kDontCareType,
              port_oss.str())));

    // Replace references to the old edge id in all nodes/ports not in the
    // supernode with reference to new edge.
    for (auto outdated_entity_id : entities_to_update) {
      if (node_map->count(outdated_entity_id) != 0) {
        Node* outdated_node = node_map->at(outdated_entity_id);
        outdated_node->SwapPortConnection(edge_id, new_edge_id);
        //assert(!((edge_id == 4594) && (outdated_entity_id == 31719)));
      } else if (port_map != NULL && port_map->count(outdated_entity_id) != 0) {
        Port& p = port_map->at(outdated_entity_id);
        p.internal_edge_id = new_edge_id;
      } else {
        printf("Internal error: Could not find entity that needs to be "
               "updated.");
        exit(1);
      }
    }
  }
}

bool PartitionEngineKlfm::ExpandSupernode(
    int supernode_id, KlfmNodeMap* node_map, KlfmEdgeMap* edge_map,
    PortMap* port_map) {

  // Check that 'supernode_id' represents a supernode.
  Node* supernode = node_map->at(supernode_id);
  if (!supernode->is_supernode()) {
    // Not a supernode.
    return false;
  }

  supernode->SetSupernodeInternalWeightVectors();
  RUN_DEBUG(DEBUG_OPT_SUPERNODE_WEIGHT_VECTOR, 1) {
    supernode->CheckSupernodeWeightVectorOrDie();
  }

  // Merge edges on the boundary of the supernode to the rest of the parent
  // node'd internal graph.
  MergeSupernodeBoundaryEdges(supernode, node_map, edge_map, port_map);

  // Transfer supernode's internal graph member to parent node's internal graph.
  for (auto entry : supernode->internal_nodes()) {
    node_map->insert(make_pair(entry.first, entry.second));
  }
  supernode->internal_nodes().clear();
  assert(supernode->internal_nodes().empty());
  for (auto entry : supernode->internal_edges()) {
    edge_map->insert(
        make_pair(entry.first, reinterpret_cast<EdgeKlfm*>(entry.second)));
  }
  supernode->internal_edges().clear();
  assert(supernode->internal_edges().empty());

  // Remove supernode.
  node_map->erase(supernode_id);
  IdManager::ReleaseNodeId(supernode_id);
  delete supernode;

  return true;
}

void PartitionEngineKlfm::MergeSupernodeBoundaryEdges(
    Node* supernode, KlfmNodeMap* node_map, KlfmEdgeMap* edge_map,
    PortMap* port_map) {
  // Iterate through all the ports on 'supernode', replacing all references
  // to the port's external connection edge with the internal edge.

  // Cycle through all the ports, merging edges on each port.
  for (auto& supernode_port_pair : supernode->ports()) {
    Port& supernode_port = supernode_port_pair.second;
    Edge* external_edge = edge_map->at(supernode_port.external_edge_id);
    Edge* internal_edge =
        supernode->internal_edges().at(supernode_port.internal_edge_id);

    // Remove reference to the supernode's port.
    internal_edge->RemoveConnection(supernode_port.id);

    // Transfer references to all of the external edge's connections to the
    // internal edge and update the connecting nodes and ports with the
    // internal edge's ID.
    for (auto entity_id : external_edge->connection_ids()) {
      if (entity_id != supernode->id) {
        internal_edge->AddConnection(entity_id);
        if (node_map->count(entity_id) != 0) {
          node_map->at(entity_id)->SwapPortConnection(
              external_edge->id, internal_edge->id);
        } else if (port_map != NULL && port_map->count(entity_id) != 0) {
          Port p = port_map->at(entity_id);
          p.internal_edge_id = internal_edge->id;
        } else {
          printf("Fatal error: Edge connection not found in internal nodes");
          assert(false);
        }
      }
    }

    // TODO What if two supernodes are connected by this edge? Will it get
    // deleted twice? Will something that shouldn't have been deleted get
    // deleted?
    // Remove external edge.
    auto external_edge_it = edge_map->find(external_edge->id);
    if (external_edge_it != edge_map->end()) {
      edge_map->erase(external_edge->id);
      delete external_edge;
    }
  }
}

void PartitionEngineKlfm::SummarizeResults(
    const vector<PartitionSummary>& summaries) {
  vector<int> costs;
  for (auto& it : summaries) {
    costs.push_back(it.total_cost);
  }
  sort(costs.begin(), costs.end());
  int min_cost = costs.front();
  int max_cost = costs.back();
  int median_index = costs.size() / 2;
  int median_cost = costs[median_index];
  int sum_of_costs = accumulate(costs.begin(), costs.end(), 0);
  double average_cost = (double)sum_of_costs / (double)costs.size();
  double variance_of_costs = 0.0;
  for (auto it : costs) {
    int dif = average_cost - it;
    variance_of_costs += dif * dif / costs.size();
  }
  double std_dev_of_costs = sqrt(variance_of_costs);

  vector<int> passes;
  for (auto& it : summaries) {
    passes.push_back(it.num_passes_used);
  }
  sort(passes.begin(), passes.end());
  int sum_of_passes = accumulate(passes.begin(), passes.end(), 0);
  double average_passes = (double)sum_of_passes / (double)passes.size();
  median_index = passes.size() / 2;
  int median_passes = passes[median_index];

  os_ << endl << "----------------KLFM Results------------------" << endl;
  os_ << "MIN COST: " << min_cost << endl;
  os_ << "MAX COST: " << max_cost << endl;
  os_ << "AVERAGE COST: " << average_cost << endl;
  os_ << "MEDIAN COST: " << median_cost << endl;
  os_ << "STANDARD DEVIATION: " << std_dev_of_costs << endl;
  os_ << "AVERAGE PASSES: " << average_passes << endl;
  os_ << "MEDIAN PASSES: " << median_passes << endl << endl;
  PrintHistogram(costs, false);
  os_ << "SORTED COSTS: " << endl;
  for (auto it : costs) {
    os_ << it << endl;
  }
  os_ << endl;
}

void PartitionEngineKlfm::PrintResultFull(const PartitionSummary& summary,
    int run_num) {

  os_ << endl << "----------------Run Summary------------------" << endl;
  os_ << "Run " << run_num << endl;
  os_ << "Passes: " << summary.num_passes_used << endl;
  os_ << "Cut size: " << summary.total_cost << endl;
  os_ << "RMS Resource Deviation: " << summary.rms_resource_deviation << endl;
  os_ << "Imbalance: ";
  for (auto imb : summary.balance) {
    os_ << imb << " ";
  }
  os_ << endl;
  os_ << "Total Resource Weights: ";
  for (auto wt : summary.total_weight) {
    os_ << wt << " ";
  }
  os_ << endl;
  os_ << "Total Resource Weight Ratio: ";
  for (auto rr : summary.total_resource_ratio) {
    os_ << rr << " ";
  }
  os_ << endl;
  os_ << "Partition A Resource Weight Ratio: ";
  for (auto rr : summary.partition_resource_ratios[0]) {
    os_ << rr << " ";
  }
  os_ << endl;
  os_ << "Partition B Resource Weight Ratio: ";
  for (auto rr : summary.partition_resource_ratios[1]) {
    os_ << rr << " ";
  }
  os_ << endl << endl;

  run_num++;
}

void PartitionEngineKlfm::PrintHistogram(
    const vector<int>& val, bool cummulative) {
  vector<int> values = val;
  sort(values.begin(), values.end());
  int min_val = values.front();
  int max_val = values.back();
  int diff = max_val - min_val;
  int num_buckets;
  if ((diff / 10) > 10) {
    num_buckets = 10;
  } else if ((diff / 5) > 10) {
    num_buckets = 5;
  } else {
    num_buckets = 1;
  }
  vector<int> ceilings;
  int bucket_width = diff / num_buckets + 1;
  for (int i = 0; i < num_buckets; i++) {
    int ceiling = min_val + (i + 1) * bucket_width;
    ceilings.push_back(ceiling);
  }
  vector<int> bucket_counts;
  bucket_counts.assign(num_buckets, 0);
  for (auto it : values) {
    for (int i = 0; i < num_buckets; i++) {
      if (it < ceilings[i]) {
        bucket_counts[i]++;
        if (!cummulative) {
          break;
        }
      }
    }
  }
  for (int i = 0; i < num_buckets; i++) {
    int lower_limit;
    if (i == 0) {
      lower_limit = min_val;
    } else {
      lower_limit = ceilings[i-1];
    }
    os_ << "[" << lower_limit << " - " << ceilings[i] << "] "
        << (100 * bucket_counts[i] / values.size()) << "%" << endl;
  }
}

// The SCIP SOL format only prints variables if they are non-zero. It also
// prints their contribution to the objective function.
void PartitionEngineKlfm::WriteScipSol(const NodePartitions& partitions,
                                       const std::string& base_filename) {
  string filename_with_extension = base_filename + ".sol";
  ofstream of(filename_with_extension.c_str());
  assert(of.is_open());

  // Use this instead of internal_node_map because we want to guarantee
  // order.
  set<int> combined_node_ids;
  for (int id : partitions.first) {
    combined_node_ids.insert(id);
  }
  for (int id : partitions.second) {
    combined_node_ids.insert(id);
  }

  of << "solution status: unknown\n";
  of << "objective value: " << RecomputeCurrentCost() << "\n";
  // Add node identity variables. This will the variable names for the selected
  // partitions and personalities.
  for (int node_id : combined_node_ids) {
    of << "V" << mps_name_hash::Hash(node_id);
    char partition_id =
        (partitions.first.find(node_id) != partitions.first.end()) ? 'A' : 'B';
    of << partition_id;
    int personality_id =
        internal_node_map_.at(node_id)->selected_weight_vector_index();
    of << personality_id;
    // Nodes do not contribute to the objective function.
    of << " 1\t (obj:0)\n";
  }
  // These will not print in order.
  for (pair<int, EdgeKlfm*> ep : internal_edge_map_) {
    EdgeKlfm* edge = CHECK_NOTNULL(ep.second);
    // For edge crossing variables, print the names for any edge that crosses
    // partitions.
    if (edge->CrossesPartitions()) {
      of << "X" << mps_name_hash::Hash(edge->id) << " 1\t (obj:" << edge->weight
         << ")\n";
    }

    // For edge partition connectivity variables, print them if the edge touches
    // the partition. It is not important which partition is denoted as A vs B,
    // as long as we are consistent with what we did for the nodes.
    if (edge->TouchesPartitionA()) {
      of << "C" << mps_name_hash::Hash(edge->id) << "A 1\t (obj:0)\n";
    }
    if (edge->TouchesPartitionB()) {
      of << "C" << mps_name_hash::Hash(edge->id) << "B 1\t (obj:0)\n";
    }
  }
}

void PartitionEngineKlfm::WriteScipSolAlt(const NodePartitions& partitions,
                                          const std::string& base_filename) {
  string filename_with_extension = base_filename + ".sol";
  ofstream of(filename_with_extension.c_str());
  assert(of.is_open());

  // Use this instead of internal_node_map because we want to guarantee
  // order.
  set<int> combined_node_ids;
  for (int id : partitions.first) {
    combined_node_ids.insert(id);
  }
  for (int id : partitions.second) {
    combined_node_ids.insert(id);
  }
  set<int> combined_edge_ids;
  for (pair<int, EdgeKlfm*> p : internal_edge_map_) {
    combined_edge_ids.insert(p.first);
  }

  of << "solution status: unknown\n";
  of << "objective value: " << RecomputeCurrentCost() << "\n";
  // Add node identity variables. This will the variable names for the selected
  // partitions and personalities.
  for (int node_id : combined_node_ids) {
    Node* n = CHECK_NOTNULL(internal_node_map_.at(node_id));
    for (int part = 0; part < 2; ++part) {
      const NodeIdSet& this_partition = (part == 0) ?
          partitions.first : partitions.second;
      bool in_this_partition =
          this_partition.find(node_id) != this_partition.end();
      for (int per = 0; per < n->num_personalities(); ++per) {
        bool uses_this_personality =
            n->selected_weight_vector_index() == per;
        of << "V" << mps_name_hash::Hash(node_id) << (char)('A' + part) << per;
        if (uses_this_personality && in_this_partition) {
          of << " 1";
        } else {
          of << " 0";
        }
        of << "\t (obj:0)\n";
      }
    }
  }
  // These will not print in order.
  for (int edge_id : combined_edge_ids) {
    EdgeKlfm* edge = CHECK_NOTNULL(internal_edge_map_.at(edge_id));
    // For edge crossing variables, print the names for any edge that crosses
    // partitions.
    of << "X" << mps_name_hash::Hash(edge->id);
    if (edge->CrossesPartitions()) {
      of << " 1\t (obj:" << edge->weight << ")\n";
    } else {
      of << " 0\t (obj:0)\n";
    }

    // For edge partition connectivity variables, print them if the edge touches
    // the partition. It is not important which partition is denoted as A vs B,
    // as long as we are consistent with what we did for the nodes.
    of << "C" << mps_name_hash::Hash(edge->id) << "A ";
    if (edge->TouchesPartitionA()) {
      of << "1";
    } else {
      of << "0";
    }
    of << "\t (obj:0)\n";

    of << "C" << mps_name_hash::Hash(edge->id) << "B ";
    if (edge->TouchesPartitionB()) {
      of << "1";
    } else {
      of << "0";
    }
    of << "\t (obj:0)\n";
  }
}

// The Gurobi MST format simply prints the variable name followed by the value
// for all variables.
void PartitionEngineKlfm::WriteGurobiMst(const NodePartitions& partitions,
                                         const std::string& base_filename) {
  string filename_with_extension = base_filename + ".mst";
  ofstream of(filename_with_extension.c_str());
  assert(of.is_open());

  // Use this instead of internal_node_map because we want to guarantee
  // order.
  set<int> combined_node_ids;
  for (int id : partitions.first) {
    combined_node_ids.insert(id);
  }
  for (int id : partitions.second) {
    combined_node_ids.insert(id);
  }

  of << "# MIP start\n";
  // Add node identity variables. This will the variable names for the selected
  // partitions and personalities.
  for (int node_id : combined_node_ids) {
    Node* n = CHECK_NOTNULL(internal_node_map_.at(node_id));
    for (int part = 0; part < 2; ++part) {
      const NodeIdSet& this_partition = (part == 0) ?
          partitions.first : partitions.second;
      bool in_this_partition =
          this_partition.find(node_id) != this_partition.end();
      for (int per = 0; per < n->num_personalities(); ++per) {
        bool uses_this_personality =
            n->selected_weight_vector_index() == per;
        of << "V" << mps_name_hash::Hash(node_id) << (char)('A' + part) << per;
        if (uses_this_personality && in_this_partition) {
          of << " 1\n";
        } else {
          of << " 0\n";
        }
      }
    }
  }
  // These will not print in order.
  for (pair<int, EdgeKlfm*> ep : internal_edge_map_) {
    EdgeKlfm* edge = CHECK_NOTNULL(ep.second);
    // For edge crossing variables, print the names for any edge that crosses
    // partitions.
    of << "X" << mps_name_hash::Hash(edge->id);
    if (edge->CrossesPartitions()) {
      of << " 1\n";
    } else {
      of << " 0\n";
    }

    // For edge partition connectivity variables, print them if the edge touches
    // the partition. It is not important which partition is denoted as A vs B,
    // as long as we are consistent with what we did for the nodes.
    of << "C" << mps_name_hash::Hash(edge->id);
    if (edge->TouchesPartitionA()) {
      of << "A 1\n";
    } else {
      of << "A 0\n";
    }
    of << "C" << mps_name_hash::Hash(edge->id);
    if (edge->TouchesPartitionB()) {
      of << "B 1\n";
    } else {
      of << "B 0\n";
    }
  }
}
