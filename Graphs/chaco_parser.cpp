#include "chaco_parser.h"

#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>

#include "id_manager.h"

using namespace std;

bool ChacoParser::Parse(Node* top_level_graph, const char* filename) {
  base_filename_.assign(filename);
  input_file_.open(filename);

  string cur_line;
  if (!input_file_.is_open()) {
    printf("Failed to open %s\n", filename);
    return Finish(false);
  }

  if (!ParseHeaderLine()) {
    printf("An error occurred while parsing the header line.\n");
    return Finish(false);
  }
  IdManager::Reset(num_nodes_ + 1);

  // In CHACO format, the line number and the node number are the same.
  printf("Parse: Extracting graph structure from file %s.\n", filename);
  int node_num = 1;
  getline(input_file_, cur_line);
  while (!input_file_.eof()) {
    if ((node_num % 50000) == 0) {
      printf("Processed %d nodes\n", node_num);
    }
    if (input_file_.bad()) {
      printf("Error while reading %s\n", filename);
      return Finish(false);
    }
    if (cur_line.empty()) {
      printf("Warning, empty line on %d\n", node_num);
      node_num++;
      getline(input_file_, cur_line);
      continue;
    }
    vector<int> line_ints = ExtractIntsFromLine(cur_line);
    assert(!line_ints.empty());
    cur_line.clear();

    // Check that the number of entries is consistent with the mode.
    int temp_entries = line_ints.size();
    if (node_weight_mode_ == USER_SPECIFIED_WEIGHT_MODE) {
      temp_entries--;
    }
    if ((temp_entries < 1) ||
        (edge_weight_mode_ == USER_SPECIFIED_WEIGHT_MODE &&
         (temp_entries % 2 != 0))) {
      printf("Error: Invalid number of entries (%ld) on line %d.\n",
             line_ints.size(), node_num);
      return Finish(false);
    }

    Node* new_node = new Node(node_num);
    parsed_nodes_.insert(make_pair(node_num, new_node));

    int pos = 0;
    if (node_weight_mode_ == USER_SPECIFIED_WEIGHT_MODE) {
      int weight = line_ints[pos++];
      vector<int> weight_vec = {weight};
      new_node->AddWeightVector(weight_vec);
    } else if (node_weight_mode_ == UNITARY_WEIGHT_MODE) {
      vector<int> weight_vec = {1};
      new_node->AddWeightVector(weight_vec);
    }
    // Handle externally specified weights later.

    while (pos < line_ints.size()) {
      int connected_node_id = line_ints[pos++];
      // Create a new edge when the current node is lower than the connected
      // node. This avoids creating duplicated nodes, but only works for
      // graphs, not hypergraphs (CHACO only describes graphs).
      if (node_num < connected_node_id) {
        Edge* connecting_edge = new Edge(IdManager::AcquireEdgeId());
        connecting_edge->AddConnection(node_num);
        connecting_edge->AddConnection(connected_node_id);
        pair<int,Edge*> my_pair =
            make_pair(connecting_edge->id, connecting_edge);
        parsed_edges_.insert(my_pair);
        new_node->AddConnection(connecting_edge->id);
        back_connections[connected_node_id].insert(
            make_pair(node_num, connecting_edge->id));
        if (edge_weight_mode_ == USER_SPECIFIED_WEIGHT_MODE) {
          connecting_edge->weight = line_ints[pos++];
        } else {
          connecting_edge->weight = 1;
        }
      } else {
        // Add connection to an edge that was created in an earlier iteration.
        assert(back_connections.count(node_num) != 0);
        assert(back_connections[node_num].count(connected_node_id) != 0);
        int connected_edge_id = back_connections[node_num][connected_node_id];
        new_node->AddConnection(connected_edge_id);
        if (edge_weight_mode_ == USER_SPECIFIED_WEIGHT_MODE) {
          // Skip the weight since it was already set earlier. Does not check
          // to make weights don't conflict.
          pos++;
        }
      }
    }
    node_num++;
    getline(input_file_, cur_line);
  }
  // Check that the number of nodes and edges processed matches what was in the
  // header line.
  if (num_nodes_ != parsed_nodes_.size()) {
    printf("Error: Graph file header line specified %d nodes, but found %ld",
           num_nodes_, parsed_nodes_.size());
    return Finish(false);
  }
  if (num_edges_ != parsed_edges_.size()) {
    printf("Error: Graph file header line specified %d edges, but found %ld",
           num_edges_, parsed_edges_.size());
    return Finish(false);
  }

  if (node_weight_mode_ == EXTERNAL_USER_SPECIFIED_WEIGHT_MODE) {
    if(!ParseExternalWeightFile()) {
      printf("An error occurred while parsing external weights.\n");
      Finish(false);
    }
  }

  // Transfer parsed nodes and edges to graph.
  printf("Parse: Adding nodes to internal graph.\n");
  for (auto it : parsed_nodes_) {
    top_level_graph->AddInternalNode(it.first, it.second);
  }
  printf("Parse: Adding edges to internal graph.\n");
  for (auto it : parsed_edges_) {
    top_level_graph->AddInternalEdge(it.first, it.second);
  }
  printf("Parse: Done parsing.\n");
  printf("Parse: Found %lu nodes.\n", parsed_nodes_.size());
  printf("Parse: Found %lu edges.\n", parsed_edges_.size());
  return Finish(true);
}

bool ChacoParser::ParseHeaderLine() {
  // Read setup line.
  string header_line;
  while (header_line.empty()) {
    getline(input_file_, header_line);
    assert(input_file_.good());
  }
  vector<int> setup_vals = ExtractIntsFromLine(header_line);
  if (setup_vals.size() < 2 || setup_vals.size() > 3) {
    printf("Error parsing file. First line of file contains %ld values.",
           setup_vals.size());
    printf("The first line of a CHACO formatted file must be as follows:\n");
    printf("NUM_NODES NUM_EDGES [WEIGHTING_MODE]\n");
    return false;
  }

  num_nodes_ = setup_vals[0];
  num_edges_ = setup_vals[1];
  if (setup_vals.size() == 3) {
    switch(setup_vals[2]) {
      case kChacoUnueMode:
        printf("Setting mode UNUE\n");
        node_weight_mode_ = UNITARY_WEIGHT_MODE;
        edge_weight_mode_ = UNITARY_WEIGHT_MODE;
        break;
      case kChacoUnseMode:
        printf("Setting mode UNSE\n");
        node_weight_mode_ = UNITARY_WEIGHT_MODE;
        edge_weight_mode_ = USER_SPECIFIED_WEIGHT_MODE;
        break;
      case kChacoSnueMode:
        printf("Setting mode SNUE\n");
        node_weight_mode_ = USER_SPECIFIED_WEIGHT_MODE;
        edge_weight_mode_ = UNITARY_WEIGHT_MODE;
        break;
      case kChacoSnseMode:
        printf("Setting mode SNSE\n");
        node_weight_mode_ = USER_SPECIFIED_WEIGHT_MODE;
        edge_weight_mode_ = USER_SPECIFIED_WEIGHT_MODE;
        break;
      case kChacoXnueMode:
        printf("Setting mode XNUE\n");
        node_weight_mode_ = EXTERNAL_USER_SPECIFIED_WEIGHT_MODE;
        edge_weight_mode_ = UNITARY_WEIGHT_MODE;
        break;
      case kChacoXnseMode:
        printf("Setting mode XNSE\n");
        node_weight_mode_ = EXTERNAL_USER_SPECIFIED_WEIGHT_MODE;
        edge_weight_mode_ = USER_SPECIFIED_WEIGHT_MODE;
        break;
      default:
        printf("Unrecognized CHACO weight mode value: %d\n", setup_vals[2]);
        return false;
    }
  } else {
    printf("No mode specified. Setting mode UNUE by default.\n");
    node_weight_mode_ = UNITARY_WEIGHT_MODE;
    edge_weight_mode_ = UNITARY_WEIGHT_MODE;
  }
  return true;
}

bool ChacoParser::ParseWeightHeaderLine() {
  // Read setup line.
  string header_line;
  while (header_line.empty()) {
    getline(weight_file_, header_line);
    assert(weight_file_.good());
    assert(!weight_file_.eof());
  }
  vector<int> setup_vals = ExtractIntsFromLine(header_line);
  if (setup_vals.size() != 2) {
    printf("Error parsing weight file. First line of file contains %ld values.",
           setup_vals.size());
    printf("The first line of a weight file must be as follows:\n");
    printf("NUM_NODES NUM_WEIGHTS_IN_WEIGHT_VECTOR\n");
    return false;
  }

  // Make sure number of nodes specified in weight file matches number specified
  // in graph description.
  assert(setup_vals[0] == num_nodes_);

  num_entries_per_weight_vector_ = setup_vals[1];
  assert(num_entries_per_weight_vector_ > 0);
  return true;
}

std::vector<int> ChacoParser::ExtractIntsFromLine(const string& line) const {
  if (line.empty()) {
    return vector<int>();
  }
  istringstream line_stream(line);
  istream_iterator<int> eos;
  istream_iterator<int> stream_it(line_stream);
  return vector<int>(stream_it, eos);
}

bool ChacoParser::ParseExternalWeightFile() {
  int num_nodes = parsed_nodes_.size();
  string weight_filename = base_filename_;
  weight_filename.append(".wt");
  printf("Parse: Extracting weights from file %s.\n", weight_filename.c_str());

  weight_file_.open(weight_filename.c_str());

  string cur_line;
  if (!weight_file_.is_open()) {
    printf("Failed to open %s\n", weight_filename.c_str());
    return false;
  }

  if (!ParseWeightHeaderLine()) {
    printf("An error occurred while parsing the header line.\n");
    return false;
  }
  assert(!weight_file_.eof());

  int node_num = 1;
  getline(weight_file_, cur_line);
  while (!weight_file_.eof()) {
    if ((node_num % 50000) == 0) {
      printf("Processed %d weights\n", node_num);
    }
    if (weight_file_.bad()) {
      printf("Error while reading %s\n", weight_filename.c_str());
      return false;
    }
    vector<int> weight_vectors = ExtractIntsFromLine(cur_line);
    cur_line.clear();
    assert(weight_vectors.size() % num_entries_per_weight_vector_ == 0);
    int num_weight_vectors =
        weight_vectors.size() / num_entries_per_weight_vector_;
    for (int start_offset = 0; start_offset < weight_vectors.size();
         start_offset += num_entries_per_weight_vector_) {
      vector<int> weight_vector;
      for (int entry_offset = 0; entry_offset < num_entries_per_weight_vector_;
           entry_offset++) {
        weight_vector.push_back(weight_vectors[start_offset + entry_offset]);
      }
      parsed_nodes_[node_num]->AddWeightVector(weight_vector);
    }
    node_num++;
    getline(weight_file_, cur_line);
  }

  assert(--node_num == parsed_nodes_.size());

  return true;
}

bool ChacoParser::Finish(bool ret_val) {
  // Clean-up temporary storage.
  if (!ret_val) {
    // Parsing was killed with an error. Temporary data must be free'd,
    // since the pointers were not transfered to the graph object.
    for (auto it : parsed_nodes_) {
      delete it.second;
    }
    for (auto it : parsed_edges_) {
      delete it.second;
    }
  }
  parsed_nodes_.clear();
  parsed_edges_.clear();

  input_file_.close();
  return ret_val;
}
