#include <cassert>

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include "structural_netlist_parser.h"

using namespace std;

// Prints command-line invokation format and exit.
void PrintHelpAndDie() {
  cout << "Usage: ./structural_netlist_parser netlist_file [parsed_netlist]"
       << endl;
  exit(1);
}

int main(int argc, char *argv[]) {
  map<string, VlogModule> modules;
  map<string, VlogNet> nets;

  StructuralNetlistParser parser;

  ifstream v_file;
  ofstream out_file;
  if (argc != 2 && argc != 3) {
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
    if (argc == 3) {
      string file_to_write = argv[2];   
      out_file.open(file_to_write.c_str());
      if (!out_file.is_open()) {
        cout << "Can't open output file " << file_to_write << endl;
        exit(1);
      }
    }
  }

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
  parsed_lines = parser.RawLinesToSemicolonTerminated(parsed_lines);

  parser.PopulateModules(&modules, parsed_lines);
  parser.PopulateNets(&nets, parsed_lines);

  // Remove synthesis-tool-specific nets that should not be included in graph.
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

  set<string> blacklisted_net_substrings = {
    "UNCONNECTED",
  };
  parser.RemoveNetConnectionFromModulesSubstring(
      &modules, blacklisted_net_substrings);

  // Add expanding of subranges.
  parser.ExpandModuleBusConnections(nets, &modules);

  // Write output in NTL format.
  for (const auto& module_pair : modules) {
    parser.PrintModuleNtlFormat(module_pair.second, nets, output_stream);
    output_stream << endl;
  }

  if (out_file.is_open()) {
    out_file.close();
  }

  return 0;
}
