#ifndef LP_SOLVE_INTERFACE_H_
#define LP_SOLVE_INTERFACE_H_

#include <exception>
#include <map>
#include <memory>
#include <vector>
#include <string>

#include "lp_solve/lp_lib.h"
#include "universal_macros.h"

class Edge;
class Node;

class LpSolveInterface {
 public:
  LpSolveInterface() : state_(nullptr) {}
  ~LpSolveInterface() {}

  // Load and construct ILP model from CHACO graph or netlist format.
  void LoadFromChaco(const std::string& filename);
  void LoadFromNtl(const std::string& filename);

  // If model has been previously constructed and written to a file, it can
  // be loaded without additional construction.
  void WriteToMps(const std::string& filename) const;
  void LoadFromMps(const std::string& filename);
  
  // Encapsulates state related to lp_solve.
  class LpSolveState {
   public:
    LpSolveState();
    LpSolveState(lprec* model) : model_(model, delete_lp) {}

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
    GraphParsingState(Node* graph) : graph_(CHECK_NOTNULL(graph)) {}

    // Returns an LP model using the structure in 'graph_'. Caller takes
    // ownership of model.
    lprec* ConstructModel();

   private:
    void SetObjectiveFunction(lprec* model);
    // Elements must be added to the model in the following order:
    // 1. Imbalance constraints
    // 2. Nodes
    // 3. Edges
    void AddImbalanceConstraintsToModel(lprec* model);
    void AddNodeToModel(lprec* model, const Node& node);
    void AddEdgeToModel(lprec* model, const Edge& edge);

    Node* graph_;
    std::vector<double> max_weight_imbalance_fraction_;
    std::map<int, int> variable_index_to_id_;
    // Provides 3-D access to variable (column) indexes in the model.
    //
    // Access a node index by map[id][partition_num][personality_num]
    // Access an edge crossing index by map[id][num_partitions][0]
    // Access an edge partition connectivity by map[id][partition_num][0]
    //
    // Where 'id' is node.id or edge.id.
    std::map<int, std::vector<std::vector<int>>> id_to_variable_indices_;
  };

  std::unique_ptr<LpSolveState> state_;
};

#endif // LP_SOLVE_INTERFACE_H_
