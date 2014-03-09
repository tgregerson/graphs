/* NOTE: To link with the libxml libraries, use `xml2-config --cflags --libs` when compiling with GCC.
** Must have LIBXML2 installed to compile this code: sudo apt-get install libxml */

#ifndef XML_PARSER_H_
#define XML_PARSER_H_

#include "parser_interface.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "directed_node.h"
#include "directed_edge.h"
#include "port.h"

class XmlParser : ParserInterface {
 public:
  /* Populates a graph from XML data in 'filename'.
     This function should be run once after reading the XML file
     Returns true if no parsing errors */
  virtual bool Parse(Node* top_level_graph, const char* filename);

 private:
  // Reset internal state and delete any pointers stored in internal containers.
  void Abort();
  // Reset private internal state.
  void Reset();
  // Cycle through parsed nodes and edges, adding connections to other nodes
  // and edges based on the connection names, translating them to the correct
  // node and edge IDs in the process.
  void PopulateConnections();
  void PopulateConnectionsNodeToEdges(bool source_edges);
  void PopulateConnectionsEdgeToNodesPorts(bool source_nodes);
  bool PopulateEntities(xmlNodePtr nodePtr);
  void PopulateNodeFromXml(DirectedNode* newNode, xmlNodePtr curNodePtr);
  void PopulateEdgeFromXml(DirectedEdge* newEdge, xmlNodePtr curNodePtr);
  void PopulatePortFromXml(Port* newPort, xmlNodePtr curNodePtr);

  // Maps of the connections of each node and edge, using the name strings
  // specified in the graph description.
  std::unordered_map<std::string, std::unordered_set<std::string>>
    node_to_source_edges_connection_names;
  std::unordered_map<std::string, std::unordered_set<std::string>>
    node_to_sink_edges_connection_names;
  std::unordered_map<std::string, std::unordered_set<std::string>>
    edge_to_source_nodes_ports_connection_names;
  std::unordered_map<std::string, std::unordered_set<std::string>>
    edge_to_sink_nodes_ports_connection_names;

  // Maps to translate from entity name to ID. Note that this requires that
  // entity names are unique in the graph description.
  std::unordered_map<std::string, int> node_name_to_id;
  std::unordered_map<std::string, int> edge_name_to_id;
  std::unordered_map<std::string, int> port_name_to_id;

  // Used for temporary storage during parsing. Ownership of pointers will
  // be passed to the graph if parsing succeeds.
  std::unordered_map<int, DirectedNode*> parsed_nodes;
  std::unordered_map<int, DirectedEdge*> parsed_edges;
  std::unordered_map<int, Port*> parsed_ports;
};

#endif /* XML_PARSER_H_ */
