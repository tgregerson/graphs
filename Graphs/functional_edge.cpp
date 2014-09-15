/*
 * functional_edge.cpp
 *
 *  Created on: Sep 8, 2014
 *      Author: gregerso
 */

#include "functional_edge.h"

#include <exception>

#include "functional_node.h"

using namespace std;

double FunctionalEdge::Entropy(map<string, FunctionalEdge*>* edges,
                               map<string, FunctionalNode*>* nodes) {
  if (entropy_ < 0) {
    entropy_ = ComputeEntropy(edges, nodes);
  }
  return entropy_;
}

double FunctionalEdge::ProbabilityOne(
    int bit_pos,
    map<string, FunctionalEdge*>* edges,
    map<string, FunctionalNode*>* nodes) {
  if (p_one_ < 0.0) {
    p_one_ = ComputeProbabilityOne(bit_pos, edges, nodes);
  }
  return p_one_;
}

double FunctionalEdge::ComputeEntropy(map<string, FunctionalEdge*>* edges,
                                      map<string, FunctionalNode*>* nodes) {
  if (name == "n_0_zrl_proc_i_1") {
    cout << "N0 ComputeEntropy" << endl;
  }
  double ret;
  if (entropy_calculation_in_progress_) {
    // If we have a dependency loop, break it by returning full entropy.
    ret = weight_;
    cout << "Got Entropy from Loop." << endl;
  } else {
    entropy_calculation_in_progress_ = true;
    if (source_ports_.size() < 1) {
      // TODO
      // These are primary inputs (assuming non-error).
      // They need to have entropies set manually.
      ret = weight_;
      cout << "Got Entropy from Primary Input." << endl;
    } else if (source_ports_.size() > 1) {
      // TODO: Better way of handling nets with multiple drivers?
      cout << "Got Entropy from Multiple Drivers." << endl;
      ret = weight_;
    } else {
      const NodePortDescriptor& port = source_ports_[0];
      auto source_node_it = nodes->find(port.node_instance_name);
      if (source_node_it == nodes->end()) {
        throw exception();
      }
      cout << "Got Entropy from Driving Node." << endl;
      ret = source_node_it->second->ComputeEntropy(
          port.node_port_name, edges, nodes, port.bit_high, port.bit_low);
    }
  }
  entropy_calculation_in_progress_ = false;
  return ret;
}

double FunctionalEdge::ComputeProbabilityOne(
    int bit_pos,
    map<string, FunctionalEdge*>* edges,
    map<string, FunctionalNode*>* nodes) {
  double ret;
  if (probability_calculation_in_progress_) {
    // If we have a dependency loop, break it by returning full entropy.
    ret = 0.5;
  } else {
    probability_calculation_in_progress_ = true;
    if (source_ports_.size() < 1) {
      // TODO
      // These are primary inputs (assuming non-error).
      // They need to have entropies set manually.
      ret = 0.5;
    } else if (source_ports_.size() > 1) {
      // TODO: Better way of handling nets with multiple drivers?
      ret = 0.5;
    } else {
      const NodePortDescriptor& port = source_ports_[0];
      auto source_node_it = nodes->find(port.node_instance_name);
      if (source_node_it == nodes->end()) {
        throw exception();
      }
      ret = source_node_it->second->ComputeProbabilityOne(
          port.node_port_name, bit_pos, edges, nodes);
    }
  }
  probability_calculation_in_progress_ = false;
  return ret;
}
