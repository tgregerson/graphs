/*
 * functional_edge.h
 *
 *  Created on: Sep 8, 2014
 *      Author: gregerso
 */

#ifndef FUNCTIONAL_EDGE_H_
#define FUNCTIONAL_EDGE_H_

#include <map>
#include <string>
#include <vector>

class FunctionalNode;

class FunctionalEdge {
 public:
  struct NodePortDescriptor {
    NodePortDescriptor(const std::string& inst, const std::string& port,
                       int bh, int bl)
        : node_instance_name(inst), node_port_name(port), bit_high(bh),
          bit_low(bl) {}
    std::string node_instance_name;
    std::string node_port_name;
    int bit_high{0};
    int bit_low{0};
  };
  FunctionalEdge(const std::string& n, int w)
    : name(n), width(w), bit_high(w - 1), bit_low(0) {}
  FunctionalEdge(const std::string& n, int w, int bh, int bl)
    : name(n), width(w), bit_high(bh), bit_low(bl) {}

  double Entropy(std::map<std::string, FunctionalEdge*>* edges,
                 std::map<std::string, FunctionalNode*>* nodes);
  double ProbabilityOne(
      int bit_pos,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalNode*>* nodes);
  // A port is described by a pair of strings, where the first string is the
  // FunctionalNode instance name and the second string is the name of the
  // connection port on that node.
  void AddSourcePort(const NodePortDescriptor& port) {
    source_ports_.push_back(port);
  }
  void AddSinkPort(const NodePortDescriptor& port) {
    sink_ports_.push_back(port);
  }
  void SetEntropy(double e) {
    entropy_ = e;
  }
  void SetProbabilityOne(double p) {
    p_one_ = p;
  }
  void SetWeight(double w) {
    weight_ = w;
  }

  std::string name;
  int width{1};
  int bit_high{0};
  int bit_low{0};

 private:
  double ComputeEntropy(std::map<std::string, FunctionalEdge*>* edges,
                        std::map<std::string, FunctionalNode*>* nodes);
  double ComputeProbabilityOne(
      int bit_pos,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalNode*>* nodes);

  std::vector<NodePortDescriptor> source_ports_;
  std::vector<NodePortDescriptor> sink_ports_;

  double entropy_{-1.0};
  double p_one_{-1.0};
  double weight_{1.0};
  // Used to detect dependency loops.
  bool entropy_calculation_in_progress_{false};
  bool probability_calculation_in_progress_{false};
};


#endif /* FUNCTIONAL_EDGE_H_ */
