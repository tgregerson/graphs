#ifndef LP_SOLVE_INTERFACE_H_
#define LP_SOLVE_INTERFACE_H_

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lp_solve/lp_lib.h"
#include "universal_macros.h"

class Edge;
class Node;

class LpSolveInterface {
 public:
  LpSolveInterface() : state_(nullptr), max_imbalance_(0.01), verbose_(false) {}
  LpSolveInterface(double max_imbalance, bool verbose)
      : state_(nullptr), max_imbalance_(max_imbalance), verbose_(verbose) {}
  ~LpSolveInterface() {}

  // If model has been previously constructed and written to a file, it can
  // be loaded without additional construction.
  void LoadFromMps(const std::string& filename);

  // Load and construct ILP model from CHACO graph or netlist format.
  void LoadFromChaco(const std::string& filename);
  void LoadFromNtl(const std::string& filename);

  // After loading a model, it can be written to a native ILP format for much
  // faster parsing in the future.
  void WriteToLp(const std::string& filename) const;
  void WriteToMps(const std::string& filename) const;

  // Run the solver for 'timeout_s' seconds.
  void RunSolver(long timeout_s);

  enum MpsNameType {
    kDefaultNames,
    kCompressedNames,
  };

  // Encapsulates state related to lp_solve.
  class LpSolveState {
   public:
    LpSolveState();
    LpSolveState(lprec* model) : model_(CHECK_NOTNULL(model), delete_lp) {}

    lprec* model() const {
      return model_.get();
    }

   private:
    std::unique_ptr<lprec, void (*)(lprec*)> model_;
  };

  // Exceptions that wrap errors from lp_solve.
  class LpSolveException : public std::exception {
   public:
    LpSolveException(const std::string& msg) : msg_(msg) {}
    const char* what() const noexcept { return (msg_ + "\n").c_str(); }

   private:
    const std::string msg_;
  };

 private:
  class GraphParsingState {
   public:
    GraphParsingState(Node* graph, double max_imbalance, bool verbose);

    // Returns an LP model using the structure in 'graph_'. Caller takes
    // ownership of model. Can choose between using row or column building
    // mode. Typically column mode is much faster, but requires more memory
    // to build model.
    lprec* ConstructModel(bool column_mode = true);

   private:
    // Uses row-centric model building.
    lprec* ConstructModelRowMode();
    // Uses column-centric model building.
    lprec* ConstructModelColumnMode();

    // Pre-allocation is not necessary, but can improve performance.
    // This function should be called after the graph has been populated, but
    // prior to adding nodes/edges to the model
    void PreAllocateModelMemory(lprec* model);
    void SetObjectiveFunction(lprec* model);

    // Convenience methods for accessing the variable index data structure.
    inline int GetNodeVariableIndex(
        int node_id, int partition_num, int personality_num) {
      return node_to_variable_indices_[node_id][partition_num][personality_num];
    }
    void SetNodeVariableIndex(
        int node_id, int partition_num, int personality_num, int index);

    inline int GetEdgeCrossingVariableIndex(int edge_id) {
      return edge_to_crossing_variable_indices_[edge_id];
    }
    void SetEdgeCrossingVariableIndex(int edge_id, int index);

    inline int GetEdgePartitionConnectivityVariableIndex(
        int edge_id, int partition_num) {
      return edge_to_partition_connectivity_variable_indices_[edge_id][partition_num];
    }
    void SetEdgePartitionConnectivityVariableIndex(
        int edge_id, int partition_num, int index);

    void AssignAllVariableIndices();
    void AssignAllNodeVariableIndices();
    void AssignAllEdgeVariableIndices();
    void AssignNodeVariableIndices(const Node& node);
    void AssignEdgeVariableIndices(const Edge& edge);

    void AddAllConstraintsToModel(lprec* model, bool defer_to_columns);
    void AddAllNodeConstraintsToModel(lprec* model, bool defer_to_columns);
    void AddAllEdgeConstraintsToModel(lprec* model, bool defer_to_columns);
    void AddImbalanceConstraintsToModel(lprec* model, bool defer_to_columns);
    void AddNodeConstraintsToModel(lprec* model, const Node& node,
                                   bool defer_to_columns);
    void AddEdgeConstraintsToModel(lprec* model, const Edge& edge,
                                   bool defer_to_columns);
    void AddEdgeConstraintsNewToModel(lprec* model, const Edge& edge,
                                      bool defer_to_columns);

    void SetAllVariablesBinary(lprec* model);
    void SetAllVariableNames(lprec* model);
    
    int NumTotalNodeVariablesNeeded();
    int NumTotalEdgeVariablesNeeded();
    int NumTotalNodeConstraintsNeeded();
    int NumTotalEdgeConstraintsNeeded();

    // For Deferred column construction.
    void ConstraintExToDeferredColumns(
        int count, const REAL* const constraint_coeffs,
        const int* const constraint_indices);
    void ConstraintExToDeferredColumns(
        const std::vector<REAL>& constraint_coeffs,
        const std::vector<int>& constraint_indices);
    void AddDeferredColumnsEx(lprec* model);
    char AddColumnEx(lprec* model, int count, const REAL* const coeffs,
                     const int* const indices);
    char AddColumnEx(lprec* model, const std::vector<REAL>& coeffs,
                     const std::vector<int>& indices);
    char AddConstraintEx(lprec* model, int count, const REAL* const coeffs,
                         const int* const indices, int ctype, REAL rhs);
    char AddConstraintEx(lprec* model, const std::vector<REAL>& coeffs,
                         const std::vector<int>& indices, int ctype, REAL rhs);

    const Node* const graph_;
    int num_resources_;
    const double max_weight_imbalance_fraction_;
    const bool verbose_;

    // Indexed [NodeId][NodePartitionId][NodePersonalityId]
    // Partition and Personality IDs start at zero and are sequential.
    // Node IDs are non-negative.
    std::map<int, std::vector<std::vector<int>>> node_to_variable_indices_;
    // Indexed by [EdgeId]
    // Edge IDs are non-negative.
    std::map<int, int> edge_to_crossing_variable_indices_;
    // Indexed by [EdgeId][EdgePartitionId]
    // Edge Personality IDs start at zero and are sequential.
    std::map<int, std::vector<int>>
        edge_to_partition_connectivity_variable_indices_;
    int next_variable_index_;

    // Used in deferred column construction method.
    int columns_ex_next_row_;
    std::map<int, std::vector<std::pair<int, REAL>>> columns_ex_;

    // For now, only support bipartitioning.
    const int num_partitions_ = 2;
    const int kIndexGuard = -1;
    const int kVerboseAddQuanta = 10000;
  };

  std::unique_ptr<LpSolveState> state_;
  const double max_imbalance_;
  const bool verbose_;
};

#endif // LP_SOLVE_INTERFACE_H_
