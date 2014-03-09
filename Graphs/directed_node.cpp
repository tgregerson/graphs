#include "directed_node.h"

#include <cstdio>
#include <sstream>
#include <utility>

#include "directed_edge.h"
#include "id_manager.h"

using std::make_pair;
using std::ostringstream;
using std::string;
using std::unordered_set;
using std::vector;

DirectedNode::DirectedNode(DirectedNode* src) {
  CopyFrom(src);
}

DirectedNode::DirectedNode(int node_id, string name)
  : Node(node_id, name) {
  if (name.empty()) {
    // Create generic name.
    name = "Node";
    ostringstream oss;
    oss << node_id;
    name.append(oss.str());
  }
}

DirectedNode::DirectedNode(int node_id, int weight, string name)
  : Node(node_id, weight, name) {
  if (name.empty()) {
    // Create generic name.
    name = "Node";
    ostringstream oss;
    oss << node_id;
    name.append(oss.str());
  }
}

void DirectedNode::AddSource(int connected_id) {
	assert(source_ports_.find(connected_id) == source_ports_.end());
  int port_id = IdManager::AcquireNodeId();
  source_ports_.insert(
		  make_pair(port_id, Port(port_id, IdManager::kReservedTerminalId,
         											connected_id, Port::kInputType)));
	AddConnection(connected_id);
}

void DirectedNode::AddSink(int connected_id) {
	assert(sink_ports_.find(connected_id) == source_ports_.end());
  int port_id = IdManager::AcquireNodeId();
  sink_ports_.insert(
		  make_pair(port_id, Port(port_id, IdManager::kReservedTerminalId,
         											connected_id, Port::kOutputType)));
	AddConnection(connected_id);
}

int DirectedNode::ConsolidateSubNodes(
    const unordered_set<int>& component_nodes) {
  assert(is_supernode());

  int supernode_id = IdManager::AcquireNodeId();
  DirectedNode* supernode = new DirectedNode(supernode_id);
  ostringstream oss;
  oss << "Supernode";
  oss << supernode->id;
  supernode->name.append(oss.str());

  // TODO: Check that component nodes are all connected?

  std::unordered_set<int> touching_edges;
  for (auto node_id : component_nodes) {
    assert(internal_nodes_.count(node_id) > 0);
    // Make a set of all edges that touch the component nodes.
    Node* node = internal_nodes_[node_id];
    for (auto& port_pair : node->ports()) {
      touching_edges.insert(port_pair.second.external_edge_id);
    }
  }
  assert(!touching_edges.empty());


  // Check if edges touching each component node are wholly internal to the
  // supernode (i.e. do they only connect other component nodes)
  unordered_set<int> supernode_internal_edges;
  unordered_set<int> supernode_external_source_edges;
  unordered_set<int> supernode_external_sink_edges;
  bool is_wholly_internal;
  for (auto edge_id : touching_edges) {
    is_wholly_internal = true;
    assert(internal_edges_.count(edge_id) > 0);
    //  TODO: Switch to static_cast once this code is thoroughly tested.
    DirectedEdge* edge = dynamic_cast<DirectedEdge*>(internal_edges_[edge_id]);
    for (auto source_id : edge->source_ids) {
      if (component_nodes.count(source_id) == 0) {
        supernode_external_source_edges.insert(edge_id);
        is_wholly_internal = false;
        break;
      }
    }
    // If an edge already has a source node that is not one of the
    // components of the new supernode, it is only necessary to make it
    // an input to the supernode, even if it also has a sink outside the
    // supernode. The edge should only be an output of the supernode if
    // it only has non-supernode sinks.
    if (supernode_external_source_edges.count(edge_id) == 0) {
      for (auto sink_id : edge->sink_ids) {
        if (component_nodes.count(sink_id) == 0) {
          supernode_external_sink_edges.insert(edge_id);
          is_wholly_internal = false;
          break;
        }
      }
    }
    if (is_wholly_internal) {
      supernode_internal_edges.insert(edge_id);
    }
  }

  // Debug.
  assert(!supernode_external_source_edges.empty());
  assert(!supernode_external_sink_edges.empty());

  // Cut the edges that span the boundaries of the supernode, making a new
  // edge with a new id to take its place in this node and moving the old
  // edge into the supernode.
  SplitSupernodeBoundaryEdgesByType(supernode, component_nodes,
      supernode_external_source_edges, true);
  SplitSupernodeBoundaryEdgesByType(supernode, component_nodes,
      supernode_external_sink_edges, false);

  // Move wholly internal edges from this node to the supernode.
  for (auto sn_int_edge_id : supernode_internal_edges) {
    Edge* sn_int_edge = internal_edges_[sn_int_edge_id];
    supernode->internal_edges_.insert(make_pair(sn_int_edge_id, sn_int_edge));
    internal_edges_.erase(sn_int_edge_id);
  }

  // Move all component nodes to the supernode.
  for (auto c_node_id : component_nodes) {
    supernode->internal_nodes_.insert(
        make_pair(c_node_id, internal_nodes_[c_node_id]));
    internal_nodes_.erase(c_node_id);
  }

  // Supernode is finished. Insert it as one of the internal nodes.
  internal_nodes_.insert(make_pair(supernode_id, supernode));

  // Debug.
  assert(supernode->is_supernode());
  assert(!supernode->ports_.empty());
  return supernode_id;
}

void DirectedNode::SplitSupernodeBoundaryEdgesByType(
    Node* supernode, const unordered_set<int>& component_nodes,
    const unordered_set<int>& boundary_edges, bool source_edges) {
  int supernode_id = supernode->id;

  for (auto edge_id : boundary_edges) {
    // Move edge to supernode.
    assert(internal_edges_.count(edge_id) != 0);
    DirectedEdge* edge = dynamic_cast<DirectedEdge*>(internal_edges_[edge_id]);
    supernode->AddInternalEdge(edge_id, edge);
    internal_edges_.erase(edge_id);

    // Make new edge.
    int new_edge_id = IdManager::AcquireEdgeId();
    DirectedEdge* new_boundary_edge = new DirectedEdge(
        new_edge_id, edge->weight, edge->GenerateSplitEdgeName(new_edge_id));
    internal_edges_.insert(make_pair(new_edge_id, new_boundary_edge));
    if (source_edges) {
      new_boundary_edge->AddSink(supernode_id); 
    } else {
      new_boundary_edge->AddSource(supernode_id);
    }

    // Create port inside supernode and update connection IDs.

    // This vector contains the IDs of Nodes/Ports that will need to be updated
    // to reflect the change in the ID of the edge that connects to them.
    vector<int> sources_to_update;
    vector<int> sinks_to_update;

    // Transfer all sources not in the supernode from the old edge to the new.
    vector<int> sources_to_remove_from_edge;
    vector<int> sinks_to_remove_from_edge;
    for (auto source : edge->source_ids) {
      // If the source node/port is not one of the components that make up
      // the new supernode, the edge connection to it must be split.
      if (component_nodes.count(source) == 0) {
        new_boundary_edge->AddSource(source);
        sources_to_update.push_back(source);
        sources_to_remove_from_edge.push_back(source);
      }
    }
    for (auto sink : edge->sink_ids) {
      if (component_nodes.count(sink) == 0) {
        new_boundary_edge->AddSink(sink);
        sinks_to_update.push_back(sink);
        sinks_to_remove_from_edge.push_back(sink);
      }
    }
    // This must not be done in the previous loop to avoid invalidating
    // iterators.
    for (auto removed_source : sources_to_remove_from_edge) {
      edge->RemoveSource(removed_source);
    }
    for (auto removed_sink : sinks_to_remove_from_edge) {
      edge->RemoveSink(removed_sink);
    }

    // Add supernode port as source for the part of the edge that is inside
    // the supernode.
    int new_port_id = IdManager::AcquireNodeId();
    ostringstream port_oss;
    port_oss << "Supernode_" << supernode_id << "_Port_" << new_port_id;
    if (source_edges) {
      edge->AddSource(new_port_id); 
      supernode->ports().insert(make_pair(new_port_id,
          Port(new_port_id, edge_id, new_edge_id, Port::kInputType,
               port_oss.str())));
    } else {
      edge->AddSink(new_port_id); 
      supernode->ports().insert(make_pair(new_port_id,
          Port(new_port_id, edge_id, new_edge_id, Port::kOutputType,
               port_oss.str())));
    }

    // Replace references to the old edge id in all nodes/ports not in the
    // supernode with reference to new edge.
    for (auto outdated_source : sources_to_update) {
      if (internal_nodes_.count(outdated_source) != 0) {
        DirectedNode* outdated_node =
            dynamic_cast<DirectedNode*>(internal_nodes_[outdated_source]);
        outdated_node->SwapSink(edge_id, new_edge_id);
      } else if (ports_.count(outdated_source) != 0) {
        Port& p = ports_[outdated_source];
        p.internal_edge_id = new_edge_id;
      } else {
        printf("Internal error: Could not find source that needs to be "
               "updated.");
        exit(1);
      }
    }
    for (auto outdated_sink : sinks_to_update) {
      if (internal_nodes_.count(outdated_sink) != 0) {
        DirectedNode* outdated_node =
            dynamic_cast<DirectedNode*>(internal_nodes_[outdated_sink]);
        outdated_node->SwapSource(edge_id, new_edge_id);
      } else if (ports_.count(outdated_sink) != 0) {
        Port& p = ports_[outdated_sink];
        p.internal_edge_id = new_edge_id;
      } else {
        printf("Internal error: Could not find sink that needs to be "
               "updated.");
        exit(1);
      }
    }
  }
}

void DirectedNode::MergeSupernodeBoundaryEdges(Node* supernode) {
  // Iterate through all the ports on 'supernode', replacing all references
  // to the port's external connection edge with the internal edge.

  // Cycle through all the ports, merging edges on each port.
  for (auto& supernode_port_pair : supernode->ports()) {
    Port& supernode_port = supernode_port_pair.second;
    DirectedEdge* external_edge = dynamic_cast<DirectedEdge*>(
        internal_edges_[supernode_port.external_edge_id]);
    DirectedEdge* internal_edge = dynamic_cast<DirectedEdge*>(
        supernode->GetInternalEdge(supernode_port.internal_edge_id));
    assert(internal_edge != NULL);

    // Remove reference to the supernode's port.
    if (supernode_port.type == Port::kInputType) {
      internal_edge->RemoveSource(supernode_port.id);
    } else {
      internal_edge->RemoveSink(supernode_port.id);
    }

    // Transfer references to all of the external edge's connections to the
    // internal edge and update the connecting nodes and ports with the
    // internal edge's ID.
    for (auto source : external_edge->source_ids) {
      if (source != supernode->id) {
        internal_edge->AddSource(source);
        if (internal_nodes_.count(source) != 0) {
          DirectedNode* node =
            dynamic_cast<DirectedNode*>(internal_nodes_[source]);
          node->SwapSink(external_edge->id, internal_edge->id);
        } else if (ports_.count(source) != 0) {
          Port p = ports_[source];
          p.internal_edge_id = internal_edge->id;
        } else {
          printf("Fatal error: Edge source not found in internal nodes");
          assert(false);
        }
      }
    }
    for (auto sink : external_edge->sink_ids) {
      if (sink != supernode->id) {
        internal_edge->AddSink(sink);
        if (internal_nodes_.count(sink) != 0) {
          DirectedNode* node =
            dynamic_cast<DirectedNode*>(internal_nodes_[sink]);
          node->SwapSource(external_edge->id, internal_edge->id);
        } else if (ports_.count(sink) != 0) {
          Port p = ports_[sink];
          p.internal_edge_id = internal_edge->id;
        } else {
          printf("Fatal error: Edge sink not found in internal nodes");
          assert(false);
        }
      }
    }

    // Remove external edge.
    internal_edges_.erase(external_edge->id);
    delete external_edge;
  }
}

void DirectedNode::SwapSource(int old_id, int new_id) {
  SwapPortConnectionByType(old_id, new_id, Port::kInputType);
}

void DirectedNode::SwapSink(int old_id, int new_id) {
  SwapPortConnectionByType(old_id, new_id, Port::kOutputType);
}

void DirectedNode::Print(bool recursive) const {
  // DEBUG
  printf("DirectedNode::Print\n");
  // DEBUG
  printf("D nodes: %ld\n", internal_nodes_.size());
  printf("D edges: %ld\n", internal_edges_.size());
  if (is_supernode()) {
    printf("SuperNode  ID=%d  Name=%s\n", id, name.c_str()); 
  } else {
    printf("Node  ID=%d  Name=%s\n", id, name.c_str()); 
  }
  printf("Number of weight vectors: %lu\n", weight_vectors_.size());
  printf("Weights:\n");
  for (auto& wv : weight_vectors_) {
    for (auto w : wv) {
      printf("%d ", w);
    }
    printf("\n");
  }
  printf("Source Edge IDs: ");
  for (auto& it : ports_) {
    if (it.second.type == Port::kInputType) {
      printf("%d ", it.second.external_edge_id);
    }
  }
  printf("\n");
  printf("Sink Edge IDs: ");
  for (auto& it : ports_) {
    if (it.second.type == Port::kOutputType) {
      printf("%d ", it.second.external_edge_id);
    }
  }
  printf("\n");
  if (is_supernode()) {
    printf("Internal Nodes: ");
    for (auto it : internal_nodes_) {
      printf("%d ", it.first);
    }
    printf("\n");
    printf("Internal Edges: ");
    for (auto it : internal_edges_) {
      printf("%d ", it.first);
    }
    printf("\n");
    if (recursive) {
      printf("\n");
      PrintPorts();
      printf("\n");
      PrintInternalNodes(true);
      printf("\n");
      PrintInternalEdges();
      printf("\n");
    }
  }
}

void DirectedNode::TransferEdgeSourcesExcluding(
    DirectedEdge* from_edge, DirectedEdge* to_edge, int exclude_id) {
  TransferEdgeConnectionsExcludingByType(from_edge, to_edge, exclude_id, true);
}

void DirectedNode::TransferEdgeSinksExcluding(
    DirectedEdge* from_edge, DirectedEdge* to_edge, int exclude_id) {
  TransferEdgeConnectionsExcludingByType(from_edge, to_edge, exclude_id, false);
}

void DirectedNode::TransferEdgeConnectionsExcludingByType(
    DirectedEdge* from_edge, DirectedEdge* to_edge, int exclude_id,
    bool sources) {
  auto edge_connections = (sources) ? from_edge->source_ids :
                                      from_edge->sink_ids;
  for (auto entity_id : edge_connections) {
    if (entity_id != exclude_id) {
      if (sources) {
        to_edge->AddSource(entity_id);
      } else {
        to_edge->AddSink(entity_id);
      }
      if (internal_nodes_.count(entity_id) != 0) {
        if (sources) {
          DirectedNode* node =
              dynamic_cast<DirectedNode*>(internal_nodes_[entity_id]);
          node->SwapSink(from_edge->id, to_edge->id);
        } else {
          DirectedNode* node =
              dynamic_cast<DirectedNode*>(internal_nodes_[entity_id]);
          node->SwapSource(from_edge->id, to_edge->id);
        }
      } else if (ports_.count(entity_id) != 0) {
        Port& p = ports_[entity_id];
        p.internal_edge_id = to_edge->id;
      } else {
        printf("Fatal error: Edge source not found in internal nodes");
        assert(false);
      }
    }
  }
}
