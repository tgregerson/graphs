/*
 * functional_netlist_parser_main.cpp
 *
 *  Created on: Sep 9, 2014
 *      Author: gregerso
 */

#include <cassert>

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include "functional_edge.h"
#include "functional_node.h"
#include "structural_netlist_parser.h"

using namespace std;

int main(int argc, char *argv[]) {
  StructuralNetlistParser parser;

  ifstream v_file;
  ofstream out_file;
  if (argc != 2 && argc != 3) {
    parser.PrintHelpAndDie();
    exit(1);
  } else {
    // Open file.
    string file_to_read = argv[1];
    v_file.open(file_to_read.c_str());
    if (!v_file.is_open()) {
      cout << "Can't open input file " << file_to_read << endl;
      exit(1);
    }
    if (argc == 3) {
      string file_to_write = argv[2];
      out_file.open(file_to_write.c_str());
      if (!out_file.is_open()) {
        cout << "Can't open output file " << file_to_write << endl;
        exit(1);
      }
    }
  }

  map<string, FunctionalEdge*> functional_edges;
  map<string, FunctionalNode*> functional_nodes;

  // If no output file, redirect to cout.
  ostream& output_stream = out_file.is_open() ? out_file : cout;

  // Parsed lines will contain a line that terminates with a ';'.
  list<string> parsed_lines;
  while (v_file.good()) {
    string line;
    getline(v_file, line);
    line = parser.trimLeadingTrailingWhiteSpace(line);
    if (line.empty()) continue;
    // Is the 'ends with ;' check really necessary? Parser handles
    // splitting and merging lines...
    while (line.back() != ';' && v_file.good()) {
      string next_line;
      getline(v_file, next_line);
      if (next_line.empty()) continue;
      line.append(" ");
      line.append(parser.trimLeadingTrailingWhiteSpace(next_line));
    }
    parsed_lines.push_back(line);
  }
  parsed_lines =
      parser.RawLinesToSemicolonTerminated(parsed_lines);

  parser.PopulateFunctionalNodes(&functional_nodes, parsed_lines);
  parser.PopulateFunctionalEdges(&functional_edges, parsed_lines);

  FunctionalEdge* const0 = functional_edges.at("\\<const0> ");
  const0->SetEntropy(0.0);
  const0->SetProbabilityOne(0.0);
  FunctionalEdge* const1 = functional_edges.at("\\<const1> ");
  const1->SetEntropy(0.0);
  const1->SetProbabilityOne(1.0);

  // Remove synthesis-tool-specific nets that should not be included in graph.
  /*
  set<string> blacklisted_global_nets = {
    "CLK_IBUF_BUFG",
    "RST_IBUF",
    "\\<const0>",
    "\\<const1>",
    "GND",
    "GND_2",
    "CLK_IBUF",
    "VCC",
    "VCC_2",
    "CLK",
  };
  parser.RemoveNetConnectionFromModules(&modules, blacklisted_global_nets);
  */

  /*
  set<string> blacklisted_net_substrings = {
    "UNCONNECTED",
  };
  parser.RemoveNetConnectionFromModulesSubstring(
      &modules, blacklisted_net_substrings);
  */

  for (auto it : functional_edges) {
    cout << it.first << " " << it.second->width << endl;
  }

  // Add expanding of subranges.
  //parser.ExpandModuleBusConnections(nets, &modules);
  parser.ExpandFunctionalNodeBusConnections(
      functional_edges, &functional_nodes);
  functional_edges = parser.ConvertEdgesToSingleBit(functional_edges);

  for (auto it : functional_edges) {
    cout << it.first << " " << it.second->width << endl;
  }

  parser.PopulateFunctionalEdgePorts(functional_nodes, &functional_edges);

  // Write output in NTL format.
  /*
  for (const auto& module_pair : modules) {
    parser.PrintModuleXNtlFormat(module_pair.second, nets, output_stream);
    output_stream << endl;
  }
  */
  for (const auto& node_pair : functional_nodes) {
    parser.PrintFunctionalNodeXNtlFormat(
        node_pair.second, &functional_edges, &functional_nodes, output_stream);
    output_stream << endl;
  }

  for (const auto& edge_pair : functional_edges) {
    if (edge_pair.second->Entropy(&functional_edges, &functional_nodes) < 1.0 ||
        edge_pair.first == "n_0_zrl_proc_i_1") {
      output_stream << edge_pair.first << ": ("
                    << edge_pair.second->Entropy(&functional_edges, &functional_nodes)
                    << ") ("
                    << edge_pair.second->ProbabilityOne(0, &functional_edges, &functional_nodes)
                    << ")\n";
    }
  }

  /*
  map<string, int> edge_connection_counts;
  for (const auto& module_pair : modules) {
    for (const string& edge_name : module_pair.second.connected_net_names) {
      if (edge_connection_counts.find(edge_name) == edge_connection_counts.end()) {
        edge_connection_counts.insert(make_pair(edge_name, 1));
      } else {
        edge_connection_counts[edge_name]++;
      }
    }
  }
  for (const auto& ecc_pair : edge_connection_counts) {
    output_stream << ecc_pair.first << " " << ecc_pair.second << "\n";
  }
  */

  if (out_file.is_open()) {
    out_file.close();
  }

  return 0;
}



