#ifndef LP_SOLVE_INTERFACE_H_
#define LP_SOLVE_INTERFACE_H_

#include <exception>
#include <memory>
#include <string>

#include "lp_solve/lp_lib.h"
#include "node.h"

class LpSolveInterface {
 public:
  LpSolveInterface() : state_(nullptr), graph_(0, 0, "top-level") {}
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
  // Returns an LP model using the structure in 'graph_'.
  lprec* ConstructModelFromGraph();

  void AddObjectiveFunction(lprec* model);
  void AddImbalanceConstraints(lprec* model);
  void AddNodeConstraint(lprec* model, const Node& node);

  std::unique_ptr<LpSolveState> state_;

  // TODO: Move inside ConstructModelFromGraph?
  Node graph_;
  std::vector<double> max_weight_imbalance_fraction_;
  // Row index 0 is not used in lp_solve.
  std::vector<int> row_index_to_id_;
  std::map<int, std::pair<int, int>> id_to_column_index_and_length_;
};


#endif // LP_SOLVE_INTERFACE_H_
