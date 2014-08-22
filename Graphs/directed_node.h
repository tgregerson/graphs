/*
 * directed_node.h
 *
 *  Created on: Aug 22, 2014
 *      Author: gregerso
 */

#ifndef DIRECTED_NODE_H_
#define DIRECTED_NODE_H_

#include "node.h"

#include <set>
#include <string>

#include "port.h"

class DirectedNode : public Node {
 public:
  DirectedNode(int node_id, const std::string& node_name = "")
      : Node(node_id, name) {}
  virtual ~DirectedNode() {}

  // Add a source edge pointing to this node.
  void AddInputConnection(int connected_id) {
    AddConnection(connected_id, Port::kInputType);
  }

  // Add a sink edge pointing from this node.
  void AddOutputConnection(int connected_id) {
    AddConnection(connected_id, Port::kOutputType);
  }

  // Returns the IDs of all connected edges on ports of 'type'.
  std::set<int> EdgeIdsByType(Port::PortType type) const;

  // Return the set of all input edge IDs.
  std::set<int> InputEdgeIds() const;

  // Return the set of all output edge IDs.
  std::set<int> OutputEdgeIds() const;

 private:

};



#endif /* DIRECTED_NODE_H_ */
