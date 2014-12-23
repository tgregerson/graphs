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
                       unsigned int bh, unsigned int bl)
        : node_instance_name(inst), node_port_name(port), bit_high(bh),
          bit_low(bl) {}
    std::string node_instance_name;
    std::string node_port_name;
    int bit_high{0};
    int bit_low{0};
  };

  FunctionalEdge(const std::string& n, int w)
    : base_name_(n), bit_high_(w - 1), bit_low_(0) {}
  FunctionalEdge(const std::string& n, int bh, int bl)
    : base_name_(n), bit_high_(bh), bit_low_(bl) {}
  virtual ~FunctionalEdge() {}

  // Returns the entropy value assigned to the edge.
  virtual double Entropy(std::map<std::string, FunctionalEdge*>* wires,
                         std::map<std::string, FunctionalNode*>* nodes);

  // Returns the probability that the component of the edge at 'bit_pos'
  // is equal to one.
  virtual double ProbabilityOne(
      std::map<std::string, FunctionalEdge*>* wires,
      std::map<std::string, FunctionalNode*>* nodes);

  virtual void SetProbabilityOne(double p1) {
    p_one_ = p1;
  }

  virtual int BitHigh() const { return bit_high_; }
  virtual int BitLow() const { return bit_low_; }
  virtual int Width() const { return bit_high_ - bit_low_ + 1; }

  // A port is described by a pair of strings, where the first string is the
  // FunctionalNode instance name and the second string is the name of the
  // connection port on that node.
  void AddSourcePort(const NodePortDescriptor& port) {
    source_ports_.push_back(port);
  }
  void AddSinkPort(const NodePortDescriptor& port) {
    sink_ports_.push_back(port);
  }

  // Gets the Base Name. This does not include a bit range.
  virtual std::string BaseName() const {
    return base_name_;
  }

  // Gets the base name + a range (for multi-bit) or index (for single-bit);
  virtual std::string IndexedName() const;

  // Gets a container with all single-bit indexed names.
  virtual std::vector<std::string> GetBitNames() const;

  // Check if the edge contains the index.
  virtual bool IndexValid(int index) {
    return index <= bit_high_ && index >= bit_low_;
  }

  void SetWeight(double w) {
    weight_ = w;
  }

  const std::vector<NodePortDescriptor>& SourcePorts() const {
    return source_ports_;
  }

  const std::vector<NodePortDescriptor>& SinkPorts() const {
    return sink_ports_;
  }

 private:
  virtual double ComputeEntropy(
      std::map<std::string, FunctionalEdge*>* wires,
      std::map<std::string, FunctionalNode*>* nodes);
  virtual double ComputeProbabilityOne(
      std::map<std::string, FunctionalEdge*>* wires,
      std::map<std::string, FunctionalNode*>* nodes);
  virtual double ComputeShannonEntropy(
      std::map<std::string, FunctionalEdge*>* wires,
      std::map<std::string, FunctionalNode*>* nodes);


  std::vector<NodePortDescriptor> source_ports_;
  std::vector<NodePortDescriptor> sink_ports_;
  std::string base_name_;
  int bit_high_{0};
  int bit_low_{0};
  double p_one_{-1.0};
  double weight_{1.0};
  // Used to detect dependency loops.
  bool probability_calculation_in_progress_{false};
};


#endif /* FUNCTIONAL_EDGE_H_ */
