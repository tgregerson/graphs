#include "structural_netlist_parser.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <fstream>
#include <initializer_list>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include "connection_descriptor.h"
#include "functional_edge.h"
#include "functional_node.h"
#include "functional_node_factory.h"
#include "structural_netlist_lexer.h"
#include "universal_macros.h"

using namespace std;

list<string> StructuralNetlistParser::RawLinesToSemicolonTerminated(
    const list<string>& raw_lines) {
  string current;
  list<string> parsed;
  for (const string& raw_line : raw_lines) {
    string line = raw_line;

    // Strip comments
    if (line.find("//") != string::npos) {
      line = line.substr(0, line.find("//"));
    }
    // TODO /* */ format comments
    assert(line.find("/*") == string::npos);
    
    // Strip Xilinx annotations in the (* annotation *) format.
    while (line.find("(*") != string::npos) {
      size_t start = line.find("(*");
      size_t end = line.find("*)");
      string front = line.substr(0, start);
      string back = line.substr(end + 2, string::npos); 
      line = front + back;
    }

    if (line.find("endmodule") != string::npos) {
      // endmodule does not have a semi-colon. For now, just hope that no one
      // tries to start the next line on the same line as the endmodule.
      current.clear();
    } else {
      size_t sc_pos = line.find(';');
      if (sc_pos == string::npos) {
        // Non-terminating line. Append.
        current.append(line);
      } else if (sc_pos == line.length() - 1) {
        // Easy case: Semi-colon at end of line.
        current.append(line);
        parsed.push_back(trimLeadingTrailingWhiteSpace(current));
        current.clear();
      } else {
        // Harder case: There are characters on the line after the semi-colon.
        // May need to include them as the beginning of the next line.
        
        // First take the semi-colon terminated substring and finish the current
        // string.
        current.append(line.substr(0, sc_pos + 1));
        parsed.push_back(trimLeadingTrailingWhiteSpace(current));

        // Get the remainder.
        current = line.substr(sc_pos + 1, string::npos);
        
        // If the remainder contains another semi-colon, we give up to avoid
        // recursion. TODO: Add support.
        assert_b(current.find(';') == string::npos) {
          cout << "Too many semi-colons in line: " << line << endl;
        }
      }
    } 
  }
  return parsed;
}

string StructuralNetlistParser::StripModuleParameters(
    const std::string& my_str) {
  size_t start_location = my_str.find("#");
  if (start_location == string::npos) {
    return my_str;
  }

  string pre = my_str.substr(0, start_location);
  string post = StructuralNetlistLexer::ConsumeModuleParameters(
      my_str.substr(start_location, string::npos), nullptr);
  return pre + " " + post;
}

string StructuralNetlistParser::trimLeadingTrailingWhiteSpace(
    const string& str) {
  size_t start_idx = 0;
  while (start_idx < str.length() && isspace(str[start_idx])) {
    start_idx++;
  }
  
  if (start_idx == str.length()) {
    return "";
  }

  size_t end_idx = str.length();
  while (isspace(str[end_idx - 1])) { 
    end_idx--;
  }

  size_t length = end_idx - start_idx;
  return str.substr(start_idx, length);
}

vector<string> StructuralNetlistParser::StringTokens(
    const string& str, const string delim) {
  vector<string> tokens;
  boost::char_separator<char> separator(delim.c_str());
  boost::tokenizer<boost::char_separator<char>> tokenizer(str, separator);
  for (const string& token : tokenizer) {
    tokens.push_back(token);
  }
  return tokens;
}

string StructuralNetlistParser::FirstStringToken(const string& str) {
  vector<string> tokens = StringTokens(str);
  assert_b(!tokens.empty()) {
    cout << "Found no token in line: " << str << endl;
  }
  return tokens.front();
}


bool StructuralNetlistParser::StartsWith(
    const string& src, const vector<string>& any_of) {
  string first_token = FirstStringToken(src);
  for (const string& any : any_of) {
    if (any == first_token) return true;
  }
  return false;
}

VlogModule StructuralNetlistParser::VlogModuleFromLine(const string& str) {
  string remaining = str;
  string module_type_name;
  remaining = StructuralNetlistLexer::ConsumeIdentifier(
      remaining, &module_type_name);

  vector<pair<string,string>> module_parameters;
  try {
    string module_parameters_str;
    remaining = StructuralNetlistLexer::ConsumeModuleParameters(
        remaining, &module_parameters_str);
    module_parameters =
        StructuralNetlistLexer::ExtractParameterConnectionsFromModuleParameters(
            module_parameters_str);
    cout << "SUCCESSFULLY PARSED:\n" << module_parameters_str << "\n";
    for (const auto& parameter : module_parameters) {
      cout << parameter.first << " : " << parameter.second << "\n";
    }
  } catch (StructuralNetlistLexer::LexingException& e) {
    // Parameter list is optional, so do nothing if not parsable.
  }

  string module_instance_name;
  remaining = StructuralNetlistLexer::ConsumeIdentifier(
      remaining, &module_instance_name);

  string wrapped_connection_list;
  remaining = StructuralNetlistLexer::ConsumeWrappedElement(
      remaining, &wrapped_connection_list,
      &StructuralNetlistLexer::ConsumeConnectionList, '(', ')');
  // Remove wrapping ()
  string connection_list = StructuralNetlistLexer::ExtractInner(
      wrapped_connection_list);

  remaining = StructuralNetlistLexer::ConsumeChar(remaining, nullptr, ';');
  if (!remaining.empty()) {
    const string error_msg = "Failed to parse module from line: " + str + "\n";
    throw StructuralNetlistLexer::LexingException(error_msg);
  }

  vector<pair<string,string>> connections =
    StructuralNetlistLexer::ExtractConnectionsFromConnectionList(
        connection_list);

  VlogModule module(module_type_name, module_instance_name);
  for (const pair<string,string>& connection : connections) {
    // The connected element could be a constant value. Don't include those.
    if (!connection.second.empty() && !isdigit(connection.second[0])) {
      module.connected_net_names.insert(connection.second);
    }
    assert(!connection.first.empty());
    if (!connection.first.empty()) {
      module.named_connections.insert(connection);
    }
    module.ordered_connections.push_back(connection.second);
  }

  for (const auto& parameter : module_parameters) {
    if (!parameter.first.empty()) {
      module.named_parameters.insert(parameter);
    }
    module.ordered_parameters.push_back(parameter.second);
  }
  return module;
}

FunctionalNode* StructuralNetlistParser::FunctionalNodeFromLine(
    const map<string, FunctionalEdge*>& edges,
    const map<string, FunctionalEdge*>& wires,
    const string& str) {
  string remaining = str;
  string module_type_name;
  remaining = StructuralNetlistLexer::ConsumeIdentifier(
      remaining, &module_type_name);

  vector<pair<string,string>> module_parameters;
  try {
    string module_parameters_str;
    remaining = StructuralNetlistLexer::ConsumeModuleParameters(
        remaining, &module_parameters_str);
    module_parameters =
        StructuralNetlistLexer::ExtractParameterConnectionsFromModuleParameters(
            module_parameters_str);
  } catch (StructuralNetlistLexer::LexingException& e) {
    // Parameter list is optional, so do nothing if not parsable.
  }

  string module_instance_name;
  remaining = StructuralNetlistLexer::ConsumeIdentifier(
      remaining, &module_instance_name);

  string wrapped_connection_list;
  remaining = StructuralNetlistLexer::ConsumeWrappedElement(
      remaining, &wrapped_connection_list,
      &StructuralNetlistLexer::ConsumeConnectionList, '(', ')');
  // Remove wrapping ()
  string connection_list = StructuralNetlistLexer::ExtractInner(
      wrapped_connection_list);

  remaining = StructuralNetlistLexer::ConsumeChar(remaining, nullptr, ';');
  if (!remaining.empty()) {
    const string error_msg = "Failed to parse module from line: " + str + "\n";
    throw StructuralNetlistLexer::LexingException(error_msg);
  }

  vector<pair<string,string>> connections =
      StructuralNetlistLexer::ExtractConnectionsFromConnectionList(
          connection_list);

  map<string, vector<string>> port_connection_map;
  for (const pair<string, string>& connection : connections) {
    // NOTE: This process reverses the connection order for the port.
    // Since Verilog uses descending ordering for concatenated nets,
    // this reversal puts the connections in ascending order, matching
    // the index ordering of the vector. This will be important when
    // calling AddBitConnections later.
    port_connection_map[connection.first].push_back(connection.second);
  }

  FunctionalNode* node =
      FunctionalNodeFactory::CreateFunctionalNode(module_type_name);
  node->instance_name = module_instance_name;
  for (const pair<string, vector<string>>& port_connections : port_connection_map) {
    assert(!port_connections.first.empty());
    ConnectionDescriptor desc(port_connections.first);

    for (const string& element : port_connections.second) {
      auto edge_it = edges.find(element);
      if (edge_it != edges.end()) {
        // Full edge connection without indexing.
        vector<string> single_bits = edge_it->second->GetBitNames();
        for (const string& single_bit : single_bits) {
          assert(wires.find(single_bit) != wires.end());
          desc.AddBitConnection(single_bit);
        }
      } else {
        auto wire_it = wires.find(element);
        if (wire_it != wires.end()) {
          // Single-bit wire connection.
          desc.AddBitConnection(wire_it->second->IndexedName());
        } else {
          // Could also be a ranged portion of a bus.
          string base_name;
          string remainder =
              StructuralNetlistLexer::ConsumeIdentifier(element, &base_name);
          string range;
          remainder =
              StructuralNetlistLexer::ConsumeBitRange(remainder, &range);
          assert(remainder.empty());
          pair<int, int> bit_range =
              StructuralNetlistLexer::ExtractBitRange(range);
          FunctionalEdge* edge = edges.at(base_name);
          const vector<string> single_bits = edge->GetBitNames();
          for (int bit = bit_range.second; bit <= bit_range.first; ++bit) {
            // Some edges are defined with non-zero starting bits.
            int index = bit - edge->BitLow();
            assert(wires.find(single_bits.at(index)) != wires.end());
            desc.AddBitConnection(single_bits[index]);
          }
        }
      }
    }
    node->AddConnection(desc);
  }

  for (const auto& parameter : module_parameters) {
    if (!parameter.first.empty()) {
      node->named_parameters.insert(parameter);
    }
  }
  return node;
}

vector<VlogNet> StructuralNetlistParser::VlogNetsFromLine(const string& str) {
  string remaining = str;

  string net_type_name;
  remaining = StructuralNetlistLexer::ConsumeSimpleIdentifier(
      remaining, &net_type_name);
  assert (net_type_name == "input" || net_type_name == "output" ||
          net_type_name == "wire"  || net_type_name == "reg");
  if (net_type_name == "input" || net_type_name == "output") {
    // Check for compound types "input wire", "output reg".
    try {
      string next_token;
      string token_removed = StructuralNetlistLexer::ConsumeSimpleIdentifier(
          remaining, &next_token);
      if (next_token == "wire" || next_token == "reg") {
        net_type_name.append(" " + next_token);
        remaining = token_removed;
      }
    } catch (StructuralNetlistLexer::LexingException& e) {
      // Compound type is optional, so OK to ignore parsing failure.
    }
  }

  pair <int, int> range_ints = {0, 0};
  try {
    string range;
    remaining = StructuralNetlistLexer::ConsumeBitRange(remaining, &range);
    range_ints = StructuralNetlistLexer::ExtractBitRange(range);
  } catch (StructuralNetlistLexer::LexingException& e) {
    // Bit range is optional, so OK to ignore parsing failure.
  }

  string identifier_list;
  remaining = StructuralNetlistLexer::ConsumeIdentifierList(
      remaining, &identifier_list);
  vector<string> identifiers =
      StructuralNetlistLexer::ExtractIdentifiersFromIdentifierList(
          identifier_list);

  remaining = StructuralNetlistLexer::ConsumeChar(remaining, nullptr, ';');
  if (!remaining.empty()) {
    const string error_msg = "Failed to parse nets from line: " + str + "\n";
    throw StructuralNetlistLexer::LexingException(error_msg);
  }

  vector<VlogNet> nets;
  for (const string& net_identifier : identifiers) {
    VlogNet net(net_identifier, range_ints.first - range_ints.second + 1,
                range_ints.first, range_ints.second);
    nets.push_back(net);
  }
  return nets;
}

vector<FunctionalEdge*> StructuralNetlistParser::FunctionalEdgesFromLine(
    const string& str) {
  string remaining = str;

  string net_type_name;
  remaining = StructuralNetlistLexer::ConsumeSimpleIdentifier(
      remaining, &net_type_name);
  assert (net_type_name == "input" || net_type_name == "output" ||
          net_type_name == "wire"  || net_type_name == "reg");
  if (net_type_name == "input" || net_type_name == "output") {
    // Check for compound types "input wire", "output reg".
    try {
      string next_token;
      string token_removed = StructuralNetlistLexer::ConsumeSimpleIdentifier(
          remaining, &next_token);
      if (next_token == "wire" || next_token == "reg") {
        net_type_name.append(" " + next_token);
        remaining = token_removed;
      }
    } catch (StructuralNetlistLexer::LexingException& e) {
      // Compound type is optional, so OK to ignore parsing failure.
    }
  }

  pair <int, int> range_ints = {0, 0};
  try {
    string range;
    remaining = StructuralNetlistLexer::ConsumeBitRange(remaining, &range);
    range_ints = StructuralNetlistLexer::ExtractBitRange(range);
  } catch (StructuralNetlistLexer::LexingException& e) {
    // Bit range is optional, so OK to ignore parsing failure.
  }

  string identifier_list;
  remaining = StructuralNetlistLexer::ConsumeIdentifierList(
      remaining, &identifier_list);
  vector<string> identifiers =
      StructuralNetlistLexer::ExtractIdentifiersFromIdentifierList(
          identifier_list);

  remaining = StructuralNetlistLexer::ConsumeChar(remaining, nullptr, ';');
  if (!remaining.empty()) {
    const string error_msg = "Failed to parse nets from line: " + str + "\n";
    throw StructuralNetlistLexer::LexingException(error_msg);
  }

  vector<FunctionalEdge*> edges;
  for (const string& net_identifier : identifiers) {
    FunctionalEdge* edge = new FunctionalEdge(
        net_identifier, range_ints.first, range_ints.second);
    edges.push_back(edge);
  }
  return edges;
}

void StructuralNetlistParser::PopulateModules(
    map<string, VlogModule>* modules, const list<string>& lines) {
  const vector<string> non_module_keywords {
    "module",
    "endmodule",
    "input",
    "output",
    "wire",
    "reg",
  };
  int lines_processed = 0;
  for (const string& line : lines) {
    if (!StartsWith(line, non_module_keywords)) {
      VlogModule module = VlogModuleFromLine(line);
      modules->insert(make_pair(module.instance_name, module));
    }
    if (lines_processed % 1000 == 0 && lines_processed != 0) {
      cout << "Processed " << lines_processed << " module lines." << endl;
    }
    lines_processed++;
  }
}

void StructuralNetlistParser::PopulateFunctionalNodes(
    const map<string, FunctionalEdge*>& edges,
    const map<string, FunctionalEdge*>& wires,
    map<string, FunctionalNode*>* nodes,
    const list<string>& lines) {
  const vector<string> non_module_keywords {
    "module",
    "endmodule",
    "input",
    "output",
    "wire",
    "reg",
  };
  int lines_processed = 0;
  for (const string& line : lines) {
    if (!StartsWith(line, non_module_keywords)) {
      FunctionalNode* node = FunctionalNodeFromLine(edges, wires, line);
      nodes->insert(make_pair(node->instance_name, node));
    }
    if (lines_processed % 1000 == 0 && lines_processed != 0) {
      cout << "Processed " << lines_processed << " module lines." << endl;
    }
    lines_processed++;
  }
}

void StructuralNetlistParser::PopulateNets(
    map<string, VlogNet>* nets, const list<string>& lines) {
  const vector<string> net_keywords {
    "input",
    "output",
    "wire",
    "reg",
  };
  int lines_processed = 0;
  for (const string& line : lines) {
    if (StartsWith(line, net_keywords)) {
      for (const VlogNet& net : VlogNetsFromLine(line)) {
        nets->insert(make_pair(net.name, net));
      }
    }
    if (lines_processed % 1000 == 0 && lines_processed != 0) {
      cout << "Processed " << lines_processed << " net lines." << endl;
    }
    lines_processed++;
  }
}

void StructuralNetlistParser::PopulateFunctionalEdges(
    map<string, FunctionalEdge*>* edges, const list<string>& lines) {
  const vector<string> net_keywords {
    "input",
    "output",
    "wire",
    "reg",
  };
  int lines_processed = 0;
  for (const string& line : lines) {
    if (StartsWith(line, net_keywords)) {
      for (FunctionalEdge* edge : FunctionalEdgesFromLine(line)) {
        edges->insert(make_pair(edge->BaseName(), edge));
      }
    }
    if (lines_processed % 1000 == 0 && lines_processed != 0) {
      cout << "Processed " << lines_processed << " net lines." << endl;
    }
    lines_processed++;
  }
}

void StructuralNetlistParser::PopulateFunctionalEdgePorts(
      const map<string, FunctionalNode*>& nodes,
      map<string, FunctionalEdge*>* edges,
      map<string, FunctionalEdge*>* wires) {
  for (auto node_pair : nodes) {
    FunctionalNode* node = node_pair.second;
    for (const auto& connection : node->named_input_connections) {
      const vector<string>& connected_bits = connection.second.connection_bit_names;
      for (unsigned int bit = 0; bit < connected_bits.size(); ++bit) {
        FunctionalEdge* connected_wire = wires->at(connected_bits[bit]);
        FunctionalEdge* connected_edge = edges->at(connected_wire->BaseName());
        connected_wire->AddSinkPort(
            FunctionalEdge::NodePortDescriptor(
                node_pair.first, connection.first, bit, bit));
        connected_edge->AddSinkPort(
            FunctionalEdge::NodePortDescriptor(
                node_pair.first, connection.first, bit, bit));
      }
    }
    for (const auto& connection : node->named_output_connections) {
      const vector<string>& connected_bits = connection.second.connection_bit_names;
      for (unsigned int bit = 0; bit < connected_bits.size(); ++bit) {
        FunctionalEdge* connected_wire = wires->at(connected_bits[bit]);
        FunctionalEdge* connected_edge = edges->at(connected_wire->BaseName());
        connected_wire->AddSourcePort(
            FunctionalEdge::NodePortDescriptor(
                node_pair.first, connection.first, bit, bit));
        connected_edge->AddSourcePort(
            FunctionalEdge::NodePortDescriptor(
                node_pair.first, connection.first, bit, bit));
      }
    }
  }
}

void StructuralNetlistParser::PrePopulateProbabilityOnes(
    ifstream& p1_file, map<string, FunctionalEdge*>* wires) {
  string cur_line;
  string wire_name;
  string wire_bit_select;
  double wire_p1;

  unsigned long long wires_processed = 0ULL;
  unsigned long long errors_found = 0ULL;

  getline(p1_file, cur_line);
  do {
    string remainder =
        StructuralNetlistLexer::ConsumeIdentifier(cur_line, &wire_name);
    remainder =
        StructuralNetlistLexer::ConsumeBitRange(remainder, &wire_bit_select);
    wire_name.append(wire_bit_select);
    string number_dec;
    unsigned long long num_0, num_1, num_x, num_z;
    try {
      remainder =
          StructuralNetlistLexer::ConsumeUnbasedImmediate(remainder, &number_dec);
      num_0 = strtoull(number_dec.c_str(), nullptr, 10);
      remainder =
          StructuralNetlistLexer::ConsumeUnbasedImmediate(remainder, &number_dec);
      num_1 = strtoull(number_dec.c_str(), nullptr, 10);
      remainder =
          StructuralNetlistLexer::ConsumeUnbasedImmediate(remainder, &number_dec);
      num_x = strtoull(number_dec.c_str(), nullptr, 10);
      remainder =
          StructuralNetlistLexer::ConsumeUnbasedImmediate(remainder, &number_dec);
      num_z = strtoull(number_dec.c_str(), nullptr, 10);
    } catch (StructuralNetlistLexer::LexingException& e) {
      string error_msg =
          "PrePopulateProbabilityOnes: Failed to consume entropy line: " +
          cur_line +".\nReceived exception: " + string(e.what());
      throw StructuralNetlistLexer::LexingException(error_msg);
    }
    unsigned long long total = num_0 + num_1 + num_x + num_z;
    unsigned long long adj_num_1 =
        ((double)num_1 > (1.1 * (double)num_0)) ? num_1 + num_x + num_z : num_1;
    wire_p1 = (double)adj_num_1 / (double)total;

    try {
      FunctionalEdge* wire = wires->at(wire_name);
      wire->SetProbabilityOne(wire_p1);
      ++wires_processed;
    } catch (std::exception& e) {
      /*
      for (auto& wire_pair : *wires) {
        cout << wire_pair.first << endl;
      }
      */
      ++errors_found;
      //cout << "Failed to find wire: " << wire_name << endl;
      //cout << e.what() << endl;
      //throw std::exception();
    }
    getline(p1_file, cur_line);
  } while (!p1_file.eof());
  cout << "Successfully set entropy for " << wires_processed << " of "
       << wires->size() << " wires in netlist. Encountered "
       << errors_found << " unrecognized wire names.\n";
}

void StructuralNetlistParser::PrintModule(const VlogModule& module) {
  cout << "Module\n";
  cout << "Type: " << module.type_name << "\n";
  cout << "Instance: " << module.instance_name << "\n";
  cout << "Connections\n";
  for (const string& cn : module.connected_net_names) {
    cout << cn << "\n";
  }
  cout << "\n";
}

void StructuralNetlistParser::PrintModuleNtlFormat(
    const VlogModule& module,
    const map<string, VlogNet>& nets,
    ostream& output_stream) {
  output_stream << "module_begin" << "\n";
  output_stream << module.type_name << "\n";
  output_stream << module.instance_name << "\n";
  for (const string& cn : module.connected_net_names) {
    if (nets.find(cn) == nets.end()) {
      cout << "WARNING: Couldn't find connection '" << cn
           << "' in list of nets.\n";
    } else {
      auto net_it = nets.find(cn);
      output_stream << cn << " (" << net_it->second.width << ")\n";
    }
  }
  output_stream << "module_end" << "\n";
}

void StructuralNetlistParser::PrintFunctionalNodeXNtlFormat(
    FunctionalNode* node,
    map<string, FunctionalEdge*>* edges,
    map<string, FunctionalEdge*>* wires,
    map<string, FunctionalNode*>* nodes,
    ostream& output_stream) {
  output_stream << "module_begin" << "\n";
  output_stream /*<< "TYPE "*/ << node->type_name << "\n";
  output_stream /*<< "NAME "*/ << node->instance_name << "\n";
  /*
  if (node->named_parameters.size() > 0) {
    output_stream << "PARAMETERS:\n";
    for (const auto& param : node->named_parameters) {
      output_stream << param.first << ": " << param.second << "\n";
    }
    output_stream << "------------\n";
  }
  */
  for (const string& wire_name : node->AllConnectedWires()) {
    output_stream << wire_name << "\n"
                  << wires->at(wire_name)->Entropy(wires, nodes) << "\n";
  }
  /*
  for (auto& conn : node->named_output_connections) {
    for (const string& wire_name : conn.second.connection_bit_names) {
      if (!wire_name.empty()) {
        output_stream << wire_name << " "
                      << wires->at(wire_name)->Entropy(wires, nodes) << "\n";
      }
    }
  }
  for (auto& conn : node->named_input_connections) {
    for (const string& wire_name : conn.second.connection_bit_names) {
      if (!wire_name.empty()) {
        output_stream << wire_name << " "
                      << wires->at(wire_name)->Entropy(wires, nodes) << "\n";
      }
    }
  }
  */
  output_stream << "module_end" << "\n";
}

void StructuralNetlistParser::PrintNet(const VlogNet& net) {
  cout << "Net" << "\n";
  cout << "Name: " << net.name << "\n";
  cout << "Width: " << net.width << "\n";
  cout << "Range: [" << net.bit_hi << ":" << net.bit_lo << "]" << "\n";
  cout << "\n";
}

set<string> StructuralNetlistParser::GetBusNames(
    const map<string, VlogNet>& net_container) {
  set<string> names;
  for (const auto& net_pair : net_container) {
    if (net_pair.second.width > 1) {
      names.insert(net_pair.second.name);
    }
  }
  return names;
}

set<string> StructuralNetlistParser::GetBusNames(
    const map<string, FunctionalEdge*>& edges) {
  set<string> names;
  for (const auto& edge_pair : edges) {
    if (edge_pair.second->Width() > 1) {
      names.insert(edge_pair.second->BaseName());
    }
  }
  return names;
}

void StructuralNetlistParser::ExpandModuleBusConnections(
    const map<string, VlogNet>& nets,
    map<string, VlogModule>* module_container) {

  // Find implicitly ranged connections (connections that use a full bus
  // with just the name rather than a range) and attach the bit range to them.
  set<string> bus_names = GetBusNames(nets);
  for (auto& module_pair_ref : *module_container) {
    // To avoid issues with invaliding iterators in the connection list, just
    // make a copy of the updated version.
    VlogModule& module_ref = module_pair_ref.second;
    auto new_connections = module_ref.connected_net_names;
    for (const string& connection_name : module_ref.connected_net_names) {
      if (bus_names.find(connection_name) != bus_names.end()) {
        string ranged_bus = AddBusRange(nets.at(connection_name));
        new_connections.erase(connection_name);
        new_connections.insert(ranged_bus);
      }
    }
    // Replace connection list in module with expanded list.
    module_ref.connected_net_names = new_connections;
  }

  // Process connections that include ranges, and split them into single-bit
  // connection names.
  for (auto& module_pair_ref : *module_container) {
    // To avoid issues with invaliding iterators in the connection list, just
    // make a copy of the updated version.
    VlogModule& module_ref = module_pair_ref.second;
    set<string> new_connections;
    for (const string& connection_name : module_ref.connected_net_names) {
      set<string> split_connections = SplitConnectionByRange(connection_name);
      for (const string& split_cl : split_connections) {
        new_connections.insert(split_cl);
      }
    }
    // Replace connection list in module with expanded list.
    module_ref.connected_net_names = new_connections;
  }
}

void StructuralNetlistParser::ExpandBusConnections(
    const map<string, VlogNet>& nets, NamedConnectionMultiMap* connections) {
  // Find implicitly ranged connections (connections that use a full bus
  // with just the name rather than a range) and attach the bit range to them.
  set<string> bus_names = GetBusNames(nets);
  NamedConnectionMultiMap new_connections;
  for (const NamedConnection& connection : *connections) {
    if (bus_names.find(connection.second) != bus_names.end()) {
      string ranged_bus = AddBusRange(nets.at(connection.second));
      new_connections.insert(make_pair(connection.first, ranged_bus));
    } else {
      new_connections.insert(connection);
    }
  }
  std::swap(*connections, new_connections);
  new_connections.clear();

  // Process connections that include ranges, and split them into single-bit
  // connection names.
  for (const NamedConnection& connection : *connections) {
    set<string> split_connections = SplitConnectionByRange(connection.second);
    for (const string& split_cl : split_connections) {
      new_connections.insert(make_pair(connection.first, split_cl));
    }
  }
  std::swap(*connections, new_connections);
}

std::string StructuralNetlistParser::AddBusRange(const VlogNet& net) {
  // Check bit_hi here because we also want to add ranges for single-bit
  // edges that don't index from zero.
  assert_b(net.width > 1 || net.bit_hi > 0) {
    cout << "Tried to add bus range for connection: "
         << net.name << ", but width is: " << net.width;
  }
  ostringstream ranged;
  if (net.width > 1) {
    ranged << net.name << "[" << net.bit_hi << ":" << net.bit_lo << "]";
  } else {
    ranged << net.name << "[" << net.bit_hi << "]";
  }
  return ranged.str();
}

bool StructuralNetlistParser::ParseRangeIfValid(
    const string& str, pair<int, int>* range) {
  assert(range != nullptr);
  size_t colon_pos = str.find(":");
  if (colon_pos == string::npos) {
    return false;
  } else {
    string first_part = str.substr(0, colon_pos);
    string second_part = str.substr(colon_pos + 1, string::npos);
    int a, b;
    try {
      a = boost::lexical_cast<int>(first_part);
      b = boost::lexical_cast<int>(second_part);
    } catch (boost::bad_lexical_cast const&) {
      return false;
    }
    if (a > b) {
      range->first = a;
      range->second = b;
    } else {
      range->first = b;
      range->second = a;
    }
    return true;
  }
}

set<string> StructuralNetlistParser::SplitConnectionByRange(
    const string& connection, size_t start_pos) {
  size_t open_brack = connection.find("[", start_pos);
  if (open_brack == string::npos) {
    // No range. Return original string.
    return set<string>({connection});
  }
  set<string> split_connections;
  size_t close_brack = FindClosingBracketOrDie(connection, open_brack + 1);
  pair<int, int> range_ints;

  // Slice the connection string into three parts:
  //   prefix: string up-to, and including, the opening bracket.
  //   range: the contents of the brackets
  //   postfix: string starting from the closing bracket
  string prefix = connection.substr(0, open_brack + 1);
  string range = connection.substr(open_brack + 1,
                                   close_brack - open_brack - 1);
  string postfix = connection.substr(close_brack, string::npos);

  set<string> intermediate_connections;

  // Check is the thing in the brackets is actually a parsable range.
  if (ParseRangeIfValid(range, &range_ints)) {
    // Create split connections with a single bit from the range. Because there
    // may be further ranges in the connection, we call these intermediates.
    for (int bit_num = range_ints.first; bit_num >= range_ints.second;
         --bit_num) {
      ostringstream intermediate_connection;
      intermediate_connection << prefix << bit_num << postfix;
      intermediate_connections.insert(intermediate_connection.str());
    }
  } else {
    // The current bracket group is not a valid range. See if there are further
    // valid ranges later in the string.
    intermediate_connections.insert(connection);
  }

  for (const string& intermediate_connection : intermediate_connections) {
    // Update the new position of close_brack if the intermediate has
    // been altered.
    close_brack = FindClosingBracketOrDie(intermediate_connection,
                                          open_brack + 1);
    for (const string& split_connection :
         SplitConnectionByRange(intermediate_connection, close_brack + 1)) {
      split_connections.insert(split_connection);
    }
  }
  return split_connections;
}

std::string StructuralNetlistParser::CleanIdentifier(const std::string& str) {
  size_t start_char_pos = 0;
  while (start_char_pos < str.length() && isspace(str[start_char_pos])) {}
  if (start_char_pos >= str.length()) {
    return "";
  }
  size_t end_char_pos = start_char_pos;
  while (end_char_pos < str.length() && !isspace(str[end_char_pos])) {}

  // If escaped identifier, we keep the first trailing whitespace char.
  if (str[start_char_pos] != '\\') {
    --end_char_pos;
  }

  size_t length = end_char_pos - start_char_pos + 1;
  return str.substr(start_char_pos, length);
}

// Helper fn to return the position of the first closing brack after the
// start position or dying if none is found.
size_t StructuralNetlistParser::FindClosingBracketOrDie(
    const string& str, size_t start_pos) {
  size_t close_brack = str.find("]", start_pos);
  assert_b(close_brack != string::npos) {
    cout << "Couldn't find matching closing bracket in connection: "
         << str << endl;
  }
  return close_brack;
}

void StructuralNetlistParser::RemoveNetConnectionFromModules(
    map<string, VlogModule>* module_container,
    const set<string>& nets_to_remove) {
  for (auto& module_pair_ref : *module_container) {
    VlogModule& module_ref = module_pair_ref.second;
    for (const string& net_to_remove : nets_to_remove) {
      auto net_iter = module_ref.connected_net_names.find(net_to_remove);
      if (net_iter != module_ref.connected_net_names.end()) {
        module_ref.connected_net_names.erase(net_iter);
      }
    }
  }
}

void StructuralNetlistParser::RemoveWires(
    map<string, FunctionalNode*>* nodes,
    map<string, FunctionalEdge*>* wires,
    const set<string>& wires_to_remove) {
  for (const string& wire_name : wires_to_remove) {
    auto wire_it = wires->find(wire_name);
    if (wire_it != wires->end()) {
      FunctionalEdge* wire = wire_it->second;
      for (const auto& desc : wire->SourcePorts()) {
        ConnectionDescriptor& conn_desc =
            nodes->at(desc.node_instance_name)->GetConnectionDescriptor(
                desc.node_port_name);
        for (int i = desc.bit_low; i <= desc.bit_high; ++i) {
          conn_desc.RemoveBitConnection(i, wire_name);
        }
      }
      for (const auto& desc : wire->SinkPorts()) {
        ConnectionDescriptor& conn_desc =
            nodes->at(desc.node_instance_name)->GetConnectionDescriptor(
                desc.node_port_name);
        for (int i = desc.bit_low; i <= desc.bit_high; ++i) {
          conn_desc.RemoveBitConnection(i, wire_name);
        }
      }
      wires->erase(wire_it);
    }
  }
}

void StructuralNetlistParser::RemoveWiresBySubstring(
    map<string, FunctionalNode*>* nodes,
    map<string, FunctionalEdge*>* wires,
    const set<string>& substrings) {
  set<string> wires_to_remove;
  for (auto wire_pair : *wires) {
    const string& wire_name = wire_pair.first;
    for (const string& substring : substrings) {
      if (wire_name.find(substring) != string::npos) {
        wires_to_remove.insert(wire_name);
        break;
      }
    }
  }
  RemoveWires(nodes, wires, wires_to_remove);
}

void StructuralNetlistParser::RemoveNetConnectionFromModulesSubstring(
    map<string, VlogModule>* module_container,
    const set<string>& nets_to_remove_substring) {
  for (auto& module_pair_ref : *module_container) {
    VlogModule& module_ref = module_pair_ref.second;
    // Make a copy to avoid invaliding iterators.
    set<string> updated_connections = module_ref.connected_net_names;
    for (const string& connection : module_ref.connected_net_names) {
      for (const string& ss : nets_to_remove_substring) {
        if (connection.find(ss) != string::npos) {
          updated_connections.erase(connection);
          break;
        }
      }
    }
    module_ref.connected_net_names = updated_connections;
  }
}

void StructuralNetlistParser::RemoveUnconnectedNodes(
    map<string, FunctionalNode*>* nodes) {
  auto it = nodes->begin();
  while (it != nodes->end()) {
    auto old_it = it;
    ++it;
    if (old_it->second->AllConnectedWires().empty()) {
      nodes->erase(old_it);
    }
  }
}
