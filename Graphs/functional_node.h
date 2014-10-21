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

class FunctionalEdge;

class FunctionalNode {
 public:
  struct ConnectionDescriptor {
    ConnectionDescriptor(const std::string& pn) : port_name(pn) {}
    // Convenience interface for single-bit connections.
    ConnectionDescriptor(const std::string& pn, const std::string& cbn)
      : port_name(pn) {
      AddBitConnection(0, cbn);
    }
    ConnectionDescriptor(const std::string& pn, int pw,
                         const std::vector<std::string>& cbn)
      : port_name(pn), port_width(pw), connection_bit_names(cbn) {}

    void AddBitConnection(unsigned int index, const std::string& bit_name) {
      if (connection_bit_names.size() <= index) {
        connection_bit_names.resize(index + 1);
        port_width = index + 1;
      }
      connection_bit_names[index] = bit_name;
    }

    void AddBitConnection(const std::string& bit_name) {
      connection_bit_names.push_back(bit_name);
      ++port_width;
    }

    // Useful because Verilog presents bit names in descending order,
    // so it may be convenient to call after doing AddBitConnection on
    // descending bits.
    void ReverseConnectedBitNames() {
      std::reverse(connection_bit_names.begin(), connection_bit_names.end());
    }

    std::string port_name;
    int port_width{0};
    std::vector<std::string> connection_bit_names;
  };

  ConnectionDescriptor& GetConnectionDescriptor(const std::string& port_name) {
    if (named_input_connections.find(port_name) !=
        named_input_connections.end()) {
      return named_input_connections.at(port_name);
    } else if (named_output_connections.find(port_name) !=
               named_output_connections.end()) {
      return named_output_connections.at(port_name);
    } else {
      return named_unknown_connections.at(port_name);
    }
  }

  typedef std::pair<std::string,std::string> NamedConnection;
  typedef std::map<std::string, FunctionalEdge*> EdgeMap;
  typedef std::map<std::string, FunctionalNode*> NodeMap;

  virtual ~FunctionalNode() {}

  virtual void AddConnection(const ConnectionDescriptor& connection) = 0;
  // TODO These functions should be changed to pure virtual. Temporarily provide
  // default implementations to make it easier to incrementally develop child classes.
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes) const {
    std::cout << "WARNING: Call to pure virtual ComputeEntropy\n";
    return 1.0;
  }
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes, int bit_high, int bit_low) const {
    std::cout << "WARNING: Call to pure virtual ComputeEntropy\n";
    return 1.0;
  }
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      EdgeMap* wires, NodeMap* nodes) const {
    std::cout << "WARNING: Call to pure virtual ComputeProbabilityOne\n";
    return 0.5;
  }

  virtual void AddParameter(const NamedConnection& parameter) {
    named_parameters.insert(parameter);
  }
  virtual double ComputeProbabilityZero(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      EdgeMap* wires, NodeMap* nodes) const {
    return 1.0 - ComputeProbabilityOne(output_name, bit_pos, edges, wires, nodes);
  }
  virtual double ComputeShannonEntropy(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      EdgeMap* wires, NodeMap* nodes) const {
    double p_one = ComputeProbabilityOne(output_name, bit_pos, edges, wires,
                                         nodes);
    double p_zero = 1.0 - p_one;
    return -p_one*log2(p_one) + -p_zero*log2(p_zero);
  }

  std::string type_name;
  std::string instance_name;

  std::map<std::string, std::string> named_parameters;

  std::map<std::string, ConnectionDescriptor> named_input_connections;
  std::map<std::string, ConnectionDescriptor> named_output_connections;
  std::map<std::string, ConnectionDescriptor> named_unknown_connections;
};

class UnknownFunctionalNode : public FunctionalNode {
 public:
  UnknownFunctionalNode() {}
  virtual ~UnknownFunctionalNode() {}

  virtual void AddConnection(const ConnectionDescriptor& connection) {
    named_unknown_connections.insert(make_pair(connection.port_name, connection));
  }
  virtual double ComputeEntropy(
      const std::string& output_name,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalEdge*>* wires,
      std::map<std::string, FunctionalNode*>* nodes) const { return 1.0; }
  // "bit_high" and "bit_low" can be used to specify a bit range if you only
  // want to get the entropy of a subset of the bits.
  virtual double ComputeEntropy(
      const std::string& output_name,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalEdge*>* wires,
      std::map<std::string, FunctionalNode*>* nodes,
      int bit_high,
      int bit_low) const { return 1.0; }
  virtual double ComputeProbabilityOne(
      const std::string& output_name,
      int bit_pos,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalEdge*>* wires,
      std::map<std::string, FunctionalNode*>* nodes) const { return 0.5; }
};

#endif /* FUNCTIONAL_NODE_H_ */
