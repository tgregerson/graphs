/*
 * directed_node.cpp
 *
 *  Created on: Aug 22, 2014
 *      Author: gregerso
 */

#include "directed_node.h"

using namespace std;

set<int> DirectedNode::InputEdgeIds() const {
  return EdgeIdsByType(Port::kInputType);
}

set<int> DirectedNode::OutputEdgeIds() const {
  return EdgeIdsByType(Port::kOutputType);
}

set<int> DirectedNode::EdgeIdsByType(Port::PortType type) const {
  std::set<int> input_edges;
  for (auto port : ports_) {
    if (port.second.type == type) {
      input_edges.insert(port.second.external_edge_id);
    }
  }
  return input_edges;
}
