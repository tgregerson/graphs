#include "xml_parser.h"

#include <cstdio>
#include <cstring>
#include <utility>

#include "id_manager.h"

using std::make_pair;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

bool XmlParser::Parse(Node* top_level_graph, const char* filename) {
  xmlDocPtr doc;
  xmlParserCtxtPtr context; //Used for validation

  context = xmlNewParserCtxt();
  assert(context != NULL);

  doc = xmlCtxtReadFile(context, filename, NULL, XML_PARSE_DTDVALID);
  if (doc == NULL) {
    printf("Error parsing XML file: %s", filename);
    exit(1);
  } else if (context->valid == 0) {
    printf("XML validation failed for file %s.\n", filename);
    xmlFreeDoc(doc);
    exit(1);
  }
      
  xmlNodePtr rootNode = xmlDocGetRootElement(doc);
  if (rootNode == NULL) {
    printf("Couldn't find XML root node in XML file %s", filename);
    xmlFreeDoc(doc);
    exit(1);
  }

  if (!PopulateEntities(rootNode)) {
    Abort();
    return false;
  }
  PopulateConnections();

  // Add parsed nodes and edges to graph.
  for (auto it : parsed_nodes) {
    top_level_graph->AddInternalNode(it.first, it.second);
  }
  for (auto it : parsed_edges) {
    top_level_graph->AddInternalEdge(it.first, it.second);
  }
  for (auto it : parsed_ports) {
    // Ports are copied rather than being passed by pointer, so their memory
    // must be free'd.
    top_level_graph->AddPort(it.first, *(it.second));
    delete it.second;
  }

  // Clear temporary data structures, transfering control of pointers to
  // graph.
  Reset();
  return true;
}

void XmlParser::PopulateConnections() {
  // Node source ports/edges.
  PopulateConnectionsNodeToEdges(true);
  // Node sink ports/edges.
  PopulateConnectionsNodeToEdges(false);
  // Edge source nodes.
  PopulateConnectionsEdgeToNodesPorts(true);
  // Edge sink nodes.
  PopulateConnectionsEdgeToNodesPorts(false);
}

void XmlParser::PopulateConnectionsNodeToEdges(bool source_edges) {
  unordered_map<string, unordered_set<string>>& connection_map =
      (source_edges) ? node_to_source_edges_connection_names :
                       node_to_sink_edges_connection_names;
  Port::PortType port_type = (source_edges) ?
      Port::kInputType : Port::kOutputType;

  for (auto it : connection_map) {
    const string& node_name = it.first;
    unordered_set<string>& edge_connections = it.second;
    assert(node_name_to_id.count(node_name));
    int node_id = node_name_to_id[node_name];
    assert(parsed_nodes.count(node_id));
    Node* node = parsed_nodes[node_id];
    for (auto edge_cnx_name : edge_connections) {
      assert(edge_name_to_id.count(edge_cnx_name));
      int edge_cnx_id = edge_name_to_id[edge_cnx_name];
      Port port(
        IdManager::AcquireNodeId(),
        IdManager::kReservedTerminalId,
        edge_cnx_id,
        port_type);
      node->AddPort(port.id, port);
    }
  }
}

void XmlParser::PopulateConnectionsEdgeToNodesPorts(bool source_nodes) {
  unordered_map<string, unordered_set<string>>& connection_map =
      (source_nodes) ? edge_to_source_nodes_ports_connection_names :
                       edge_to_sink_nodes_ports_connection_names;
  for (auto it : connection_map) {
    const string& edge_name = it.first;
    unordered_set<string>& node_port_connections = it.second;
    assert(edge_name_to_id.count(edge_name));
    int edge_id = edge_name_to_id[edge_name];
    assert(parsed_edges.count(edge_id));
    Edge* edge = parsed_edges[edge_id];
    for (auto node_port_cnx_name : node_port_connections) {
      assert(node_name_to_id.count(node_port_cnx_name) ||
             port_name_to_id.count(node_port_cnx_name));
      // Edges may be connected to either nodes or ports, so we need to
      // determine which the ID refers to before processing it.
      int node_port_cnx_id;
      if (node_name_to_id.count(node_port_cnx_name)) {
        // Connection is a node.
        node_port_cnx_id = node_name_to_id[node_port_cnx_name];
      } else {
        // Connection is a port.
        node_port_cnx_id = port_name_to_id[node_port_cnx_name];
        assert(parsed_ports.count(node_port_cnx_id));
        parsed_ports[node_port_cnx_id]->internal_edge_id = edge_id;
        assert(node_port_cnx_id != 0);
      }
      edge->AddConnection(node_port_cnx_id);
    }
  }
}

bool XmlParser::PopulateEntities(xmlNodePtr nodePtr) {
  assert(!strcmp((char*)(nodePtr->name), "graph"));
  assert(nodePtr->children != NULL);

  xmlNodePtr curNodePtr = NULL;

  for (curNodePtr = nodePtr->children; curNodePtr;
       curNodePtr = curNodePtr->next) {
    if (curNodePtr->type == XML_ELEMENT_NODE) {
      if (!strcmp((char*)(curNodePtr->name), "node")) {
        Node* newNode = new Node(IdManager::AcquireNodeId());
        PopulateNodeFromXml(newNode, curNodePtr);
        parsed_nodes.insert(make_pair(newNode->id, newNode));
      } else if (!strcmp((char*)(curNodePtr->name), "edge")) {
        Edge* newEdge = new Edge(IdManager::AcquireEdgeId());
        PopulateEdgeFromXml(newEdge, curNodePtr);
        parsed_edges.insert(make_pair(newEdge->id, newEdge));
      } else if (!strcmp((char*)(curNodePtr->name), "port")) {
        Port* newPort = new Port(IdManager::AcquireNodeId());
        PopulatePortFromXml(newPort, curNodePtr);
        parsed_ports.insert(make_pair(newPort->id, newPort));
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

void XmlParser::PopulateNodeFromXml(Node* newNode,
                                    xmlNodePtr myNodePtr) {
  assert(myNodePtr != NULL);
  assert(myNodePtr->type == XML_ELEMENT_NODE);
  assert(!strcmp((char*)(myNodePtr->name), "node"));
  assert(myNodePtr->children != NULL);

  int node_id = newNode->id;

  vector<string> sources;
  vector<string> sinks;

  for (myNodePtr = myNodePtr->children; myNodePtr != NULL;
      myNodePtr = myNodePtr->next) {
    assert(myNodePtr->name != NULL);
    if (!strcmp((char*)(myNodePtr->name),"name")) {
      assert(myNodePtr->children->content != NULL);
      newNode->name = (char*)(myNodePtr->children->content);
      assert(node_name_to_id.count(newNode->name) == 0);
      node_name_to_id.insert(make_pair(newNode->name, node_id));
    } else if (!strcmp((char*)(myNodePtr->name),"resource_weights")) {
      // This code requires that weights are given in the same order for all
      // nodes, and no weights are omitted.
      vector<int> resource_weights;
      xmlNodePtr weightPtr = myNodePtr->children;
      while (weightPtr) {
	assert(weightPtr->children != NULL);
        int weight = atoi((char*)(weightPtr->children->content));
        resource_weights.push_back(weight);
        weightPtr = weightPtr->next;
      }
      newNode->AddWeightVector(resource_weights);
    } else if(!strcmp((char*)(myNodePtr->name),"source_edges")) {
      xmlNodePtr sourcePtr = myNodePtr->children;
      for(; sourcePtr != NULL; sourcePtr = sourcePtr->next) {
        string s = (char*)(sourcePtr->children->content);
        sources.push_back(s);
      }
    } else if(!strcmp((char*)(myNodePtr->name),"sink_edges")) {
      xmlNodePtr sinkPtr = myNodePtr->children;
      for (; sinkPtr != NULL; sinkPtr = sinkPtr->next) {
        string s = (char*)(sinkPtr->children->content);
        sinks.push_back(s);
      }
    } else {
      printf("Unknown module element: --%s--\nDid you run verification with "
             "the DTD?\n", myNodePtr->name);
      exit(1);
    }
  }

  string nodeName = newNode->name;
  assert(!nodeName.empty());
  for (auto& it : sources) {
    node_to_source_edges_connection_names[nodeName].insert(it);
    edge_to_sink_nodes_ports_connection_names[it].insert(nodeName);
  }
  for (auto& it : sinks) {
    node_to_sink_edges_connection_names[nodeName].insert(it);
    edge_to_source_nodes_ports_connection_names[it].insert(nodeName);
  }
}

void XmlParser::PopulateEdgeFromXml(Edge* newEdge,
                                    xmlNodePtr myNodePtr) {
  assert(myNodePtr != NULL);
  assert(myNodePtr->type == XML_ELEMENT_NODE);
  assert(!strcmp((char*)(myNodePtr->name), "edge"));
  assert(myNodePtr->children != NULL);

  for (myNodePtr = myNodePtr->children; myNodePtr != NULL;
      myNodePtr = myNodePtr->next) {
    assert(myNodePtr->name != NULL);
    if (!strcmp((char*)(myNodePtr->name),"name")) {
      assert(myNodePtr->children->content != NULL);
      newEdge->name = (char*)(myNodePtr->children->content);
      edge_name_to_id.insert(make_pair(newEdge->name, newEdge->id));
    } else if (!strcmp((char*)(myNodePtr->name),"weight")) {
      assert(myNodePtr->children->content);
      newEdge->weight = atoi((char*)(myNodePtr->children->content));
    } else {
      printf("Unknown edge element: --%s--\nDid you run verification with "
              "the DTD?\n", myNodePtr->name);
      exit(1);
    }
  }
}

void XmlParser::PopulatePortFromXml(Port* newPort, xmlNodePtr myNodePtr) {
  assert(myNodePtr != NULL);
  assert(myNodePtr->type == XML_ELEMENT_NODE);
  assert(!strcmp((char*)(myNodePtr->name), "port"));
  assert(myNodePtr->children != NULL);

  // Ports described in the graph xml have no external connections.
  newPort->external_edge_id = IdManager::kReservedTerminalId;

  for (myNodePtr = myNodePtr->children; myNodePtr != NULL;
      myNodePtr = myNodePtr->next) {
    assert(myNodePtr->name != NULL);
    if (!strcmp((char*)(myNodePtr->name),"name")) {
      assert(myNodePtr->children->content != NULL);
      newPort->name = (char*)(myNodePtr->children->content);
      port_name_to_id.insert(make_pair(newPort->name, newPort->id));
    } else if (!strcmp((char*)(myNodePtr->name),"source_edge")) {
      assert(myNodePtr->children->content != NULL);
      string src_edge((char*)(myNodePtr->children->content));
      edge_to_sink_nodes_ports_connection_names[src_edge].insert(newPort->name);
    } else if (!strcmp((char*)(myNodePtr->name),"sink_edge")) {
      assert(myNodePtr->children->content != NULL);
      string sink_edge((char*)(myNodePtr->children->content));
      edge_to_source_nodes_ports_connection_names[sink_edge].insert(
          newPort->name);
    } else {
      printf("Unknown edge element: --%s--\nDid you run verification with "
              "the DTD?\n", myNodePtr->name);
      exit(1);
    }
  }
}

void XmlParser::Abort() {
  for (auto it : parsed_nodes) {
    delete it.second;
  }
  for (auto it : parsed_edges) {
    delete it.second;
  }
  Reset();
}

void XmlParser::Reset() {
  node_to_source_edges_connection_names.clear();
  node_to_sink_edges_connection_names.clear();
  edge_to_source_nodes_ports_connection_names.clear();
  edge_to_sink_nodes_ports_connection_names.clear();
  node_name_to_id.clear();
  edge_name_to_id.clear();
  parsed_nodes.clear();
  parsed_edges.clear();
}
