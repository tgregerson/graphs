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

#include "universal_macros.h"

using namespace std;

void StructuralNetlistParser::PrintHelpAndDie() {
  cout << "Usage: ./structural_netlist_parser netlist_file [parsed_netlist]"
       << endl;
  exit(1);
}

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

string StructuralNetlistParser::StripParameterList(
    const std::string& my_str) {
  size_t start_location = my_str.find("#");
  if (start_location == string::npos) {
    return my_str;
  }

  string pre = my_str.substr(0, start_location);
  string post = ConsumeParameterList(
      my_str.substr(start_location, string::npos), nullptr);
  return pre + " " + post;
}

string StructuralNetlistParser::trimLeadingTrailingWhiteSpace(const string& str) {
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
  remaining = ConsumeIdentifier(remaining, &module_type_name);

  try {
    remaining = ConsumeParameterList(remaining, nullptr);
  } catch (ParsingException& e) {
    // Parameter list is optional, so do nothing if not parsable.
  }

  string module_instance_name;
  remaining = ConsumeIdentifier(remaining, &module_instance_name);

  string wrapped_connection_list;
  remaining = ConsumeWrappedElement(
      remaining, &wrapped_connection_list, &ConsumeConnectionList, '(', ')');
  // Remove wrapping ()
  string connection_list = ExtractInner(wrapped_connection_list);

  remaining = ConsumeChar(remaining, nullptr, ';');
  if (!remaining.empty()) {
    const string error_msg = "Failed to parse module from line: " + str + "\n";
    throw ParsingException(error_msg);
  }

  vector<string> connections =
    ExtractConnectedElementsFromConnectionList(connection_list);

  VlogModule module(module_type_name, module_instance_name);
  for (const string& connected_net : connections) {
    // The connected element could be a constant value. Don't include those.
    if (!connected_net.empty() && !isdigit(connected_net[0])) {
      module.connected_net_names.insert(connected_net);
    }
  }
  return module;
}

vector<VlogNet> StructuralNetlistParser::VlogNetsFromLine(const string& str) {
  string remaining = str;

  string net_type_name;
  remaining = ConsumeSimpleIdentifier(remaining, &net_type_name);
  assert (net_type_name == "input" || net_type_name == "output" ||
          net_type_name == "wire"  || net_type_name == "reg");
  if (net_type_name == "input" || net_type_name == "output") {
    // Check for compound types "input wire", "output reg".
    try {
      string next_token;
      string token_removed = ConsumeSimpleIdentifier(remaining, &next_token);
      if (next_token == "wire" || next_token == "reg") {
        net_type_name.append(" " + next_token);
        remaining = token_removed;
      }
    } catch (ParsingException& e) {
      // Compound type is optional, so OK to ignore parsing failure.
    }
  }

  pair <int, int> range_ints = {0, 0};
  try {
    string range;
    remaining = ConsumeBitRange(remaining, &range);
    range_ints = ExtractBitRange(range);
  } catch (ParsingException& e) {
    // Bit range is optional, so OK to ignore parsing failure.
  }

  string identifier_list;
  remaining = ConsumeIdentifierList(remaining, &identifier_list);
  vector<string> identifiers =
      ExtractIdentifiersFromIdentifierList(identifier_list);

  remaining = ConsumeChar(remaining, nullptr, ';');
  if (!remaining.empty()) {
    const string error_msg = "Failed to parse nets from line: " + str + "\n";
    throw ParsingException(error_msg);
  }

  vector<VlogNet> nets;
  for (const string& net_identifier : identifiers) {
    VlogNet net(net_identifier, range_ints.first - range_ints.second + 1,
                range_ints.first, range_ints.second);
    nets.push_back(net);
  }
  return nets;
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
    cout << "Processed " << lines_processed << " module lines." << endl;
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
    cout << "Processed " << lines_processed << " net lines." << endl;
    lines_processed++;
  }
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
    const VlogModule& module, ostream& output_stream) {
  output_stream << "module_begin" << "\n";
  output_stream << module.type_name << "\n";
  output_stream << module.instance_name << "\n";
  for (const string& cn : module.connected_net_names) {
    output_stream << cn << "\n";
  }
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

std::string StructuralNetlistParser::AddBusRange(const VlogNet& net) {
  assert_b(net.width > 1) {
    cout << "Tried to add bus range for connection: "
         << net.name << ", but width is: " << net.width;
  }
  ostringstream ranged;
  ranged << net.name << "[" << net.bit_hi << ":" << net.bit_lo << "]";
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

std::string CleanIdentifier(const std::string& str) {
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

// Whitespace is a sequential combination of spaces, newlines, and tabs.
string StructuralNetlistParser::ConsumeWhitespaceIfPresent(
    const string& input, string* token) {
  size_t end_pos = 0;
  while (end_pos < input.length() && isspace(input[end_pos])) {
    ++end_pos;
  }
  string my_token = input.substr(0, end_pos);
  string new_input = input.substr(end_pos, string::npos);
  if (token != nullptr) {
    token->assign(my_token);
  }
  return new_input;
}

// Identifier = SimpleIdentifier || EscapedIdentifier
string StructuralNetlistParser::ConsumeIdentifier(
    const string& input, std::string* token) {
  const string error_msg = "Failed to parse Identifier from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  if (!remaining.empty()) {
    char c = remaining[0];
    try {
      if (c == '\\') {
        // EscapedIdentifier
        remaining = ConsumeEscapedIdentifier(remaining, token);
      } else {
        // SimpleIdentifier
        remaining = ConsumeSimpleIdentifier(remaining, token);
      }
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
  } else {
    throw ParsingException(error_msg);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// IdentifierList = Identifier[, Identifier]*
string StructuralNetlistParser::ConsumeIdentifierList(
    const string& input, std::string* token) {
  const string error_msg =
      "Failed to parse IdentifierList from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  if (!remaining.empty()) {
    string incremental_token;
    try {
      remaining = ConsumeIdentifier(remaining, &incremental_token);
      while (!remaining.empty() && remaining[0] == ',') {
        string identifier;
        remaining = ConsumeChar(remaining, nullptr, ',');
        remaining = ConsumeIdentifier(remaining, &identifier);
        incremental_token += ", ";
        incremental_token += identifier;
      }
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
    if (token != nullptr) {
      token->assign(incremental_token);
    }
  } else {
    throw ParsingException(error_msg);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// SimpleIdentifier = (_ || a-z)(_ || a-z || 0-9 || $)*
string StructuralNetlistParser::ConsumeSimpleIdentifier(
    const string& input, std::string* token) {
  const string error_msg =
      "Failed to parse SimpleIdentifier from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  size_t end_pos = 0;
  if (remaining.empty() ||
      !(isalpha(remaining[0]) || remaining[0] == '_')) {
    throw ParsingException(error_msg);
  }
  while (end_pos < remaining.length()) {
    char c = remaining[end_pos];
    if (!(isalnum(c) || c == '_' || c == '$')) {
      break;
    }
    end_pos++;
  }
  string my_token = remaining.substr(0, end_pos);
  string new_input = remaining.substr(end_pos, string::npos);
  if (token != nullptr) {
    token->assign(my_token);
  }
  return ConsumeWhitespaceIfPresent(new_input, nullptr);
}

// EscapedIdentifier = \\.*<single whitespace char>
string StructuralNetlistParser::ConsumeEscapedIdentifier(
    const string& input, std::string* token) {
  const string error_msg =
    "Failed to consume EscapedIdentifier from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  size_t end_pos = 0;
  if (remaining.empty() || !(remaining[0] == '\\')) {
    throw ParsingException(error_msg);
  } else {
    while (end_pos < remaining.length()) {
      if (isspace(remaining[end_pos])) {
        break;
      }
      end_pos++;
    }
    end_pos++; // Increment first because we want to keep the trailing space.
    if (!isspace(remaining[end_pos - 1])) {
      const string additional = "No trailing whitespace.\n";
      throw ParsingException(error_msg + additional);
    }
  }
  string my_token = remaining.substr(0, end_pos);
  string new_input = remaining.substr(end_pos, string::npos);
  if (token != nullptr) {
    token->assign(my_token);
  }
  return ConsumeWhitespaceIfPresent(new_input, nullptr);
}

// Connection = ConnectedElement || .Identifier(ConnectedElement)
string StructuralNetlistParser::ConsumeConnection(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume Connection from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    try {
      char c = remaining[0];
      if (c == '.') {
        // Connection by name.
        string identifier;
        remaining = ConsumeChar(remaining, nullptr, '.');
        remaining = ConsumeIdentifier(remaining, &identifier);
        string wrapped_connected_element;
        remaining = ConsumeWrappedElement(
            remaining, &wrapped_connected_element, &ConsumeConnectedElement,
            '(', ')');
        incremental_token = "." + identifier + wrapped_connected_element;
      } else {
        // Connection by position.
        remaining = ConsumeConnectedElement(remaining, &incremental_token);
      }
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// ConnectedElement = (Identifier[BitRange]) || {ConnectionList} || Immediate
string StructuralNetlistParser::ConsumeConnectedElement(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ConnectedElement from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    try {
      char c = remaining[0];
      if (c == '{') {
        // {ConnectionList}
        string wrapped_connection_list;
        remaining = ConsumeWrappedElement(
            remaining, &wrapped_connection_list, &ConsumeConnectionList,
            '{', '}');
        incremental_token.assign(wrapped_connection_list);
      } else if (isdigit(c) || c == '"') {
        // Immediate value.
        remaining = ConsumeImmediate(remaining, &incremental_token);
      } else {
        // Identifier
        remaining = ConsumeIdentifier(remaining, &incremental_token);
        try {
          string bit_range;
          remaining = ConsumeBitRange(remaining, &bit_range);
          incremental_token.append(bit_range);
        } catch (ParsingException e) {
          // BitRange is optional, so do nothing if not present.
        }
      }
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return remaining;
}

// ParameterList = #(ConnectionList)
string StructuralNetlistParser::ConsumeParameterList(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ParameterList from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.size() < 3) { // Must have at least '#()'
    throw ParsingException(error_msg);
  } else {
    try {
      remaining = ConsumeChar(remaining, &incremental_token, '#');
      string wrapped_connection_list;
      remaining = ConsumeWrappedElement(
          remaining, &wrapped_connection_list, &ConsumeConnectionList, '(',
          ')');
      incremental_token.append(wrapped_connection_list);
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return remaining;
}

// Immediate = BinaryImmediate || DecimalImmediate || HexImmediate ||
//             OctalImmediate || UnbasedImmediate || StringLiteral
string StructuralNetlistParser::ConsumeImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume Immediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    try {
      char c = remaining[0];
      if (c == '"') {
        remaining = ConsumeStringLiteral(remaining, &incremental_token);
      } else if (isdigit(c)) {
        remaining = ConsumeUnbasedImmediate(remaining, &incremental_token);
        if (!remaining.empty() && remaining[0] == '\'') {
          assert(remaining.size() > 1);
          char base = remaining[1];

          // Put the unbased immediate back on the front of the string.
          remaining = incremental_token + remaining;
          switch (base) {
            case 'b':
              remaining = ConsumeBinaryImmediate(remaining, &incremental_token);
              break;
            case 'o':
              remaining = ConsumeOctalImmediate(remaining, &incremental_token);
              break;
            case 'd':
              remaining = ConsumeDecimalImmediate(remaining, &incremental_token);
              break;
            case 'h':
              remaining = ConsumeHexImmediate(remaining, &incremental_token);
              break;
            default:
              throw ParsingException("");
          }
        }
      } else {
        throw ParsingException("");
      }
    } catch (ParsingException& e) {
      throw (e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// BinaryImmediate = (UnbasedImmedate)'b(0-1)+
string StructuralNetlistParser::ConsumeBinaryImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume BinaryImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' || 
             remaining[1] != 'b') {
      throw ParsingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '1')) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw ParsingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// OctalImmediate = (UnbasedImmediate)'o(0-7)+
string StructuralNetlistParser::ConsumeOctalImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume OctalImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' || 
             remaining[1] != 'o') {
      throw ParsingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '7')) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw ParsingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// DecimalImmediate = (UnbasedImmediate)'d(0-9)+
string StructuralNetlistParser::ConsumeDecimalImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume DecimalImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' || 
             remaining[1] != 'd') {
      throw ParsingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '9')) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw ParsingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// HexImmediate = (UnbasedImmediate)'h(0-9 || a-f || A-F)+
string StructuralNetlistParser::ConsumeHexImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume HexImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' || 
             remaining[1] != 'h') {
      throw ParsingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           ((remaining[end_pos] >= '0' && remaining[end_pos] <= '9') ||
            (remaining[end_pos] >= 'a' && remaining[end_pos] <= 'f') ||
            (remaining[end_pos] >= 'A' && remaining[end_pos] <= 'F'))) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw ParsingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// UnbasedImmediate = (0-9)+
string StructuralNetlistParser::ConsumeUnbasedImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume UnbasedImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    size_t end_pos = 0;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '9')) {
      ++end_pos;
    }
    if (end_pos < 1) {
      throw ParsingException(error_msg);
    }
    incremental_token.append(remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// StringLiteral = ".*"
string StructuralNetlistParser::ConsumeStringLiteral(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume StringLiteral from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.length() < 2 || remaining[0] != '"') {
    throw ParsingException(error_msg);
  } else {
    size_t end_pos = 1;
    while (end_pos < remaining.length()) {
      if (remaining[end_pos++] == '"') {
        break;
      }
    }
    if (remaining[end_pos - 1] != '"') {
      throw ParsingException(error_msg);
    }
    incremental_token = remaining.substr(0, end_pos);
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// BitRange = \[UnbasedImmediate\] || \[UnbasedImmediate:UnbasedImmediate\] 
string StructuralNetlistParser::ConsumeBitRange(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume BitRange from: " + input + "\n";
  string incremental_token;
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    try {
      string first_bit_num, second_bit_num;
      remaining = ConsumeChar(remaining, nullptr, '[');
      remaining = ConsumeUnbasedImmediate(remaining, &first_bit_num);
      if (!remaining.empty() && remaining[0] == ':') {
        // Multi-bit range.
        remaining = ConsumeChar(remaining, nullptr, ':');
        remaining = ConsumeUnbasedImmediate(remaining, &second_bit_num);
      } else {
        // Single-bit.
        second_bit_num = first_bit_num;
      }
      remaining = ConsumeChar(remaining, nullptr, ']');
      incremental_token = "[" + first_bit_num;
      if (first_bit_num != second_bit_num) {
        incremental_token.append(":" + second_bit_num);
      }
      incremental_token.append("]");
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

// ConnectionList = Connection[, Connection]*
string StructuralNetlistParser::ConsumeConnectionList(
    const string& input, std::string* token) {
  const string error_msg =
    "Failed to consume ConnectionList from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw ParsingException(error_msg);
  } else {
    try {
      remaining = ConsumeConnection(remaining, &incremental_token);
      while (!remaining.empty() && remaining[0] == ',') {
        string identifier;
        remaining = ConsumeChar(remaining, nullptr, ',');
        remaining = ConsumeConnection(remaining, &identifier);
        incremental_token += ", ";
        incremental_token += identifier;
      }
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

string StructuralNetlistParser::ConsumeChar(
    const string& input, string* token, char c) {
  const string error_msg =
    "Failed to consume Char '" + string(1, c) + "' from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty() || remaining[0] != c) {
    throw ParsingException(error_msg);
  } else {
    incremental_token = c;
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining.substr(1, string::npos), nullptr);
}

string StructuralNetlistParser::ConsumeWrappedElement(
    const string& input, string* token,
    string (*consumer)(const string&, string*),
    char open, char close) {
  const string error_msg =
    "Failed to consume WrappedElement '" + string(1, open) + string(1, close) +
    "' from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty() || remaining[0] != open) {
    throw ParsingException(error_msg + "Cause: Non-matching open char in: " +
                           remaining + "\n");
  } else {
    try {
      size_t num_wraps = 0;
      while (num_wraps < remaining.length() && remaining[num_wraps] == open) {
        ++num_wraps;
        incremental_token.push_back(open);
      }
      string identifier_list;
      remaining = consumer(
          remaining.substr(num_wraps, string::npos), &identifier_list);
      incremental_token.append(identifier_list);
      for (size_t i = 0; i < num_wraps && !remaining.empty(); ++i) {
        if (remaining[0] != close) {
          throw ParsingException(
              error_msg + "Cause: Non-matching closing char in: " + remaining +
              "\n");
        }
        incremental_token.push_back(close);
        remaining = remaining.substr(1, string::npos);
      }
    } catch (ParsingException& e) {
      throw ParsingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

string StructuralNetlistParser::ExtractConnectedElementFromConnection(
    const string& connection) {
  // Check that 'connection' is actually a valid Connection token.
  string clean_connected_element;
  string remaining = ConsumeConnection(connection, &clean_connected_element);
  assert(remaining.empty());

  remaining = clean_connected_element;
  if (remaining[0] == '.') {
    // Named connection.
    remaining = ConsumeChar(remaining, nullptr, '.');
    remaining = ConsumeIdentifier(remaining, nullptr);
    string wrapped_connected_element;
    ConsumeWrappedElement(
        remaining, &wrapped_connected_element, &ConsumeConnectedElement,
        '(', ')');
    return ExtractInner(wrapped_connected_element);
  } else {
    // Positional connection(s). Just return the connection.
    return remaining;
  }
}

vector<string> StructuralNetlistParser::ExtractIdentifiersFromIdentifierList(
    const string& identifier_list) {
  vector<string> identifiers;

  // Check that 'identifier_list' is actually a valid IdentifierList token.
  string clean_identifier_list;
  string remaining =
    ConsumeIdentifierList(identifier_list, &clean_identifier_list);
  assert(remaining.empty());

  string identifier;
  remaining = ConsumeIdentifier(clean_identifier_list, &identifier);
  identifiers.push_back(identifier);
  while (!remaining.empty()) {
    remaining = ConsumeChar(remaining, nullptr, ',');
    remaining = ConsumeIdentifier(remaining, &identifier);
    identifiers.push_back(identifier);
  }
  return identifiers;
}

vector<string> StructuralNetlistParser::ExtractConnectedElementsFromConnectionList(
    const string& connection_list) {
  vector<string> connected_elements;

  // Check that 'connection_list' is actually a valid ConnectionList token.
  string clean_connection_list;
  string remaining =
    ConsumeConnectionList(connection_list, &clean_connection_list);
  assert(remaining.empty());

  string connection;
  remaining = ConsumeConnection(clean_connection_list, &connection);
  string connected_element =
    ExtractConnectedElementFromConnection(connection);
  vector<string> sub_connected_elements =
    ExtractConnectedElementsFromConnectedElement(connected_element);
  for (const string& sub_element : sub_connected_elements) {
    connected_elements.push_back(sub_element);
  }
  while (!remaining.empty()) {
    remaining = ConsumeChar(remaining, nullptr, ',');
    remaining = ConsumeConnection(remaining, &connection);
    connected_element =
        ExtractConnectedElementFromConnection(connection);
    sub_connected_elements =
      ExtractConnectedElementsFromConnectedElement(connected_element);
    for (const string& sub_element : sub_connected_elements) {
      connected_elements.push_back(sub_element);
    }
  }
  return connected_elements;
}

vector<string> StructuralNetlistParser::ExtractConnectedElementsFromConnectedElement(
    const string& connected_element) {
  vector<string> connected_elements;

  // Check that 'connection_list' is actually a valid ConnectionList token.
  string clean_connected_element;
  string remaining =
    ConsumeConnectedElement(connected_element, &clean_connected_element);
  assert(remaining.empty());

  if (!clean_connected_element.empty()) {
    if (clean_connected_element[0] == '{') {
      // The element is a concatenation of elements.
      string wrapped_connection_list;
      ConsumeWrappedElement(
          clean_connected_element, &wrapped_connection_list,
          &ConsumeConnectionList, '{', '}');
      string connection_list = ExtractInner(wrapped_connection_list);
      connected_elements =
          ExtractConnectedElementsFromConnectionList(connection_list);
    } else {
      connected_elements.push_back(clean_connected_element);
    }
  }

  return connected_elements;
}

pair<int, int> StructuralNetlistParser::ExtractBitRange(
    const string& bit_range) {
  pair<int, int> bit_num_pair = {0, 0};
  string remaining = ConsumeWhitespaceIfPresent(bit_range, nullptr);

  // Check that it is actually a BitRange token.
  string clean_bit_range;
  remaining = ConsumeBitRange(remaining, &clean_bit_range);
  assert(remaining.empty());
 
  remaining = clean_bit_range;
  string first_bit_num, second_bit_num;
  remaining = ConsumeChar(remaining, nullptr, '[');
  remaining = ConsumeUnbasedImmediate(remaining, &first_bit_num);
  if (!remaining.empty() && remaining[0] == ':') {
    remaining = ConsumeChar(remaining, nullptr, ':');
    remaining = ConsumeUnbasedImmediate(remaining, &second_bit_num);
  } else {
    second_bit_num = first_bit_num;
  }
  remaining = ConsumeChar(remaining, nullptr, ']');
  int first_int = boost::lexical_cast<int>(first_bit_num);
  int second_int = boost::lexical_cast<int>(second_bit_num);
  bit_num_pair.first = (first_int > second_int) ? first_int : second_int;
  bit_num_pair.second = (first_int > second_int) ? second_int : first_int;
  return bit_num_pair;
}

string StructuralNetlistParser::ExtractInner(const string& input) {
  if (input.empty()) {
    return input;
  }
  assert (input.length() > 1);
  string inner = input.substr(1, input.length() - 2);
  return ConsumeWhitespaceIfPresent(inner, nullptr);
}
