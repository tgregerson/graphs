#ifndef PARTITION_ENGINE_KLFM_H_
#define PARTITION_ENGINE_KLFM_H_

/* Engine for performing Kernighan-Lin / Fiduccia-Mattheyes partitioning on a
   graph/hypergraph. This class is NOT thread-safe. */

#include "partition_engine.h"

#include <cstddef>
#include <ctime>
#include <iostream>
#include <fstream>
#include <functional>
#include <list>
#include <queue>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "edge_klfm.h"
#include "gain_bucket_entry.h"
#include "gain_bucket_manager.h"
#include "node.h"
#include "partitioner_config.h"

class PartitionEngineKlfm : public PartitionEngine {
 public:
  class Options;

  typedef EdgeKlfm::NodeIdSet NodeIdSet;
  typedef EdgeKlfm::NodeIdVector NodeIdVector;
  typedef Node::EdgeIdSet EdgeIdSet;
  typedef Node::PortMap PortMap;
  typedef std::unordered_map<int, Node*> KlfmNodeMap;
  typedef std::unordered_map<int, EdgeKlfm*> KlfmEdgeMap;
  typedef std::pair<NodeIdSet, NodeIdSet> NodePartitions;
  typedef std::pair<NodeIdVector, NodeIdVector> NodeVectorPair;
  typedef std::unordered_map<int, NodePartitions> NodePartitionsMap;
  typedef std::vector<std::pair<NodeIdSet, NodeIdSet>>
      NodePartitionsVector;
  typedef std::map<int, NodeVectorPair> NodeVectorPairMap;

  PartitionEngineKlfm(Node* graph, Options& options, std::ostream& os);
  virtual ~PartitionEngineKlfm();

  // Execute may be called multiple times for a given partition engine, but
  // may not be called concurrently.
  virtual void Execute(std::vector<PartitionSummary>* summaries);

  class Options {
   public:
    // Determines the mechanism for obtaining the initial partition for each
    // run. If 'kSeedModeUserSpecified' is chose, then the user must also
    // populate 'initial_a_nodes' and 'initial_b_nodes' with the IDs of the
    // nodes in each partition. These IDs must correspond to the IDs in the
    // graph passed to the constructor.
    typedef enum {
      kSeedModeRandom,
      kSeedModeUserSpecified,
      kSeedModeSimpleDeterministic
    } SeedMode;

    Options()
      : seed_mode(kSeedModeRandom),
        gain_bucket_type(PartitionerConfig::kGainBucketSingleResource),
        gain_bucket_selection_policy(
            PartitionerConfig::kGbmreSelectionPolicyLargestGain),
        use_ratio_in_imbalance_score(false),
        use_ratio_in_partition_quality(false),
        cap_passes(false),
        max_passes(100),
        num_runs(5),
        use_adaptive_node_implementations(false),
        use_multilevel_constraint_relaxation(false),
        restrict_supernodes_to_default_implementation(false),
        supernode_implementations_cap(16),
        reuse_previous_run_implementations(true),
        enable_mutation(false),
        mutation_rate(0),
        rebalance_on_start_of_pass(false),
        rebalance_on_end_of_run(false),
        rebalance_on_demand(false),
        rebalance_on_demand_cap_per_run(1),
        rebalance_on_demand_cap_per_pass(1),
        num_resources_per_node(1),
        enable_print_output(true),
        multilevel(true),
        export_initial_sol_only(false),
        sol_scip_format(true),
        sol_gurobi_format(false),
        use_entropy(false),
        save_cutset(true) {
      max_imbalance_fraction.insert(max_imbalance_fraction.begin(),     
                                    num_resources_per_node, 0.05);
      constrain_balance_by_resource.insert(
          constrain_balance_by_resource.begin(), num_resources_per_node, true);
    }

    explicit Options(int num_resources)
      : seed_mode(kSeedModeRandom),
        gain_bucket_type(PartitionerConfig::kGainBucketMultiResourceExclusive),
        gain_bucket_selection_policy(
            PartitionerConfig::kGbmreSelectionPolicyLargestGain),
        use_ratio_in_imbalance_score(false),
        use_ratio_in_partition_quality(false),
        cap_passes(false),
        max_passes(100),
        num_runs(5),
        use_adaptive_node_implementations(false),
        use_multilevel_constraint_relaxation(false),
        restrict_supernodes_to_default_implementation(false),
        supernode_implementations_cap(16),
        reuse_previous_run_implementations(true),
        enable_mutation(false),
        mutation_rate(0),
        rebalance_on_start_of_pass(false),
        rebalance_on_end_of_run(false),
        rebalance_on_demand(false),
        rebalance_on_demand_cap_per_run(1),
        rebalance_on_demand_cap_per_pass(1),
        num_resources_per_node(num_resources),
        enable_print_output(true),
        multilevel(true),
        export_initial_sol_only(false),
        sol_scip_format(true),
        sol_gurobi_format(false),
        use_entropy(false),
        save_cutset(true) {
      max_imbalance_fraction.insert(max_imbalance_fraction.begin(),     
                                    num_resources_per_node, 0.05);
      constrain_balance_by_resource.insert(
          constrain_balance_by_resource.begin(), num_resources_per_node, true);
    }

    // Prints the current configuration to 'os'.
    void Print(std::ostream& os);

    void PopulateFromPartitionerConfig(const PartitionerConfig& config);

    SeedMode seed_mode;
    NodeIdSet initial_a_nodes, initial_b_nodes;

    PartitionerConfig::GainBucketType gain_bucket_type;
    PartitionerConfig::GainBucketSelectionPolicy
        gain_bucket_selection_policy;

    // One entry to reach resource. If set to false, that resource is excluded
    // from balance constraints.
    std::vector<bool> constrain_balance_by_resource;

    // Caps the maximum imbalance in node weights that is allowed between the
    // two partitions.
    std::vector<double> max_imbalance_fraction;

    std::vector<int> target_total_weight;

    // Used for balancing resource ratios.
    bool use_ratio_in_imbalance_score;
    bool use_ratio_in_partition_quality;
    std::vector<int> resource_ratio_weights;

    // Setting true ends a run early if it has taken more than max_passes to
    // hit a point of no improvement.
    bool cap_passes;
    size_t max_passes;

    // Algorithm will run 'num_runs' times and return the best results.
    // If the algorithm does not contain a random element, there is no reason
    // to set above 1.
    size_t num_runs;

    // Indicates whether gain buckets can select between multiple
    // node implementations. Note that if this option is set to false,
    // node implmentations may still be changed by rebalancing or mutation.
    bool use_adaptive_node_implementations;

    // If enabled, will set a larger allowable imbalance in resources (other
    // than resource 0) during coarse partitioning, then switch to the normal
    // constaint for fine partitioning.
    bool use_multilevel_constraint_relaxation;

    // If set to true, supernodes are only allowed a single weight vector that
    // is the sum of the selected weight vectors of its component nodes.
    bool restrict_supernodes_to_default_implementation;

    // Sets a limit on the number of weight vectors generated for a supernode.
    // This cap is only honored if it is greater 1 + the number of resources.
    size_t supernode_implementations_cap;

    // If set to true and 'num_runs > 1', subsequent runs after the initial one
    // will use the node weight vectors that were set at the end of the previous
    // run as the starting vectors. If set to false, the weight vectors will be
    // reset to those used at the beginning of the initial run.
    bool reuse_previous_run_implementations;

    // If set to true, allows nodes with multiple implementations to have their
    // selected weight vector randomly changed during a mutation phase.
    bool enable_mutation;

    // Represents the percentage chance that a node with multiple
    // implementations will be mutated during mutation phase. Should be between
    // 0 and 100.
    unsigned int mutation_rate;

    // If set to true, node implementations will be modified at the start of
    // each pass to improve balance and/or ratio.
    bool rebalance_on_start_of_pass;

    // If set to true, node implementations will be modified at the completion
    // of a run to improve balance and/or ratio.
    bool rebalance_on_end_of_run;

    // If set to true, node implementations will be modified when balance and/or
    // ratio constraints are violated to attempt to improve them.
    bool rebalance_on_demand;

    // Limits the number of on-demand rebalances that are allowed per run.
    // Setting to zero removes the limit.
    size_t rebalance_on_demand_cap_per_run;

    // Limits the number of on-demand rebalances that are allowed per pass.
    // Setting to zero removes the limit. Note: Allowing unlimited rebalances
    // in a pass increases the algorithm complexity to O(N^2).
    size_t rebalance_on_demand_cap_per_pass;

    size_t num_resources_per_node;
    std::vector<int> device_resource_capacities;

    // Controls whether the algorithm prints its output.
    bool enable_print_output;

    // Controls whether the algorithm performs multi-level partitioning.
    bool multilevel;

    // If non-empty, will output a basic testbench skeleton for monitoring the
    // signal values of edges included in the cutset at the end of partitioning.
    std::string testbench_filename;

    // If non-empty, will output the initial solution to the filename, with an
    // extension based on the format.
    std::string initial_sol_base_filename;

    // If non-empty, will output the final solution to the filename, with an
    // extension based on the format.
    std::string final_sol_base_filename;

    // If set to true, will write the initial .SOL file after the first pass
    // that meets balance constraints, then exit. 'initial_sol_filename' must
    // be non-empty.
    bool export_initial_sol_only;

    // If more than one solution format is set, will write multiple files with
    // different extensions.

    // Will write solutions in the SCIP .SOL format.
    bool sol_scip_format;

    // Will write solutions in the Gurobi .MST format.
    bool sol_gurobi_format;

    // Use edge entropy to determine move cost.
    bool use_entropy;

    // If set to false, edge names and node IDs that define a cutset will
    // not be stored in the partition summary. This is a memory-saving
    // technique used when performing a large number of runs if it is not
    // necessary to print the cutset.
    bool save_cutset;
  };

 private:
  void AppendPartitionSummary(
    std::vector<PartitionSummary>* summaries, const NodePartitions& partitions,
    std::vector<int>& current_partition_balance, double current_partition_cost,
    int num_passes);

  // Verifies that all of weight vectors for every node in the node map have
  // the same number of entries as num_resources_per_node_;
  void CheckSizeOfWeightVectors();

  void StoreInitialImplementations(std::map<int,int>* initial_implementations)
    const;

  void ResetImplementations(const std::map<int,int>& initial_implementations);

  void ExecuteRun(int cur_run, std::vector<PartitionSummary>* summaries);

  // Returns number of passes taken.
  int RunKlfmAlgorithm(int cur_run, NodePartitions& current_partition,
      double& current_partition_cost, std::vector<int>& current_partition_balance);

  void ExecutePass(NodePartitions& current_partition,
      double& current_partition_cost, std::vector<int>& current_partition_balance,
      bool& partition_changed);

  // Reset any execution-specific state to allow Execute() to be called again.
  void Reset();

  // Print progress information on current status.
  void PrintPassInfo(int cur_pass, int cur_run);

  // Prints a representation of the imbalances of each resource in 'balance' as
  // a decimal fraction of the total in the corresponding resource in
  // 'max_imbalance'. Does not print an end-line.
  void PrintWeightImbalanceFraction(
      const std::vector<int>& balance, const std::vector<int>& max_imbalance);

  // Resets state of nodes and edges for the beginning of a KLFM iteration.
  // Unlocks all nodes, resets edges and their criticality, and computes initial
  // node gains. KLFM helper fn.
  void ResetNodeAndEdgeKlfmState(const NodePartitions& current_partition);

  // Performs one node move for the KLFM algorithm.
  void MakeKlfmMove(
      std::vector<int>& current_partition_balance,
      double& current_partition_cost,
      std::vector<int>& best_cost_balance_by_pass,
      double& best_cost_by_pass,
      double& best_cost_br_power_by_pass,
      NodePartitions& current_partition,
      std::vector<int>& nodes_moved_since_best_result);

  // Handles operations at the end of a profiling interval (printing data,
  // reseting variables, etc.).
  void RefreshProfilingData(bool record_previous_results);

  // Computes the initial gain of 'node' and adds it to the appropriate bucket.
  // 'in_part_a' indicates if the node is in Partition A or B. KLFM helper fn.
  void ComputeInitialNodeGainAndUpdateBuckets(Node* node, bool in_part_a);

  // Compute the gain for 'node_id'. KLFM helper fn.
  double ComputeNodeGain(int node_id);

  // Moves node (to 'part_b' if 'from_part_a' is true, else to 'part_a') and
  // updates 'balance' according to the change in weight. KLFM helper fn.
  void MoveNodeAndUpdateBalance(
      bool from_part_a, NodePartitions& current_partition, Node* node,
      const std::vector<int>& weight_vector,
      const std::vector<int>& prev_weight_vector, std::vector<int>& balance);

  // Updates the edges connected to 'moved_node' and change the gain on all
  // nodes connected to those edges. KLFM helper fn.
  void UpdateMovedNodeEdgesAndNodeGains(Node* moved_node, bool from_part_a);

  // Moves all nodes in 'nodes_moved_since_best_result' to the opposite
  // node set they are currently in, according to 'current_a_nodes' and
  // 'current_b_nodes'. Updates current cost and balance to the best
  // cost and balance for the current pass. 'nodes_moved_since_best_result'
  // is cleared. KLFM helper fn.
  void RollBackToBestResultOfPass(
      std::vector<int>& nodes_moved_since_best_result,
      NodePartitions& current_partition,
      double& current_partition_cost, std::vector<int>& current_partition_balance,
      const double& best_cost, const std::vector<int>& best_cost_balance);

  // Removes ports from node and edge sets. Removes any edges that no
  // longer have both a source and sink. NOTE: The pointers to any removed
  // edges will be deleted, so the caller must own the edge pointers in order
  // to use this method.
  void StripPorts(KlfmNodeMap* node_map, KlfmEdgeMap* edge_set,
                  const NodeIdSet& port_ids);

  // Populates the partition-specific node id data structures in all internal
  // edges.
  void PopulateEdgePartitionConnections(const NodePartitions& current_partition);

  // Returns true if the two pairs of node sets are the same. Order of 
  // node sets does not matter.
  bool PartitionsIdentical(const NodePartitions& a,
                           const NodePartitions& b) const;

  // Generates the initial partition by dividing all nodes between 'part_a' and
  // 'part_b'. The method used to generate the partition is dependent on the
  // seed mode set in KlfmPartitionEngine::Options. Also returns the cost
  // and balance of the initial partition.
  void GenerateInitialPartition(NodePartitions* partition,
                                double* cost, std::vector<int>* balance);
  void GenerateInitialPartitionRandom(NodePartitions* partition,
                                      double* cost, std::vector<int>* balance);
  void GenerateInitialPartitionRandomEntropyAware(
      NodePartitions* partition, double* cost, std::vector<int>* balance);
  // Always returns the same initial partition for a given graph. Used for
  // debugging purposes.
  void GenerateInitialPartitionSimpleDeterministic(
      NodePartitions* partition, double* cost, std::vector<int>* balance);

  // Attempts to correct a weight imbalance that exceeds the maximum allowable
  // imbalance, either by moving a node between partitions or by changing
  // a node's implementation. Updates 'part_a', 'part_b', and 'current_balance'
  // accordingly. Not safe to call during a KLFM iteration, as it does not
  // consider whether nodes are locked or update KLFM state.
  void FixInitialWeightImbalance(NodePartitions* partition,
                                 std::vector<int>* part_a_current_weight,
                                 std::vector<int>* part_b_current_weight,
                                 std::vector<int>* current_balance);

  // Returns true is the 'current_balance' exceeds the max imbalance for any
  // of the resources.
  bool ExceedsMaxWeightImbalance(const std::vector<int>& current_balance) const;

  int CurrentSpan();
  double CurrentEntropy();
  void RecomputeTotalWeightAndMaxImbalance();
  double RecomputeCurrentCost();
  double ComputeEdgeCost(const EdgeKlfm& edge);
  std::vector<int> RecomputeCurrentBalance(
      const NodePartitions& current_partition);
  std::vector<int> RecomputeCurrentBalanceAtBaseLevel(
      const NodePartitions& current_partition);
  void GetCutSet(const NodePartitions& current_partition,
                 std::set<int>* cut_set);
  void GetCutSetNames(const NodePartitions& current_partition,
                      std::set<std::string>* cut_set);

  // Coarsen the graph, making supernodes composed of as many as
  // 'num_nodes_per_supernode', based on the nodes currently in the
  // internal node map. Note that the component nodes may already be supernodes,
  // but are only counted as a single node as far as 'num_nodes_per_supernode'.
  void CoarsenSimple(int num_nodes_per_supernode);

  void CoarsenMaxEdgeDegree(int max_nodes_per_supernode);
  void CoarsenMaxNodeDegree(int max_nodes_per_supernode);
  // 'neighbor_limit' sets a limit on the number of neighboring nodes to be
  // considered when forming a supernode. It prevents unacceptable worst-case
  // performance in large, heavily-connected graphs. Setting to 0 disables the
  // limit.
  // Neighborhood is greedy. Tends to form a few good clusters, but
  // generally the average cluster size is much lower than the max size and
  // many nodes remain unclustered. Generally fast performance.
  void CoarsenNeighborhoodInterconnection(
      int max_nodes_per_supernode, int neighbor_limit);
  // Hierarchal lets each node(set) make one decision per pass to
  // partner with another node(set) and continues making passes until no more
  // consolidation can be made. Tends to coarsen the graph to a higher degree
  // than Neighborhood.
  void CoarsenHierarchalInterconnection(
      int max_nodes_per_supernode, int neighbor_limit);

  // Transfers nodes from 'coarsened' to 'decoarsened', breaking them down
  // into component nodes if they are supernodes.
  void DecoarsenPartitions(
      NodePartitions* coarsened, NodePartitions* decoarsened);

  // Decoarsen a single partition.
  void DecoarsenPartition(
      NodeIdSet* coarsened, NodeIdSet* decoarsened);


  // De-Coarsen the graph by one level - i.e. break every supernode in the
  // internal node map down once. Returns false if none of the nodes in the
  // internal node map was a supernode.
  bool DeCoarsen();

  // Attempts to improve the partition's imbalance by changing the
  // implementation of the nodes.
  void RebalanceImplementations(const NodePartitions& current_partition,
                                std::vector<int>& partition_imbalance,
                                bool use_imbalance, bool use_ratio);

  // For nodes in 'internal_node_map_' that have multiple implementations,
  // there is a 'mutation_rate'/100 chance that the implementation will be
  // randomly set to one of the other implementations.
  // No guarantees are given that any current partitions will still be
  // balanced after mutation. Does not update partition balance, so that
  // must be done manually if this method is called during a partitioning
  // run.
  void MutateImplementations(int mutation_rate);

  // Creates a supernode that contains the nodes specifed by the IDs in
  // 'component_nodes'. The nodes and edges connected to them must be present
  // in 'node_map' and 'edge_map' respectively. The two maps will be updated
  // to remove the consolidated nodes and edges and insert the supernode and
  // any its boundary edges. The supernode gains ownership of the nodes and
  // edges that are removed from the maps. 'port_map' must contain the ports
  // of the graph/parent node of 'component_nodes' or NULL if none of the
  // nodes in 'component_nodes' has an edge that connects to one of the
  // parent's ports. Returns the ID of the supernode.
  int MakeSupernode(const NodeIdSet& component_nodes,
      KlfmNodeMap* node_map, KlfmEdgeMap* edge_map, PortMap* port_map);

  // Breaks a supernode into its component internal graph. This only breaks
  // down a single level of hierarchy; to completely reduce the hierarchy, it
  // must be recursively executed on any supernodes present in this supernode's
  // internal graph. The supernode and its connected edges must be present in
  // 'node_map' and 'edge_map' respectively. These maps are updated to include
  // the components of the supernode and remove the supernode and any
  // redundant edge entries. These objects are also deleted.
  // Returns false and does not otherwise alter the graph if 'supernode_id'
  // denotes a valid node in 'node_map' that is not a supernode.
  static bool ExpandSupernode(int supernode_id, KlfmNodeMap* node_map,
      KlfmEdgeMap* edge_map, PortMap* port_map);

  // Computes the difference of 'old_weight_vector' and 'new_weight_vector'
  // and adjusts 'total_weights_' by that amount.
  void UpdateTotalWeightsForImplementationChange(
      const std::vector<int>& old_weight_vector,
      const std::vector<int>& new_weight_vector);

  // Split edges that are not wholly internal to the supernode. Keep the
  // original id on the portion that is internal to the supernode and assign
  // a new id to the external part(s), update the nodes connected to the
  // external part, and connect the split edges to ports on the supernode.
  // Remove the original edges from 'edge_map' and add the new ones,
  // transferring ownership of the original edges to the supernode. 'node_map'
  // must contain the nodes in 'component_nodes'. 'port_map' must contain the
  // ports of the graph/parent node of 'component_nodes', or may contain NULL if
  // none of the nodes in 'component_nodes' connects externally to a port.
  static void SplitSupernodeBoundaryEdges(
      Node* supernode, const NodeIdSet& component_nodes,
      const EdgeIdSet& boundary_edges, KlfmNodeMap* node_map,
      KlfmEdgeMap* edge_map, PortMap* port_map);

  // Merge the edges that span the boundary of the specifed supernode. The
  // supernode-internal edge IDs will be kept and will replace the corresponding
  // edges external to the supernode. Removed edges are deleted and their IDs
  // free'd. If the supernode does not connect to any ports in its parent node,
  // 'port_map' may be passed as NULL.
  static void MergeSupernodeBoundaryEdges(
      Node* supernode, KlfmNodeMap* node_map, KlfmEdgeMap* edge_map,
      PortMap* port_map);

  void SummarizeResults(const std::vector<PartitionSummary>& summaries);
  void SummarizeResultMetric(
    std::vector<double>& data, const std::string& name, bool extended);
  void PrintResultFull(const PartitionSummary& summary, int run_num);
  void PrintHistogram(const std::vector<double>& val, bool cummulative);
  // Write solution in .sol format used by SCIP.
  void WriteScipSol(const NodePartitions& partition,
                    const std::string& filename);
  // Write solution in .sol format used by SCIP.
  void WriteScipSolAlt(const NodePartitions& partition,
                       const std::string& filename);
  // Write solution in .mst format used by Gurobi.
  void WriteGurobiMst(const NodePartitions& partition,
                      const std::string& filename);

  // Comparison fn for sort.
  static bool cmp_pair_second_gt(const std::pair<int,int>& lhs,
                          const std::pair<int,int>& rhs) {
    return lhs.second > rhs.second;
  }

  // Needs -lrt during linking.
  static uint64_t GetTimeUsec() {
    timespec tS;
    assert(clock_gettime(CLOCK_REALTIME, &tS) != -1);
    return (uint64_t)tS.tv_sec * 1000000LL + (uint64_t)tS.tv_nsec / 1000LL;
  }

  Options options_;

  // Output stream.
  std::ostream& os_;

  // Random number generator used for initial partitions. Used to prevent
  // class from becoming thread-hostile.
  std::default_random_engine random_engine_initial_;
  std::default_random_engine random_engine_rebalance_;
  std::default_random_engine random_engine_mutate_;
  std::default_random_engine random_engine_coarsen_;

  // The objects in this map are copied from the starting graph. As such, the
  // pointers are owned by this object.
  KlfmNodeMap internal_node_map_;
  KlfmEdgeMap internal_edge_map_;
  GainBucketManager* gain_bucket_manager_;
  std::vector<int> total_weight_;
  std::vector<int> max_weight_imbalance_;
  std::vector<double> max_imbalance_fraction_;
  std::vector<int> total_capacity_;
  size_t num_resources_per_node_;
  // This flag is used to indicate if a partition rebalance occurred, failed,
  // and no better partition was found later in the pass. When this situation
  // occurs, it is necessary to recompute the partition balance after
  // rolling back to the best partition, because the rebalance will have
  // invalidated the stored best partition balance.
  bool recompute_best_balance_flag_;

  // Variables used for tracking algorithm performance states.
  size_t node_count_;
  size_t max_at_node_count_;
  bool balance_exceeded_;
  unsigned int rebalances_this_run_;
  unsigned int rebalances_this_pass_;

  // Used for profiling run-time of methods in this class.
  //uint64_t start_time_;
  uint64_t gbe_start_time_;
  uint64_t gbe_time_;
  uint64_t set_vector_and_update_weights_start_time_;
  uint64_t set_vector_and_update_weights_time_;
  uint64_t move_node_and_update_balance_start_time_;
  uint64_t move_node_and_update_balance_time_;
  uint64_t gain_update_start_time_;
  uint64_t gain_update_time_;
  uint64_t rebalance_start_time_;
  uint64_t rebalance_time_;
  uint64_t weight_update_start_time_;
  uint64_t weight_update_time_;
  uint64_t cost_update_start_time_;
  uint64_t cost_update_time_;
  uint64_t total_move_start_time_;
  uint64_t total_move_time_;

  // temp
  uint64_t move_node_time_;
  uint64_t update_gains_time_;
  uint64_t num_connected_edges_;
  uint64_t num_critical_connected_edges_;
};

#endif /* PARTITION_ENGINE_KLFM_H_ */
