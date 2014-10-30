/*
 * functional_node.h
 *
 *  Created on: Sep 4, 2014
 *      Author: gregerso
 */

#ifndef FUNCTIONAL_NODE_H_
#define FUNCTIONAL_NODE_H_

#include <cmath>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "connection_descriptor.h"

class FunctionalEdge;

class FunctionalNode {
 public:

  typedef std::pair<std::string,std::string> NamedConnection;
  typedef std::map<std::string, FunctionalEdge*> EdgeMap;
  typedef std::map<std::string, FunctionalNode*> NodeMap;

  virtual ~FunctionalNode() {}

  // Abstract Methods

  virtual void AddConnection(const ConnectionDescriptor& connection) = 0;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const = 0;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const = 0;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const = 0;

  // Concrete Methods

  virtual void AddParameter(const NamedConnection& parameter) {
    named_parameters.insert(parameter);
  }

  virtual double ComputeProbabilityOne(
      const std::string& output_name, const std::string& wire_name,
      EdgeMap* wires, NodeMap* nodes) const {
    const std::vector<std::string>& bit_names =
        named_output_connections.at(output_name).connection_bit_names;
    for (size_t i = 0; i < bit_names.size(); ++i) {
      if (bit_names[i] == wire_name) {
        return ComputeProbabilityOne(output_name, i, wires, nodes);
      }
    }
    throw std::exception();
  }

  virtual double ComputeProbabilityZero(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const {
    return 1.0 - ComputeProbabilityOne(output_name, bit_pos, wires, nodes);
  }

  virtual double ComputeShannonEntropy(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const {
    double p_one = ComputeProbabilityOne(output_name, bit_pos, wires, nodes);
    if (p_one >= 1.0 || p_one <= 0.0) {
      return 0.0;
    } else {
      double p_zero = 1.0 - p_one;
      return -p_one*log2(p_one) + -p_zero*log2(p_zero);
    }
  }

  virtual double ComputeShannonEntropy(
        const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
        int bit_high, int bit_low) const {
    if (bit_high != bit_low) {
      // Just sum the entropies of individual bits.
      double total_entropy = 0.0;
      for (int bit = bit_low; bit <= bit_high; ++bit) {
        total_entropy +=
            ComputeShannonEntropy(output_name, bit,  wires, nodes);
      }
      return total_entropy;
    } else {
      return ComputeShannonEntropy(output_name, bit_high, wires, nodes);
    }
  }

  ConnectionDescriptor& GetConnectionDescriptor(const std::string& port_name) {
    if (named_input_connections.find(port_name) !=
        named_input_connections.end()) {
      return named_input_connections.at(port_name);
    } else if (named_output_connections.find(port_name) !=
               named_output_connections.end()) {
      return named_output_connections.at(port_name);
    } else {
      throw std::exception();
    }
  }

  std::string type_name;
  std::string instance_name;

  std::map<std::string, std::string> named_parameters;

  std::map<std::string, ConnectionDescriptor> named_input_connections;
  std::map<std::string, ConnectionDescriptor> named_output_connections;
};

#endif /* FUNCTIONAL_NODE_H_ */
