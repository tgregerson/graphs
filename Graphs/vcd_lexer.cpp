/*
 * vcd_lexer.cpp
 *
 *  Created on: Nov 14, 2014
 *      Author: gregerso
 */

using namespace std;

#include "vcd_lexer.h"

#include "structural_netlist_lexer.h"

string VcdLexer::ConsumeExactString(
    const string& str, const string& input, string* token) {
  bool failed = false;
  string remaining =
      StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  if (str.length() > input.length()) {
    failed = true;
  } else {
    string prefix = input.substr(0, str.length());
    if (prefix != str) {
      failed = true;
    } else if (token != nullptr) {
      *token = prefix;
    }
  }
  if (failed) {
    throw LexingException("ConsumeString: " + str + " - " + input);
  } else {
    remaining = input.substr(str.length(), string::npos);
    remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(remaining);
    return remaining;
  }
}

string VcdLexer::ConsumeTag(
    const string& tag_name, const string& input, string* token) {
  try {
    return ConsumeExactString("$" + tag_name, input, token);
  } catch (LexingException& e) {
    string msg = "ConsumeTag: ";
    throw LexingException(msg + e.what());
  }
}

string VcdLexer::ConsumeNonWhitespace(
    const string& input, string* token) {
  string remaining =
      StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  if (remaining.empty()) {
    throw LexingException("ConsumeNonWhitespace");
  }
  int end_pos = 0;
  while (end_pos < remaining.length() && !isspace(remaining[end_pos])) {
    ++end_pos;
  }
  if (token != nullptr) {
    *token = remaining.substr(0, end_pos);
  }
  return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
      remaining.substr(end_pos, string::npos));
}

string VcdLexer::ConsumeVar(
    const string& input, string* token) {
  string remaining;
  string incremental_token;
  string temp_token;
  remaining = ConsumeTag("var", input, &incremental_token);
  // TODO look up specification; there is probably more possibilities than
  // just wire.
  remaining = ConsumeExactString("wire", remaining, &temp_token);
  incremental_token += temp_token;
  remaining =
      StructuralNetlistLexer::ConsumeUnbasedImmediate(remaining, &temp_token);
  incremental_token += temp_token;
  remaining = ConsumeNonWhitespace(remaining, &temp_token);
  incremental_token += temp_token;
  remaining =
      StructuralNetlistLexer::ConsumeIdentifier(remaining, &temp_token);
  incremental_token += temp_token;
  remaining = ConsumeTag("end", remaining, &temp_token);
  incremental_token += temp_token;
  if (token != nullptr) {
    *token = incremental_token;
  }
  return remaining;
}

