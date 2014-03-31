#include "ntl_parser.h"

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include <boost/tokenizer.hpp>
#include <fstream>
#include <set>
#include <utility>

#include "id_manager.h"
#include "node.h"
#include "universal_macros.h"

using namespace std;

void NtlParser::Parse(
    Node* graph, const char* filename, map<int, string>* edge_id_to_name) {
  parsed_modules_.clear();

  PopulateModuleTypeImplementationsMap();

  ifstream input_file(filename);
  assert_b(input_file.is_open()) {
    cout << "Failed to open graph file: " << filename
         << ". Terminating." << endl;
  }

  cout << "Reading graph from " << filename << endl;
  string line;
  std::getline(input_file, line);
  while (!input_file.eof()) {
    // Keep namespace to differentiate from getline syscall.
    assert(line == "module_begin");
    std::getline(input_file, line);
    vector<string> module_lines;
    while (line != "module_end") {
      assert(!input_file.eof());
      module_lines.push_back(line);
      std::getline(input_file, line);
    }

    ParsedModule module;

    assert (module_lines.size() >=2);
    module.type_name = module_lines[0];
    module.instance_name = module_lines[1];

    if (module_lines.size() < 4) {
      if (module.type_name != "GTECH_ONE" &&
          module.type_name != "GTECH_ZERO") {
        cout << "WARNING: Odd number of connections for module type: "
            << module_lines[0]
            << " instance: "
            << module_lines[1]
            << endl;
      }
    }
    for(size_t i = 2; i < module_lines.size(); ++i) {
      module.connected_edge_names.push_back(module_lines[i]);
    }
    // Sanity check on number of connected edges.
    if (module.connected_edge_names.size() > 200) {
      cout << "Warning: Over 200 connected edges for node "
            << module.instance_name << ". Normal?" << endl;
    }
    parsed_modules_.push_back(module);
    if (parsed_modules_.size() % 10000 == 0) {
      cout << "Parsed " << parsed_modules_.size() << " modules." << endl;
    }

    do { std::getline(input_file, line); } while (line.empty() && !input_file.eof());
  }

  PopulateGraph(graph, edge_id_to_name);
}

void NtlParser::PopulateModuleTypeImplementationsMap() {
  module_type_implementations_map_.clear();
 
  string implementation_filename = "implementation_file.txt";
  ifstream input_file(implementation_filename);

  assert_b(input_file.is_open()) {
    cout << "Can't open implementations file "
         << implementation_filename << endl;
    assert(false);
  }

  string line;
  while (!input_file.eof()) {
    // Keep namespace to differentiate from getline syscall.
    std::getline(input_file, line);
    vector<string> tokens;

    // Tokenize module string.
    boost::char_separator<char> delims(" ");
    boost::tokenizer<boost::char_separator<char>> tokenizer(line, delims);
    for (const string& token : tokenizer) {
      tokens.push_back(token);
    }
    if (tokens.empty()) continue;
    
    // TODO Add support to specify number of resources in implementation file.
    // For now assume all implementation files use 3 resources.
    assert_b((tokens.size() - 1) % 3 == 0) {
      cout << "Unexpected number of tokens: " << line << endl;
    }

    vector<vector<int>> impls;
    for (size_t i = 1; i < tokens.size(); i += 3) {
      vector<int> impl;
      for (size_t j = i; j < i + 3; ++j) {
        assert_b(tokens[i].find_last_not_of("0123456789") == string::npos) {
          cout << "Found non-number token when expecting number token in "
               << "string: " << line << endl;
        }
        impl.push_back(atoi(tokens[j].c_str()));
      }
      assert(impl.size() == 3);
      impls.push_back(impl);
    }
    module_type_implementations_map_.insert(make_pair(tokens[0], impls));
  }
  assert(module_type_implementations_map_.size() > 0);
}

void NtlParser::PopulateGraph(
    Node* graph, map<int, string>* edge_id_to_name) {
  assert(graph != nullptr);
  cout << "Populating Graph" << endl;
  map<string,int> edge_name_to_id;
  map<string,set<int>> edge_name_to_connection_ids;

  // Populate all nodes.
  cout << "Populating Nodes" << endl;
  for (auto& parsed_module : parsed_modules_) {
    int new_node_id = IdManager::AcquireNodeId();
    Node* new_node = new Node(new_node_id, parsed_module.instance_name);
    auto it = module_type_implementations_map_.find(parsed_module.type_name);
    if (it == module_type_implementations_map_.end()) {
      cout << "Could not find implementation details for module of type: "
           << parsed_module.type_name << endl;
    }
    vector<vector<int>> new_node_implementations =
        module_type_implementations_map_.at(parsed_module.type_name);
    assert(new_node_implementations.size() > 0);
    for (auto& impl : new_node_implementations) {
      new_node->AddWeightVector(impl);
    }
    for (const string& cnx_edge_name : parsed_module.connected_edge_names) {
      assert(!cnx_edge_name.empty());
      int new_edge_id;
      if (edge_name_to_id.find(cnx_edge_name) == edge_name_to_id.end()) {
        new_edge_id = IdManager::AcquireEdgeId();
        edge_name_to_id.insert(make_pair(cnx_edge_name, new_edge_id));
      } else {
        new_edge_id = edge_name_to_id.at(cnx_edge_name);
      }
      new_node->AddConnection(new_edge_id);
      edge_name_to_connection_ids[cnx_edge_name].insert(new_node_id);
    }
    graph->AddInternalNode(new_node_id, new_node);
  }

  // Remove any edges with very high fanout, as these are likely to be global
  // clock/reset signals.
  const int max_connected_nodes = 200;
  vector<string> edge_names_to_erase;
  for (auto& name_connection_ids_pair : edge_name_to_connection_ids) {
    const string& edge_name = name_connection_ids_pair.first;
    const set<int>& connected_node_ids = name_connection_ids_pair.second;
    if (connected_node_ids.size() > max_connected_nodes) {
      cout << "Removing high fanout edge: " << edge_name
           << " (" << connected_node_ids.size() << ")" << endl;
      edge_names_to_erase.push_back(edge_name);
      int rmv_edge_id = edge_name_to_id.at(edge_name);
      for (int node_id : connected_node_ids) {
        Node* node = graph->internal_nodes().at(node_id);
        assert(node->RemoveConnection(rmv_edge_id) > 0);
      }
    }
  }
  for (auto& erase_name : edge_names_to_erase) {
    edge_name_to_id.erase(erase_name);
    edge_name_to_connection_ids.erase(erase_name);
  }

  // Populate all edges.
  cout << "Populating Edges" << endl;
  assert(!edge_name_to_id.empty());
  for (auto& edge_name_id_pair : edge_name_to_id) {
    int new_edge_id = edge_name_id_pair.second;
    Edge* new_edge = new Edge(new_edge_id, 1, edge_name_id_pair.first);
    for (auto node_id :
         edge_name_to_connection_ids.at(edge_name_id_pair.first)) {
      new_edge->AddConnection(node_id);
    }
    graph->AddInternalEdge(new_edge_id, new_edge);
    if (edge_id_to_name != nullptr) {
      edge_id_to_name->insert(
          make_pair(new_edge_id, edge_name_id_pair.first));
    }
  }
}

void NtlParser::PrintImplementationMap() {
  cout << endl << "Printing Implementation Map" << endl;
  if (module_type_implementations_map_.empty()) {
    cout << "<EMPTY>" << endl;
  } else {
    for (auto& imp_pair : module_type_implementations_map_) {
      const string& type = imp_pair.first;
      const vector<vector<int>>& impls = imp_pair.second;
      cout << type << ": ";
      for (const vector<int>& impl : impls) {
        cout << "[ ";
        for (int wt : impl) {
          cout << wt << " ";
        }
        cout << "] ";
      }
      cout << endl;
    }
  }
  cout << endl;
}
