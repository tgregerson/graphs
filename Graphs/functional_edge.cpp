/*
 * functional_edge.cpp
 *
 *  Created on: Sep 8, 2014
 *      Author: gregerso
 */

#include "functional_edge.h"

#include <exception>
#include <sstream>

#include "functional_node.h"

using namespace std;

double FunctionalEdge::Entropy(map<string, FunctionalEdge*>* wires,
                               map<string, FunctionalNode*>* nodes) {
  return ComputeEntropy(wires, nodes);
}

double FunctionalEdge::ProbabilityOne(
    map<string, FunctionalEdge*>* wires,
    map<string, FunctionalNode*>* nodes) {
  if (p_one_ < 0.0) {
    p_one_ = ComputeProbabilityOne(wires, nodes);
  }
  return p_one_;
}

double FunctionalEdge::ComputeEntropy(map<string, FunctionalEdge*>* wires,
                                      map<string, FunctionalNode*>* nodes) {
  return ComputeShannonEntropy(wires, nodes);
}

double FunctionalEdge::ComputeShannonEntropy(map<string, FunctionalEdge*>* wires,
                                             map<string, FunctionalNode*>* nodes) {
  double p1 = ProbabilityOne(wires, nodes);
  if (p1 >= 1.0 || p1 <= 0.0) {
    return 0.0;
  } else {
    double p0 = 1.0 - p1;
    return -p1*log2(p1) + -p0*log2(p0);
  }
}

double FunctionalEdge::ComputeProbabilityOne(
    map<string, FunctionalEdge*>* wires,
    map<string, FunctionalNode*>* nodes) {
  double ret;
  if (probability_calculation_in_progress_) {
    // If we have a dependency loop, break it by returning full entropy.
    // cout << base_name_ << ": Got P1 from Loop." << endl;
    // TODO We could try to repeatedly run the calculation on the loop until it
    // converges to get a more accurate entropy.
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
      cout << base_name_ << ": Got P1 from Multiple Drivers." << endl;
      ret = 0.5;
    } else {
      const NodePortDescriptor& port = source_ports_.at(0);
      auto source_node_it = nodes->find(port.node_instance_name);
      if (source_node_it == nodes->end()) {
        throw exception();
      }
      ret = source_node_it->second->ComputeProbabilityOne(
          port.node_port_name, IndexedName(), wires, nodes);
    }
  }
  probability_calculation_in_progress_ = false;
  return ret;
}

string FunctionalEdge::IndexedName() const {
  stringstream ss;
  ss << base_name_ << "[" << bit_high_;
  if (bit_high_ != bit_low_) {
    ss << ":" << bit_low_;
  }
  ss << "]";
  return ss.str();
}

vector<string> FunctionalEdge::GetBitNames() const {
  vector<string> bit_names;
  for (int i = bit_low_; i <= bit_high_; ++i) {
    stringstream ss;
    ss << base_name_;
    ss << "[" << i << "]";
    bit_names.push_back(ss.str());
  }
  return bit_names;
}
