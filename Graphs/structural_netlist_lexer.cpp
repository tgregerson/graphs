/*
 * structural_netlist_lexer.cpp
 *
 *  Created on: Sep 4, 2014
 *      Author: gregerso
 */

#include "structural_netlist_lexer.h"

#include <cassert>

#include <algorithm>
#include <iostream>

#include <boost/lexical_cast.hpp>

using namespace std;

// Whitespace is a sequential combination of spaces, newlines, and tabs.
string StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
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

bool StructuralNetlistLexer::ConsumeWhitespaceStream(
    istream& input, string* token) {
  if (!isspace(input.peek())) {
    return false;
  }
  if (token != nullptr) {
    token->clear();
  }
  while (isspace(input.peek())) {
    if (token != nullptr) {
      token->push_back((char)input.get());
    }
  }
  return true;
}

// Identifier = SimpleIdentifier || EscapedIdentifier
string StructuralNetlistLexer::ConsumeIdentifier(
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
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  } else {
    throw LexingException(error_msg);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeIdentifierStream(
    istream& input, std::string* token) {
  ConsumeWhitespaceStream(input);
  if (!input.eof()) {
    if (input.peek() == '\\') {
      return ConsumeEscapedIdentifierStream(input, token);
    } else {
      return ConsumeSimpleIdentifierStream(input, token);
    }
  } else {
    return false;
  }
}

// IdentifierList = Identifier[, Identifier]*
string StructuralNetlistLexer::ConsumeIdentifierList(
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
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
    if (token != nullptr) {
      token->assign(incremental_token);
    }
  } else {
    throw LexingException(error_msg);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeIdentifierListStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    bool valid = true;
    valid &= ConsumeIdentifierStream(input, token);
    ConsumeWhitespaceStream(input);
    while (valid && ConsumeCharStream(input, nullptr, ',')) {
      string identifier;
      valid &= ConsumeIdentifierStream(input, &identifier);
      if (nullptr != token) {
        token->append(", " + identifier);
      }
    }
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// SimpleIdentifier = (_ || a-z)(_ || a-z || 0-9 || $)*
string StructuralNetlistLexer::ConsumeSimpleIdentifier(
    const string& input, std::string* token) {
  const string error_msg =
      "Failed to parse SimpleIdentifier from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  size_t end_pos = 0;
  if (remaining.empty() ||
      !(isalpha(remaining[0]) || remaining[0] == '_')) {
    throw LexingException(error_msg);
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

bool StructuralNetlistLexer::ConsumeSimpleIdentifierStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  char c = input.peek();
  if (EOF == 'c' || '_' == c || !isalpha(c)) {
    return false;
  } else {
    if (nullptr != token) {
      token->clear();
    }
    while (!isalpha(c) || EOF == c || '_' == c || '$' == c) {
      if (nullptr != token) {
        token->push_back(c);
      }
      input.ignore();
      c = input.peek();
    }
    return true;
  }
}

// EscapedIdentifier = \\.*<single whitespace char>
string StructuralNetlistLexer::ConsumeEscapedIdentifier(
    const string& input, std::string* token) {
  const string error_msg =
    "Failed to consume EscapedIdentifier from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  size_t end_pos = 0;
  if (remaining.empty() || !(remaining[0] == '\\')) {
    throw LexingException(error_msg);
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
      throw LexingException(error_msg + additional);
    }
  }
  string my_token = remaining.substr(0, end_pos);
  string new_input = remaining.substr(end_pos, string::npos);
  if (token != nullptr) {
    token->assign(my_token);
  }
  return ConsumeWhitespaceIfPresent(new_input, nullptr);
}

bool StructuralNetlistLexer::ConsumeEscapedIdentifierStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if ('\\' != input.peek()) {
    return false;
  } else {
    if (nullptr != token) {
      token->clear();
    }
    char c = input.peek();
    while (' ' != c && EOF != c) {
      if (nullptr != token) {
        token->push_back(c);
      }
      input.ignore();
      input.peek();
    }
    return true;
  }
}

// Connection = ConnectedElement || .Identifier(ConnectedElement)
string StructuralNetlistLexer::ConsumeConnection(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume Connection from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
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
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeConnectionStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    if ('.' == input.peek()) {
      // Connection by name.
      bool valid = true;
      std::streampos initial_pos = input.tellg();
      input.ignore();
      string identifier;
      valid &= ConsumeIdentifierStream(input, &identifier);
      string wrapped_connected_element;
      valid &= ConsumeWrappedElementStream(
          input, &wrapped_connected_element, &ConsumeConnectedElementStream,
          '(', ')');
      if (valid) {
        if (nullptr != token) {
          token->assign("." + identifier + wrapped_connected_element);
        }
      } else {
        input.seekg(initial_pos);
      }
      return valid;
    } else {
      // Connection by position.
      return ConsumeConnectedElementStream(input, token);
    }
  }
}

// ConnectionList = Connection[, Connection]*
string StructuralNetlistLexer::ConsumeConnectionList(
    const string& input, std::string* token) {
  const string error_msg =
    "Failed to consume ConnectionList from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
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
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeConnectionListStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);

  std::streampos initial_pos = input.tellg();
  bool valid = ConsumeConnectionStream(input, token);
  ConsumeWhitespaceStream(input);
  string connection;
  while (valid && ConsumeCharStream(input, nullptr, ',')) {
    valid &= ConsumeConnectionStream(input, &connection);
    ConsumeWhitespaceStream(input);
    if (nullptr != token) {
      token->append(", " + connection);
    }
  }
  if (!valid) {
    input.seekg(initial_pos);
  }
  return valid;
}

// ConnectedElement = (Identifier[BitRange]) || {ConnectedElementList} || Immediate
string StructuralNetlistLexer::ConsumeConnectedElement(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ConnectedElement from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    try {
      char c = remaining[0];
      if (c == '{') {
        // {ConnectionList}
        string wrapped_connection_list;
        remaining = ConsumeWrappedElement(
            remaining, &wrapped_connection_list, &ConsumeConnectedElementList,
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
        } catch (LexingException e) {
          // BitRange is optional, so do nothing if not present.
        }
      }
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return remaining;
}

bool StructuralNetlistLexer::ConsumeConnectedElementStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    char c = input.peek();
    if ('{' == c) {
      // {ConnectionList}
      return ConsumeWrappedElementStream(
          input, token, &ConsumeConnectedElementListStream, '{', '}');
    } else if ('"' == c || isdigit(c)) {
      // Immediate value
      return ConsumeImmediateStream(input, token);
    } else {
      // Identifier
      bool valid = ConsumeIdentifierStream(input, token);
      if (valid) {
        string bit_range;
        if (ConsumeBitRangeStream(input, &bit_range) && nullptr != token) {
          token->append(bit_range);
        }
      }
      return valid;
    }
  }
}

// ConnectedElementList = ConnectedElement[, ConnectedElement]*
string StructuralNetlistLexer::ConsumeConnectedElementList(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ConnectedElementList from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    try {
      remaining = ConsumeConnectedElement(remaining, &incremental_token);
      while (!remaining.empty() && remaining[0] == ',') {
        string identifier;
        remaining = ConsumeChar(remaining, nullptr, ',');
        remaining = ConsumeConnectedElement(remaining, &identifier);
        incremental_token += ", ";
        incremental_token += identifier;
      }
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeConnectedElementListStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    bool valid = ConsumeConnectedElementStream(input, token);
    ConsumeWhitespaceStream(input);
    string identifier;
    while (valid && ConsumeCharStream(input, nullptr, ',')) {
      valid &= ConsumeConnectedElementStream(input, &identifier);
      if (nullptr != token) {
        token->append(", " + identifier);
      }
    }
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// ParameterConnectedElement = Identifier || Immediate
string StructuralNetlistLexer::ConsumeParameterConnectedElement(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ParameterConnectedElement from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    try {
      char c = remaining[0];
      if (isdigit(c) || c == '"') {
        // Immediate value.
        remaining = ConsumeImmediate(remaining, &incremental_token);
      } else {
        // Identifier
        remaining = ConsumeIdentifier(remaining, &incremental_token);
      }
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return remaining;
}

bool StructuralNetlistLexer::ConsumeParameterConnectedElementStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);

  char c = input.peek();
  if (isdigit(c) || '"' == c) {
    return ConsumeImmediateStream(input, token);
  } else {
    return ConsumeIdentifierStream(input, token);
  }
}

// ParameterConnection = ParameterConnectedElement ||
//                       .Identifier(ParameterConnectedElement)
string StructuralNetlistLexer::ConsumeParameterConnection(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ParameterConnection from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
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
            remaining, &wrapped_connected_element,
            &ConsumeParameterConnectedElement,
            '(', ')');
        incremental_token = "." + identifier + wrapped_connected_element;
      } else {
        // Connection by position.
        remaining = ConsumeParameterConnectedElement(
            remaining, &incremental_token);
      }
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeParameterConnectionStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    if ('.' == input.peek()) {
      // Connection by name
      std::streampos initial_pos = input.tellg();
      input.ignore();
      string identifier;
      string wrapped_connected_element;
      bool valid = ConsumeIdentifierStream(input, &identifier);
      valid &= ConsumeWrappedElementStream(
          input, &wrapped_connected_element,
          &ConsumeParameterConnectedElementStream, '(', ')');
      if (valid) {
        if (nullptr != token) {
          token->assign("." + identifier + wrapped_connected_element);
        }
      } else {
        input.seekg(initial_pos);
      }
      return valid;
    } else {
      // Connection by position
      return ConsumeParameterConnectedElementStream(input, token);
    }
  }
}

// ParameterList = ParameterConnection[, ParameterConnection]*
string StructuralNetlistLexer::ConsumeParameterList(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ParameterList from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    try {
      remaining = ConsumeParameterConnection(remaining, &incremental_token);
      while (!remaining.empty() && remaining[0] == ',') {
        string identifier;
        remaining = ConsumeChar(remaining, nullptr, ',');
        remaining = ConsumeParameterConnection(remaining, &identifier);
        incremental_token += ", ";
        incremental_token += identifier;
      }
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeParameterListStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    bool valid = ConsumeParameterConnectionStream(input, token);
    ConsumeWhitespaceStream(input);
    string identifier;
    while (valid && ConsumeCharStream(input, nullptr, ',')) {
      valid &= ConsumeParameterConnectionStream(input, &identifier);
      ConsumeWhitespaceStream(input);
      if (nullptr != token) {
        token->append(", " + identifier);
      }
    }
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// ModuleParameters = #(ParameterList)
string StructuralNetlistLexer::ConsumeModuleParameters(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume ParameterList from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.size() < 3) { // Must have at least '#()'
    throw LexingException(error_msg);
  } else {
    try {
      remaining = ConsumeChar(remaining, &incremental_token, '#');
      string wrapped_parameter_list;
      remaining = ConsumeWrappedElement(
          remaining, &wrapped_parameter_list, &ConsumeParameterList, '(',
          ')');
      incremental_token.append(wrapped_parameter_list);
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return remaining;
}

bool StructuralNetlistLexer::ConsumeModuleParametersStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if ('#' != input.peek()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    input.ignore();
    string wrapped_parameter_list;
    bool valid = ConsumeWrappedElementStream(
        input, &wrapped_parameter_list, &ConsumeParameterListStream, '(', ')');
    if (valid) {
      if (nullptr != token) {
        token->assign("#" + wrapped_parameter_list);
      }
    } else {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// Immediate = BinaryImmediate || DecimalImmediate || HexImmediate ||
//             OctalImmediate || UnbasedImmediate || StringLiteral
string StructuralNetlistLexer::ConsumeImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume Immediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
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
              throw LexingException("");
          }
        }
      } else {
        throw LexingException("");
      }
    } catch (LexingException& e) {
      throw (e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeImmediateStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  char c = input.peek();
  if ('"' == c) {
    // String literal
    return ConsumeStringLiteralStream(input, token);
  } else if (isdigit(c)) {
    // Value
    std::streampos initial_pos = input.tellg();
    bool valid = ConsumeUnbasedImmediateStream(input, token);
    if (valid && '\'' == input.peek()) {
      input.ignore();
      char base = input.peek();
      // The unbased immediate we consumed earlier was the bitwidth for the
      // based number. Restore it to the stream.
      input.seekg(initial_pos);
      switch (base) {
        case 'b':
          return ConsumeBinaryImmediateStream(input, token);
        case 'o':
          return ConsumeOctalImmediateStream(input, token);
        case 'd':
          return ConsumeDecimalImmediateStream(input, token);
        case 'h':
          return ConsumeHexImmediateStream(input, token);
        default:
          return false;
      }
    }
    return valid;
  } else {
    return false;
  }
}

// BinaryImmediate = (UnbasedImmedate)'b(0-1)+
string StructuralNetlistLexer::ConsumeBinaryImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume BinaryImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' ||
             remaining[1] != 'b') {
      throw LexingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '1')) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw LexingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeBinaryImmediateStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    bool valid = ConsumeUnbasedImmediateStream(input, token);
    if ('\'' == input.get() && 'b' == input.get()) {
      if (nullptr != token) {
        token->append("\'b");
      }
      valid = false; // Make sure at least one bit is found.
      char c = input.peek();
      while ('0' == c || '1' == c) {
        valid = true;
        input.ignore();
        if (nullptr != token) {
          token->push_back(c);
        }
        c = input.peek();
      }
    } else {
      valid = false;
    }
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// OctalImmediate = (UnbasedImmediate)'o(0-7)+
string StructuralNetlistLexer::ConsumeOctalImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume OctalImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' ||
             remaining[1] != 'o') {
      throw LexingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '7')) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw LexingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeOctalImmediateStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    bool valid = ConsumeUnbasedImmediateStream(input, token);
    if ('\'' == input.get() && 'o' == input.get()) {
      if (nullptr != token) {
        token->append("\'o");
      }
      valid = false; // Make sure at least one bit is found.
      char c = input.peek();
      while ('0' <= c && '7' >= c) {
        valid = true;
        input.ignore();
        if (nullptr != token) {
          token->push_back(c);
        }
        c = input.peek();
      }
    } else {
      valid = false;
    }
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// DecimalImmediate = (UnbasedImmediate)'d(0-9)+
string StructuralNetlistLexer::ConsumeDecimalImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume DecimalImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' ||
             remaining[1] != 'd') {
      throw LexingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '9')) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw LexingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeDecimalImmediateStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    bool valid = ConsumeUnbasedImmediateStream(input, token);
    if ('\'' == input.get() && 'd' == input.get()) {
      if (nullptr != token) {
        token->append("\'d");
      }
      valid = false; // Make sure at least one bit is found.
      char c = input.peek();
      while ('0' <= c && '9' >= c) {
        valid = true;
        input.ignore();
        if (nullptr != token) {
          token->push_back(c);
        }
        c = input.peek();
      }
    } else {
      valid = false;
    }
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// HexImmediate = (UnbasedImmediate)'h(0-9 || a-f || A-F)+
string StructuralNetlistLexer::ConsumeHexImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume HexImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    string num_bits;
    try {
      remaining = ConsumeUnbasedImmediate(remaining, &num_bits);
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
    if (remaining.size() < 2 || remaining[0] != '\'' ||
             remaining[1] != 'h') {
      throw LexingException(error_msg);
    }
    size_t end_pos = 2;
    while (end_pos < remaining.size() &&
           ((remaining[end_pos] >= '0' && remaining[end_pos] <= '9') ||
            (remaining[end_pos] >= 'a' && remaining[end_pos] <= 'f') ||
            (remaining[end_pos] >= 'A' && remaining[end_pos] <= 'F'))) {
      ++end_pos;
    }
    if (end_pos < 3) {
      throw LexingException(error_msg);
    }
    incremental_token.append(num_bits + remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeHexImmediateStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if (input.eof()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    bool valid = ConsumeUnbasedImmediateStream(input, token);
    if ('\'' == input.get() && 'h' == input.get()) {
      if (nullptr != token) {
        token->append("\'h");
      }
      valid = false; // Make sure at least one bit is found.
      char c = input.peek();
      while (('0' <= c && '9' >= c) ||
             ('a' <= c && 'f' >= c) ||
             ('A' <= c && 'F' >= c)) {
        valid = true;
        input.ignore();
        if (nullptr != token) {
          token->push_back(c);
        }
        c = input.peek();
      }
    } else {
      valid = false;
    }
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

// UnbasedImmediate = (0-9)+
string StructuralNetlistLexer::ConsumeUnbasedImmediate(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume UnbasedImmediate from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty()) {
    throw LexingException(error_msg);
  } else {
    size_t end_pos = 0;
    while (end_pos < remaining.size() &&
           (remaining[end_pos] >= '0' && remaining[end_pos] <= '9')) {
      ++end_pos;
    }
    if (end_pos < 1) {
      throw LexingException(error_msg);
    }
    incremental_token.append(remaining.substr(0, end_pos));
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeUnbasedImmediateStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  bool valid = false;
  if (nullptr != token) {
    token->clear();
  }
  char c = input.peek();
  while (isdigit(c)) {
    valid = true;
    if (nullptr != token) {
      token->push_back(c);
    }
  }
  return valid;
}

// StringLiteral = ".*"
string StructuralNetlistLexer::ConsumeStringLiteral(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume StringLiteral from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.length() < 2 || remaining[0] != '"') {
    throw LexingException(error_msg);
  } else {
    size_t end_pos = 1;
    while (end_pos < remaining.length()) {
      if (remaining[end_pos++] == '"') {
        break;
      }
    }
    if (remaining[end_pos - 1] != '"') {
      throw LexingException(error_msg);
    }
    incremental_token = remaining.substr(0, end_pos);
    remaining = remaining.substr(end_pos, string::npos);
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeStringLiteralStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if ('"' != input.peek()) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    input.ignore();
    if (nullptr != token) {
      *token = "\"";
    }
    char c;
    do {
      c = input.get();
      if (nullptr != token) {
        token->push_back(c);
      }
    } while (EOF != c && '"' != c);
    if ('"' == c) {
      return true;
    } else {
      input.seekg(initial_pos);
      return false;
    }
  }
}

// BitRange = \[UnbasedImmediate\] || \[UnbasedImmediate:UnbasedImmediate\]
string StructuralNetlistLexer::ConsumeBitRange(
    const string& input, string* token) {
  const string error_msg =
    "Failed to consume BitRange from: " + input + "\n";
  string incremental_token;
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  if (remaining.empty()) {
    throw LexingException(error_msg);
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
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeBitRangeStream(
    istream& input, string* token) {
  ConsumeWhitespaceStream(input);
  if ('[' == input.peek()) {
    std::streampos initial_pos = input.tellg();
    input.ignore();
    string first_bit_num, second_bit_num;
    bool valid = ConsumeUnbasedImmediateStream(input, &first_bit_num);
    if (ConsumeCharStream(input, nullptr, ':')) {
      // Range
      valid &= ConsumeUnbasedImmediateStream(input, &second_bit_num);
    }
    valid &= ConsumeCharStream(input, nullptr, ']');
    if (valid) {
      if (nullptr != token) {
        token->assign("[" + first_bit_num);
        if (!second_bit_num.empty()) {
          token->append(":" + second_bit_num);
        }
        token->push_back(']');
      }
    } else {
      input.seekg(initial_pos);
    }
    return valid;
  } else {
    return false;
  }
}

string StructuralNetlistLexer::ConsumeChar(
    const string& input, string* token, char c) {
  const string error_msg =
    "Failed to consume Char '" + string(1, c) + "' from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty() || remaining[0] != c) {
    throw LexingException(error_msg);
  } else {
    incremental_token = c;
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining.substr(1, string::npos), nullptr);
}

bool StructuralNetlistLexer::ConsumeCharStream(
    istream& input, string* token, char c) {
  ConsumeWhitespaceStream(input);
  if (c == (char)input.peek()) {
    if (nullptr != token) {
      token->clear();
      token->push_back(c);
    }
    input.ignore();
    return true;
  } else {
    return false;
  }
}

string StructuralNetlistLexer::ConsumeWrappedElement(
    const string& input, string* token,
    string (*consumer)(const string&, string*),
    char open, char close) {
  const string error_msg =
    "Failed to consume WrappedElement '" + string(1, open) + string(1, close) +
    "' from: " + input + "\n";
  string remaining = ConsumeWhitespaceIfPresent(input, nullptr);
  string incremental_token;
  if (remaining.empty() || remaining[0] != open) {
    throw LexingException(error_msg + "Cause: Non-matching open char in: " +
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
          throw LexingException(
              error_msg + "Cause: Non-matching closing char in: " + remaining +
              "\n");
        }
        incremental_token.push_back(close);
        remaining = remaining.substr(1, string::npos);
      }
    } catch (LexingException& e) {
      throw LexingException(e.what() + error_msg);
    }
  }
  if (token != nullptr) {
    token->assign(incremental_token);
  }
  return ConsumeWhitespaceIfPresent(remaining, nullptr);
}

bool StructuralNetlistLexer::ConsumeWrappedElementStream(
    istream& input, string* token, bool (*consumer)(istream&, string*),
    char open, char close) {
  ConsumeWhitespaceStream(input);
  if (input.peek() != open) {
    return false;
  } else {
    std::streampos initial_pos = input.tellg();
    if (nullptr != token) {
      token->clear();
    }
    size_t num_wraps = 0;
    char c = input.peek();
    while (c == open) {
      input.ignore();
      ++num_wraps;
      if (nullptr != token) {
        token->push_back(open);
      }
      c = input.peek();
    }
    string identifier_list;
    bool valid = consumer(input, &identifier_list);
    while (valid && num_wraps > 0 && input.peek() == close) {
      input.ignore();
      if (nullptr != token) {
        token->push_back(close);
      }
    }
    valid &= num_wraps == 0;
    if (!valid) {
      input.seekg(initial_pos);
    }
    return valid;
  }
}

pair<string, string> StructuralNetlistLexer::ExtractConnectionFromConnection(
    const string& connection) {
  pair<string,string> parsed_connection;
  // Check that 'connection' is actually a valid Connection token.
  string clean_connected_element;
  assert(ConsumeConnection(connection, &clean_connected_element).empty());

  if (clean_connected_element[0] == '.') {
    // Named connection.
    clean_connected_element = ConsumeChar(
        clean_connected_element, nullptr, '.');
    clean_connected_element = ConsumeIdentifier(
        clean_connected_element, &(parsed_connection.first));
    string wrapped_connected_element;
    ConsumeWrappedElement(
        clean_connected_element, &wrapped_connected_element, &ConsumeConnectedElement,
        '(', ')');
   clean_connected_element = ExtractInner(wrapped_connected_element);
  }
  parsed_connection.second = clean_connected_element;
  return parsed_connection;
}

pair<string, string> StructuralNetlistLexer::ExtractParameterConnectionFromParameterConnection(
    const string& connection) {
  pair<string,string> parsed_connection;
  // Check that 'connection' is actually a valid Connection token.
  string clean_connected_element;
  assert(ConsumeParameterConnection(connection, &clean_connected_element).empty());

  if (clean_connected_element[0] == '.') {
    // Named connection.
    clean_connected_element = ConsumeChar(
        clean_connected_element, nullptr, '.');
    clean_connected_element = ConsumeIdentifier(
        clean_connected_element, &(parsed_connection.first));
    string wrapped_connected_element;
    ConsumeWrappedElement(
        clean_connected_element, &wrapped_connected_element, &ConsumeParameterConnectedElement,
        '(', ')');
    clean_connected_element = ExtractInner(wrapped_connected_element);
  }
  parsed_connection.second = clean_connected_element;
  return parsed_connection;
}

vector<string> StructuralNetlistLexer::ExtractIdentifiersFromIdentifierList(
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

vector<pair<string,string>> StructuralNetlistLexer::ExtractConnectionsFromConnectionList(
    const string& connection_list) {
  vector<pair<string,string>> parsed_connections;

  // Check that 'connection_list' is actually a valid ConnectionList token.
  string clean_connection_list;
  string remaining =
    ConsumeConnectionList(connection_list, &clean_connection_list);
  assert(remaining.empty());

  string connection;
  remaining = ConsumeConnection(clean_connection_list, &connection);
  pair<string,string> parsed_connection =
    ExtractConnectionFromConnection(connection);
  vector<string> sub_connected_elements =
    ExtractConnectedElementsFromConnectedElement(parsed_connection.second);
  for (const string& sub_element : sub_connected_elements) {
    parsed_connections.push_back(
        make_pair(parsed_connection.first, sub_element));
  }
  while (!remaining.empty()) {
    remaining = ConsumeChar(remaining, nullptr, ',');
    remaining = ConsumeConnection(remaining, &connection);
    parsed_connection =
        ExtractConnectionFromConnection(connection);
    sub_connected_elements =
      ExtractConnectedElementsFromConnectedElement(parsed_connection.second);
    for (const string& sub_element : sub_connected_elements) {
      parsed_connections.push_back(
          make_pair(parsed_connection.first, sub_element));
    }
  }
  return parsed_connections;
}

vector<pair<string,string>> StructuralNetlistLexer::ExtractParameterConnectionsFromParameterList(
    const string& plist) {
  vector<pair<string,string>> parsed_connections;

  // Check that 'connection_list' is actually a valid ConnectionList token.
  string clean_parameter_list;
  string remaining = ConsumeParameterList(plist, &clean_parameter_list);
  assert(remaining.empty());

  string parameter_connection;
  remaining = ConsumeParameterConnection(
      clean_parameter_list, &parameter_connection);
  pair<string,string> parsed_connection =
    ExtractParameterConnectionFromParameterConnection(parameter_connection);
  parsed_connections.push_back(parsed_connection);
  while (!remaining.empty()) {
    remaining = ConsumeChar(remaining, nullptr, ',');
    remaining = ConsumeParameterConnection(
        remaining, &parameter_connection);
    pair<string,string> parsed_connection =
        ExtractParameterConnectionFromParameterConnection(parameter_connection);
    parsed_connections.push_back(parsed_connection);
  }
  return parsed_connections;
}

vector<pair<string,string>> StructuralNetlistLexer::ExtractParameterConnectionsFromModuleParameters(
    const string& module_parameters) {
  // Check that 'module_parameters' is actually a valid token.
  string clean_module_parameters;
  string remaining = ConsumeModuleParameters(module_parameters, &clean_module_parameters);
  assert(remaining.empty());

  string plist = ExtractInner(ConsumeChar(clean_module_parameters, nullptr, '#'));
  return ExtractParameterConnectionsFromParameterList(plist);
}

vector<string> StructuralNetlistLexer::ExtractConnectedElementsFromConnectedElement(
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
      string wrapped_connected_element_list;
      ConsumeWrappedElement(
          clean_connected_element, &wrapped_connected_element_list,
          &ConsumeConnectedElementList, '{', '}');
      string connected_element_list = ExtractInner(
          wrapped_connected_element_list);
      connected_elements = ExtractConnectedElementsFromConnectedElementList(
              connected_element_list);
      // Reverse order of concatenated elements so their position in the vector
      // corresponds to the high-order to low-order ordering in a Verilog vector.
      std::reverse(connected_elements.begin(), connected_elements.end());
    } else {
      connected_elements.push_back(clean_connected_element);
    }
  }
  return connected_elements;
}

vector<string> StructuralNetlistLexer::ExtractConnectedElementsFromConnectedElementList(
    const string& connected_element_list) {
  vector<string> connected_elements;

  // Check that 'connection_list' is actually a valid ConnectionList token.
  string clean_connected_element_list;
  string remaining = ConsumeConnectedElementList(
      connected_element_list, &clean_connected_element_list);
  assert(remaining.empty());

  string connected_element;
  remaining = ConsumeConnectedElement(
      clean_connected_element_list, &connected_element);
  vector<string> sub_connected_elements =
    ExtractConnectedElementsFromConnectedElement(connected_element);
  for (const string& sub_element : sub_connected_elements) {
    connected_elements.push_back(sub_element);
  }
  while (!remaining.empty()) {
    remaining = ConsumeChar(remaining, nullptr, ',');
    remaining = ConsumeConnectedElement(remaining, &connected_element);
    vector<string> sub_connected_elements =
      ExtractConnectedElementsFromConnectedElement(connected_element);
    for (const string& sub_element : sub_connected_elements) {
      connected_elements.push_back(sub_element);
    }
  }
  return connected_elements;
}

pair<int, int> StructuralNetlistLexer::ExtractBitRange(
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

string StructuralNetlistLexer::ExtractInner(const string& input) {
  if (input.empty()) {
    return input;
  }
  assert (input.length() > 1);
  string inner = input.substr(1, input.length() - 2);
  return ConsumeWhitespaceIfPresent(inner, nullptr);
}

pair<int,uint64_t> StructuralNetlistLexer::VlogImmediateToUint64 (
    const string& str) {
  pair<int, uint64_t> ret;
  string bits_str;
  string remaining = ConsumeChar(
      ConsumeUnbasedImmediate(str, &bits_str), nullptr, '\'');
  size_t dont_care;
  string prefix_stripped;
  // Figure out the number of characters to strip off the beginning of the
  // string to get rid of the radix format code.
  // If the entire string was an unbased immediate, don't strip anything;
  // it's a decimal value.
  if (remaining.empty()) {
    ret.first = 32;
    prefix_stripped = str;
  } else {
    ret.first = stoi(bits_str, &dont_care, 10);
    int num_bit_chars = (ret.first > 99) ? 3 : (ret.first > 9) ? 2 : 1;
    prefix_stripped = !remaining.empty() ?
        str.substr(num_bit_chars + 2, string::npos) : str;
  }
  try {
    ConsumeBinaryImmediate(str, nullptr);
    try {
      ret.second = stoul(prefix_stripped, &dont_care, 2);
    } catch (std::exception& e) {
      std::cout << e.what() << ": " << prefix_stripped << "\n";
      assert(false);
    }
    return ret;
  } catch (LexingException& e) {}
  try {
    ConsumeDecimalImmediate(str, nullptr);
    try {
      ret.second = stoul(prefix_stripped, &dont_care, 10);
    } catch (std::exception& e) {
      std::cout << e.what() << ": " << prefix_stripped << "\n";
      assert(false);
    }
    return ret;
  } catch (LexingException& e) {}
  try {
    ConsumeHexImmediate(str, nullptr);
    try {
      ret.second = stoul(prefix_stripped, &dont_care, 16);
    } catch (std::exception& e) {
      std::cout << e.what() << " : " << prefix_stripped << " : " << str << "\n";
      assert(false);
    }
    return ret;
  } catch (LexingException& e) {}
  const string error_msg =
      "Failed to convert immediate: " + str + "\n";
  throw LexingException(error_msg);
}

StructuralNetlistLexer::NetDescriptor
StructuralNetlistLexer::ExtractDescriptorFromIdentifier(const std::string& str) {
  NetDescriptor desc;
  string clean_identifier;
  string remaining = ConsumeIdentifier(str, &clean_identifier);
  desc.raw_name = str;
  desc.base_name = clean_identifier;
  if (remaining.empty()) {
    desc.uses_index = false;
    desc.bit_high = 0;
    desc.bit_low = 0;
  } else {
    string bit_range = ConsumeBitRange(remaining, &bit_range);
    pair<int, int> br = ExtractBitRange(bit_range);
    desc.uses_index = true;
    desc.bit_high = br.first;
    desc.bit_low = br.second;
  }
  return desc;
}
