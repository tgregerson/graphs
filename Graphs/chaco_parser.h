#ifndef CHACO_PARSER_H_
#define CHACO_PARSER_H_

#include "parser_interface.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "node.h"
#include "edge.h"

// CHACO mode constants.
const int kChacoUnueMode = 0; // Unitary node, unitary edge weight.
const int kChacoUnseMode = 1; // Unitary node, user-spec edge weight.
const int kChacoSnueMode = 10; // User-spec node, unitary edge weight.
const int kChacoSnseMode = 11; // User-spec node, user-spec edge weight.

// Extended mode constants.
const int kChacoXnueMode = 20; // Externally-specified node, unitary edge.
const int kChacoXnseMode = 21; // Externally-specified node, user-spec edge.

class ChacoParser : ParserInterface {
 public:
  typedef enum {
    UNITARY_WEIGHT_MODE,
    USER_SPECIFIED_WEIGHT_MODE,
    EXTERNAL_USER_SPECIFIED_WEIGHT_MODE
  } ChacoWeightMode;

  ~ChacoParser() {}

  virtual bool Parse(Node* top_level_graph, const char* filename);

 private:
  bool ParseHeaderLine();
  bool ParseWeightHeaderLine();
  std::vector<int> ExtractIntsFromLine(const std::string& line) const;
  bool ParseExternalWeightFile();
  bool Finish(bool ret_val);

  std::string base_filename_;
  std::ifstream input_file_;
  std::ifstream weight_file_;

  int num_nodes_;
  int num_edges_;
  int num_entries_per_weight_vector_;
  ChacoWeightMode node_weight_mode_;
  ChacoWeightMode edge_weight_mode_;

  // Used to track edge ids for node to node connections. When an edge is
  // created that connects to a node that hasn't been created yet, the node's
  // ID (which can be determined a priori) is inserted as the first index. The
  // second index is the other node connected to the edge, and the value is
  // the edge ID.
  std::map<int, std::map<int, int>> back_connections;

  // Temporary storage for parsed elements.
  std::unordered_map<int, Node*> parsed_nodes_;
  std::unordered_map<int, Edge*> parsed_edges_;
};

#endif /* CHACO_PARSER_H_ */
