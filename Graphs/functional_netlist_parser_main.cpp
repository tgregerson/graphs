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
#include "xilinx_functional_nodes.h"
#include "structural_netlist_parser.h"

using namespace std;

void PrintHelpAndDie() {
  cout << "Usage: ./functional_netlist_parser netlist_file [entropy_file [parsed_netlist]]"
       << endl;
  exit(1);
}

int main(int argc, char *argv[]) {
  StructuralNetlistParser parser;

  ifstream v_file;
  ifstream p1_file;
  ofstream out_file;
  if (argc < 2 || argc > 4) {
    PrintHelpAndDie();
    exit(1);
  } else {
    // Open file.
    string file_to_read = argv[1];
    v_file.open(file_to_read.c_str());
    if (!v_file.is_open()) {
      cout << "Can't open input file " << file_to_read << endl;
      exit(1);
    }
    if (argc > 2) {
      string p1_file_name = argv[2];
      p1_file.open(p1_file_name.c_str());
      if (!p1_file.is_open()) {
        cout << "Can't open entropy input file " << p1_file_name << endl;
        exit(1);
      }
    }
    if (argc > 3) {
      string file_to_write = argv[3];
      out_file.open(file_to_write.c_str());
      if (!out_file.is_open()) {
        cout << "Can't open output file " << file_to_write << endl;
        exit(1);
      }
    }
  }

  map<string, FunctionalEdge*> functional_edges;
  map<string, FunctionalEdge*> functional_wires;
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

  parser.PopulateFunctionalEdges(&functional_edges, parsed_lines);
  for (auto f_edge_pair : functional_edges) {
    FunctionalEdge* edge = f_edge_pair.second;
    for (int bit = edge->BitLow(); bit <= edge->BitHigh(); ++bit) {
      FunctionalEdge* wire = new FunctionalEdge(edge->BaseName(), bit, bit);
      functional_wires.insert(make_pair(wire->IndexedName(), wire));
    }
  }
  parser.PopulateFunctionalNodes(
      functional_edges, functional_wires, &functional_nodes, parsed_lines);

  FunctionalEdge* const0 = functional_edges.at("\\<const0> ");
  const0->SetProbabilityOne(0.0);
  FunctionalEdge* const0w = functional_wires.at("\\<const0> [0]");
  const0w->SetProbabilityOne(0.0);
  FunctionalEdge* const1 = functional_edges.at("\\<const1> ");
  const1->SetProbabilityOne(1.0);
  FunctionalEdge* const1w = functional_wires.at("\\<const1> [0]");
  const1w->SetProbabilityOne(1.0);
  if (p1_file.is_open()) {
    parser.PrePopulateProbabilityOnes(p1_file, &functional_wires);
  }

  parser.PopulateFunctionalEdgePorts(
      functional_nodes, &functional_edges, &functional_wires);

  for (const auto& wire_pair : functional_wires) {
    FunctionalEdge* wire = wire_pair.second;
    wire->Entropy(&functional_wires, &functional_nodes);
  }

  // Remove synthesis-tool-specific nets that should not be included in graph.
  set<string> blacklisted_global_nets = {
    "CLK_IBUF_BUFG[0]",
    "RST_IBUF[0]",
    "\\<const0> [0]",
    "\\<const1> [0]",
    "GND[0]",
    "GND_2[0]",
    "CLK_IBUF[0]",
    "VCC[0]",
    "VCC_2[0]",
    "CLK[0]",
    "RST[0]",
  };
  parser.RemoveWires(&functional_nodes, &functional_wires, blacklisted_global_nets);

  set<string> blacklisted_net_substrings = {
    "UNCONNECTED",
  };
  parser.RemoveWiresBySubstring(
      &functional_nodes, &functional_wires, blacklisted_net_substrings);

  parser.RemoveUnconnectedNodes(&functional_nodes);

  /*
  const double BIN1_MIN = 0.999;
  const double BIN2_MIN = 0.95;
  const double BIN3_MIN = 0.90;
  const double BIN4_MIN = 0.80;
  const double BIN5_MIN = 0.60;
  const double BIN6_MIN = 0.10;
  const double BIN7_MIN = 0.01;
  const double BIN8_MIN = 0.0;

  map<double, int> bin_count;

  for (const auto& wire_pair : functional_wires) {
    FunctionalEdge* wire = wire_pair.second;
    double reported_entropy = wire->Entropy(&functional_wires,
                                            &functional_nodes);
    if (reported_entropy >= BIN1_MIN) {
      bin_count[BIN1_MIN]++;
    } else if (reported_entropy >= BIN2_MIN) {
      bin_count[BIN2_MIN]++;
    } else if (reported_entropy >= BIN3_MIN) {
      bin_count[BIN3_MIN]++;
    } else if (reported_entropy >= BIN4_MIN) {
      bin_count[BIN4_MIN]++;
    } else if (reported_entropy >= BIN5_MIN) {
      bin_count[BIN5_MIN]++;
    } else if (reported_entropy >= BIN6_MIN) {
      bin_count[BIN6_MIN]++;
    } else if (reported_entropy >= BIN7_MIN) {
      bin_count[BIN7_MIN]++;
    } else {
      bin_count[BIN8_MIN]++;
    }
  }

  output_stream << "[1.0:" << BIN1_MIN << "] "
       << (100.0 * bin_count[BIN1_MIN] / functional_wires.size())
       << "% " << bin_count[BIN1_MIN] << endl;
  output_stream << "[" << BIN1_MIN << ":" << BIN2_MIN << "] "
       << (100.0 * bin_count[BIN2_MIN] / functional_wires.size())
       << "% " << bin_count[BIN2_MIN] << endl;
  output_stream << "[" << BIN2_MIN << ":" << BIN3_MIN << "] "
       << (100.0 * bin_count[BIN3_MIN] / functional_wires.size())
       << "% " << bin_count[BIN3_MIN] << endl;
  output_stream << "[" << BIN3_MIN << ":" << BIN4_MIN << "] "
       << (100.0 * bin_count[BIN4_MIN] / functional_wires.size())
       << "% " << bin_count[BIN4_MIN] << endl;
  output_stream << "[" << BIN4_MIN << ":" << BIN5_MIN << "] "
       << (100.0 * bin_count[BIN5_MIN] / functional_wires.size())
       << "% " << bin_count[BIN5_MIN] << endl;
  output_stream << "[" << BIN5_MIN << ":" << BIN6_MIN << "] "
       << (100.0 * bin_count[BIN6_MIN] / functional_wires.size())
       << "% " << bin_count[BIN6_MIN] << endl;
  output_stream << "[" << BIN6_MIN << ":" << BIN7_MIN << "] "
       << (100.0 * bin_count[BIN7_MIN] / functional_wires.size())
       << "% " << bin_count[BIN7_MIN] << endl;
  output_stream << "[" << BIN7_MIN << ":" << BIN8_MIN << "] "
       << (100.0 * bin_count[BIN8_MIN] / functional_wires.size())
       << "% " << bin_count[BIN8_MIN] << endl;
       */

  // Write output in NTL format.
  for (const auto& node_pair : functional_nodes) {
    parser.PrintFunctionalNodeXNtlFormat(
        node_pair.second, &functional_edges, &functional_wires,
        &functional_nodes, output_stream);
    output_stream << endl;
  }

  if (out_file.is_open()) {
    out_file.close();
  }

  return 0;
}



