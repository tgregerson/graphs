/*
 * functional_netlist_parser_main.cpp
 *
 *  Created on: Sep 9, 2014
 *      Author: gregerso
 */

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include "functional_edge.h"
#include "functional_node.h"
#include "tclap/CmdLine.h"
#include "xilinx_functional_nodes.h"
#include "structural_netlist_parser.h"

using namespace std;

int main(int argc, char *argv[]) {
  StructuralNetlistParser parser;

  TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

  TCLAP::ValueArg<string> verilog_input_file_flag(
      "v", "verilog", "Verilog input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> entropy_input_file_flag(
      "e", "entropy", "Entropy input file name", false, "", "string",
      cmd);

  TCLAP::ValueArg<string> xntl_output_file_flag(
      "x", "xntl", "XNTL output file name", false, "", "string",
      cmd);

  cmd.parse(argc, argv);

  ifstream v_file;
  ifstream p1_file;
  ofstream xntl_file;

  // Open file.
  v_file.open(verilog_input_file_flag.getValue());
  if (!v_file.is_open()) {
    cout << "Can't open input file " << verilog_input_file_flag.getValue()
         << endl;
    exit(1);
  }
  if (entropy_input_file_flag.isSet()) {
    p1_file.open(entropy_input_file_flag.getValue());
    if (!p1_file.is_open()) {
      cout << "Can't open entropy input file "
           << entropy_input_file_flag.getValue() << endl;
      exit(1);
    }
  }
  if (xntl_output_file_flag.isSet()) {
    xntl_file.open(xntl_output_file_flag.getValue());
    if (!xntl_file.is_open()) {
      cout << "Can't open output file "
           << xntl_output_file_flag.getValue() << endl;
      exit(1);
    }
  }

  map<string, FunctionalEdge*> functional_edges;
  map<string, FunctionalEdge*> functional_wires;
  map<string, FunctionalNode*> functional_nodes;

  // If no output file, redirect to cout.
  ostream& output_stream = xntl_file.is_open() ? xntl_file : cout;

  // Parsed lines will contain a line that terminates with a ';'.
  cout << "-----------------Parsing Netlist--------------------\n";
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

  cout << "-----------------Populating Edges--------------------\n";
  parser.PopulateFunctionalEdges(&functional_edges, parsed_lines);

  // Add Xilinx constant edges. We use these in place of connections to
  // ground or VCC.
  const string const0s = "\\<const0> ";
  if (functional_edges.find(const0s) == functional_edges.end()) {
    FunctionalEdge* const0_edge = new FunctionalEdge(const0s, 1);
    functional_edges.insert(make_pair(const0s, const0_edge));
  }
  const string const1s = "\\<const1> ";
  if (functional_edges.find(const1s) == functional_edges.end()) {
    FunctionalEdge* const1_edge = new FunctionalEdge(const1s, 1);
    functional_edges.insert(make_pair(const1s, const1_edge));
  }

  // Convert edges into wires, which are all single-bit and explicitly indexed.
  for (auto f_edge_pair : functional_edges) {
    FunctionalEdge* edge = f_edge_pair.second;
    for (int bit = edge->BitLow(); bit <= edge->BitHigh(); ++bit) {
      FunctionalEdge* wire = new FunctionalEdge(edge->BaseName(), bit, bit);
      functional_wires.insert(make_pair(wire->IndexedName(), wire));
    }
  }
  cout << "-----------------Populating Nodes--------------------\n";
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
    cout << "-----------------Pre-Populating Entropies--------------------\n";
    parser.PrePopulateProbabilityOnes(p1_file, &functional_wires);
  }

  parser.PopulateFunctionalEdgePorts(
      functional_nodes, &functional_edges, &functional_wires);

  cout << "-----------------Computing Entropies--------------------\n";
  for (auto& wire_pair : functional_wires) {
    FunctionalEdge* wire = wire_pair.second;
    wire->Entropy(&functional_wires, &functional_nodes);
  }

  // WARNING: Entropy computation must be done before pruning wires from the
  // graph, otherwise the necessary inputs may not be present when computing
  // the output probability.
  cout << "-----------------Pruning Graph--------------------\n";
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

  if (xntl_file.is_open()) {
    cout << "-----------------Writing XNTL to "
         << xntl_output_file_flag.getValue()
         << "--------------------\n";
    // Write output in XNTL format.
    for (const auto& node_pair : functional_nodes) {
      parser.PrintFunctionalNodeXNtlFormat(
          node_pair.second, &functional_edges, &functional_wires,
          &functional_nodes, output_stream);
      output_stream << endl;
    }
    xntl_file.close();
  }

  return 0;
}



