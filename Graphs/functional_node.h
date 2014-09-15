/*
 * functional_node.h
 *
 *  Created on: Sep 4, 2014
 *      Author: gregerso
 */

#ifndef FUNCTIONAL_NODE_H_
#define FUNCTIONAL_NODE_H_

#include <cmath>

#include <iostream>
#include <map>
#include <string>
#include <utility>

class FunctionalEdge;

class FunctionalNode {
 public:
  typedef std::pair<std::string,std::string> NamedConnection;
  typedef std::map<std::string, FunctionalEdge*> EdgeMap;
  typedef std::map<std::string, FunctionalNode*> NodeMap;

  virtual ~FunctionalNode() {}

  virtual void AddConnection(const NamedConnection& connection) = 0;
  // TODO These functions should be changed to pure virtual. Temporarily provide
  // default implementations to make it easier to incrementally develop child classes.
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes) const {
    std::cout << "WARNING: Call to pure virtual ComputeEntropy\n";
    return 1.0;
  }
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes,
      int bit_high, int bit_low) const {
    std::cout << "WARNING: Call to pure virtual ComputeEntropy\n";
    return 1.0;
  }
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      NodeMap* nodes) const {
    std::cout << "WARNING: Call to pure virtual ComputeProbabilityOne\n";
    return 0.5;
  }

  virtual void AddParameter(const NamedConnection& parameter) {
    named_parameters.insert(parameter);
  }
  virtual double ComputeProbabilityZero(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      NodeMap* nodes) const {
    return 1.0 - ComputeProbabilityOne(output_name, bit_pos, edges, nodes);
  }
  virtual double ComputeShannonEntropy(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      NodeMap* nodes) const {
    double p_one = ComputeProbabilityOne(output_name, bit_pos, edges, nodes);
    double p_zero = 1.0 - p_one;
    return -p_one*log2(p_one) + -p_zero*log2(p_zero);
  }

  std::string type_name;
  std::string instance_name;

  std::map<std::string, std::string> named_parameters;

  std::multimap<std::string, std::string> named_input_connections;
  std::multimap<std::string, std::string> named_output_connections;
  std::multimap<std::string, std::string> named_unknown_connections;
};

class UnknownFunctionalNode : public FunctionalNode {
 public:
  UnknownFunctionalNode() {}
  virtual ~UnknownFunctionalNode() {}

  virtual void AddConnection(const NamedConnection& connection) {
    named_unknown_connections.insert(connection);
  }
  virtual double ComputeEntropy(
      const std::string& output_name,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalNode*>* nodes) const { return 1.0; }
  // "bit_high" and "bit_low" can be used to specify a bit range if you only
  // want to get the entropy of a subset of the bits.
  virtual double ComputeEntropy(
      const std::string& output_name,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalNode*>* nodes,
      int bit_high,
      int bit_low) const { return 1.0; }
  virtual double ComputeProbabilityOne(
      const std::string& output_name,
      int bit_pos,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalNode*>* nodes) const { return 0.5; }
};

#endif /* FUNCTIONAL_NODE_H_ */
