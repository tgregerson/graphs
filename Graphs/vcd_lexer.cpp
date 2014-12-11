/*
 * vcd_lexer.cpp
 *
 *  Created on: Nov 14, 2014
 *      Author: gregerso
 */


#include "vcd_lexer.h"

#include <cctype>

#include <sstream>
#include <utility>

#include "structural_netlist_lexer.h"

using namespace std;

string VcdLexer::ConsumeVcdDefinitions(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);

  bool declarations_done = false;
  while (!declarations_done) {
    try {
      remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
          remaining, &temp_token);
      incremental_token.append(temp_token);
      remaining = ConsumeDeclarationCommand(remaining, &temp_token);
      incremental_token.append(temp_token);
    } catch (LexingException& e) {
      declarations_done = true;
    }
  }

  bool simulations_done = false;
  while (!simulations_done) {
    try {
      remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
          remaining, &temp_token);
      incremental_token.append(temp_token);
      remaining = ConsumeSimulationCommand(remaining, &temp_token);
      incremental_token.append(temp_token);
    } catch (LexingException& e) {
      simulations_done = true;
    }
  }

  if (token != nullptr) {
    *token = incremental_token;
  }
  return remaining;
}

bool VcdLexer::ConsumeVcdDefinitionsStream(
    istream& in, string* token) {
  string consumed;
  ConsumeWhitespaceOptionalStream(in);

  int cur_pos = in.tellg();
  in.seekg(0, ios::end);
  const int max_pos = in.tellg();
  in.seekg(cur_pos);

  int last_print_point = 0;

  bool declarations_done = false;
  while (!declarations_done) {
    // todo remove
    cur_pos = in.tellg();
    if (cur_pos - last_print_point > 1000000) {
      last_print_point = cur_pos;
      cout << "Cur pos: " << in.tellg() << " of "
          << max_pos << endl;
    }
    string append_token;
    ConsumeWhitespaceOptionalStream(in, &append_token);
    consumed.append(append_token);
    if (ConsumeDeclarationCommandStream(in, &append_token)) {
      consumed.append(append_token);
    } else {
      declarations_done = true;
    }
  }

  cout << "-----------Done Lexing Declarations------------" << endl;

  bool simulations_done = false;
  while (!simulations_done) {
    // todo remove
    cur_pos = in.tellg();
    if (cur_pos - last_print_point > 1000000) {
      last_print_point = cur_pos;
      cout << "Cur pos: " << in.tellg() << " of "
          << max_pos << endl;
    }
    string append_token;
    ConsumeWhitespaceOptionalStream(in, &append_token);
    consumed.append(append_token);
    if (ConsumeSimulationCommandStream(in, &append_token)) {
      consumed.append(append_token);
    } else {
      simulations_done = true;
    }
  }

  cout << "-----------Done Lexing Simulations------------" << endl;

  if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return true;
}

string VcdLexer::ConsumeDeclarationCommand(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    remaining = ConsumeDeclarationKeyword(remaining, &incremental_token);
    remaining = ConsumeTextToEnd(remaining, &temp_token);
    incremental_token.append(temp_token);
    if (token != nullptr) {
      *token = incremental_token;
    }
    return remaining;
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeDeclarationCommand: ");
  }
}

bool VcdLexer::ConsumeDeclarationCommandStream(
    istream& in, string* token) {
  string consumed;
  bool found = false;
  if (ConsumeDeclarationKeywordStream(in, &consumed)) {
    string append_token;
    if (ConsumeTextToEndStream(in, &append_token)) {
      consumed.append(append_token);
      found = true;
    }
  }
  if (found && token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string VcdLexer::ConsumeSimulationCommand(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    return ConsumeValueChange(remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeSimulationTime(remaining, token);
  } catch (LexingException& e) {}
  try {
    ConsumeSimulationValueCommand(remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeComment(remaining, token);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeSimulationCommand: ");
  }
}

bool VcdLexer::ConsumeSimulationCommandStream(
    istream& in, string* token) {
  return ConsumeValueChangeStream(in, token) ||
         ConsumeSimulationTimeStream(in, token) ||
         ConsumeSimulationValueCommandStream(in, token) ||
         ConsumeCommentStream(in, token);
}

string VcdLexer::ConsumeSimulationValueCommand(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string ws_temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    remaining = ConsumeSimulationKeyword(remaining, &temp_token);
    incremental_token.append(temp_token);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeSimulationValueCommand: ");
  }
  bool done = false;
  while (!done) {
    try {
      remaining = ConsumeValueChange(
          StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
              remaining, &ws_temp_token), &temp_token);
      incremental_token.append(ws_temp_token + temp_token);
    } catch (LexingException& e) {
      done = true;
    }
  }
  remaining = ConsumeExactString("$end",
      StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
          remaining, &ws_temp_token), &temp_token);
  incremental_token.append(ws_temp_token + temp_token);
  if (token != nullptr) {
    *token = incremental_token;
  }
  return remaining;
}

bool VcdLexer::ConsumeSimulationValueCommandStream(
    istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  string consumed;
  bool found = false;
  stringstream error_msg;
  if (ConsumeSimulationKeywordStream(in, &consumed)) {
    bool done_processing_value_changes = false;
    string append_token;
    while (!done_processing_value_changes) {
      if (ConsumeWhitespaceStream(in, &append_token)) {
        consumed.append(append_token);
        if (ConsumeValueChangeStream(in, &append_token)) {
          consumed.append(append_token);
        } else {
          done_processing_value_changes = true;
        }
      } else {
        done_processing_value_changes = true;
      }
    }
    if (ConsumeExactStringStream("$end", in)) {
      consumed.append("$end");
      found = true;
    }
  }
  if (!found) {
    in.seekg(initial_pos);
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string VcdLexer::ConsumeComment(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    remaining = ConsumeExactString("$comment", remaining, &temp_token);
    remaining = ConsumeTextToEnd(remaining, &temp_token);
    incremental_token.append(temp_token);
    if (token != nullptr) {
      *token = temp_token;
    }
    return remaining;
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeComment: ");
  }
}

bool VcdLexer::ConsumeCommentStream(
    istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  string consumed;
  bool found = false;
  ConsumeWhitespaceOptionalStream(in);
  if (ConsumeExactStringStream("$comment", in, &consumed)) {
    string append_token;
    if (ConsumeTextToEndStream(in, &append_token)) {
      consumed.append(append_token);
      found = true;
    }
  }
  if (!found) {
    in.seekg(initial_pos);
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string VcdLexer::ConsumeDeclarationKeyword(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    return ConsumeExactString("$var", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$comment", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$date", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$enddefinitions", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$scope", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$timescale", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$upscope", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$version", remaining, token);
  } catch (LexingException& e) {
    throw LexingException("ConsumeDeclarationKeyword: " + string(e.what()));
  }
}

bool VcdLexer::ConsumeDeclarationKeywordStream(
    istream& in, string* token) {
  ConsumeWhitespaceOptionalStream(in);
  return ConsumeExactStringStream("$var", in, token) ||
         ConsumeExactStringStream("$comment", in, token) ||
         ConsumeExactStringStream("$date", in, token) ||
         ConsumeExactStringStream("$enddefinitions", in, token) ||
         ConsumeExactStringStream("$scope", in, token) ||
         ConsumeExactStringStream("$timescale", in, token) ||
         ConsumeExactStringStream("$upscope", in, token) ||
         ConsumeExactStringStream("$version", in, token);
}

string VcdLexer::ConsumeSimulationKeyword(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    return ConsumeExactString("$dumpall", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$dumpoff", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$dumpon", remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeExactString("$dumpvars", remaining, token);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeSimulationKeyword: ");
  }
}

bool VcdLexer::ConsumeSimulationKeywordStream(
    istream& in, string* token) {
  ConsumeWhitespaceOptionalStream(in);
  return ConsumeExactStringStream("$dumpall", in, token) ||
         ConsumeExactStringStream("$dumpoff", in, token) ||
         ConsumeExactStringStream("$dumpon", in, token) ||
         ConsumeExactStringStream("$dumpvars", in, token);
}

string VcdLexer::ConsumeSimulationTime(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  if (remaining.size() < 2 || remaining[0] != '#') {
    throw LexingException("ConsumeSimulationTime: ");
  } else {
    try {
      string temp_token;
      remaining = StructuralNetlistLexer::ConsumeUnbasedImmediate(
          remaining.substr(1, string::npos), &temp_token);
      if (token != nullptr) {
        *token = "#" + temp_token;
      }
      return remaining;
    } catch (LexingException& e) {
      throw LexingException(string(e.what()) + "ConsumeSimulationTime: ");
    }
  }
}

bool VcdLexer::ConsumeSimulationTimeStream(
    istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  string consumed;
  bool found = false;
  ConsumeWhitespaceOptionalStream(in);
  if (!in.eof() && in.peek() == '#') {
    consumed.push_back(in.get());
    string append_token;
    if (ConsumeDecimalNumberStream(in, &append_token)) {
      consumed.append(append_token);
      found = true;
    }
  }
  if (!found) {
    in.seekg(initial_pos);
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string VcdLexer::ConsumeValueNoWhitespace(
    const string& input, string* token) {
  if (input.empty() ||
      !(input[0] == '0' || input[0] == '1' || input[0] == 'x' ||
        input[0] == 'X' || input[0] == 'z' || input[0] == 'Z')) {
    throw LexingException("ConsumeValueNoWhitespace: ");
  } else {
    if (token != nullptr) {
      *token = string(1, input[0]);
    }
    return input.substr(1, string::npos);
  }
}

bool VcdLexer::ConsumeValueNoWhitespaceStream(
    istream& in, string* token) {
  if (!in.eof()) {
    char c = in.peek();
    if (c == '0' || c == '1' || c == 'x' ||
        c == 'z' || c == 'X' || c == 'Z') {
        if (token != nullptr) {
          token->clear();
          token->push_back(c);
        }
        in.get();
        return true;
    }
  }
  return false;
}

string VcdLexer::ConsumeValueChange(
    const string& input, string* token) {
  try {
    return ConsumeScalarValueChange(input, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeVectorValueChange(input, token);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeValueChange: ");
  }
}

bool VcdLexer::ConsumeValueChangeStream(
    istream& in, string* token) {
  return ConsumeScalarValueChangeStream(in, token) ||
         ConsumeVectorValueChangeStream(in, token);
}

string VcdLexer::ConsumeScalarValueChange(
    const string& input, string* token) {
  string remaining;
  string incremental_token;
  string temp_token;
  try {
    remaining =
        StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
    remaining = ConsumeValueNoWhitespace(remaining, &temp_token);
    incremental_token.append(temp_token);
    remaining = ConsumeIdentifierCode(remaining, &temp_token);
    incremental_token.append(temp_token);
    if (token != nullptr) {
      *token = incremental_token;
    }
    return remaining;
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeScalarValue");
  }
}

bool VcdLexer::ConsumeScalarValueChangeStream(
    istream& in, string* token) {
  string consumed;
  bool found = false;
  ConsumeWhitespaceOptionalStream(in);
  if (!in.eof()) {
    string append_token;
    if (ConsumeValueNoWhitespaceStream(in, &consumed) &&
        ConsumeIdentifierCodeStream(in, &append_token)) {
      consumed.append(append_token);
      found = true;
    }
  }
  if (found && token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string VcdLexer::ConsumeBinaryVectorValueChange(
    const string& input, string* token) {
  const string fname = "ConsumeBinaryVectorValueChange: ";
  string remaining =
      StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  if (remaining.empty() ||
      (remaining[0] != 'b' && remaining[0] != 'B')) {
    throw LexingException(fname);
  } else {
    bool done = false;
    bool got_at_least_one_value = false;
    string incremental_token = string(1, remaining[0]);
    string append_token;
    while (!done) {
      try {
        remaining = ConsumeValueNoWhitespace(remaining, &append_token);
        incremental_token.append(append_token);
        got_at_least_one_value = true;
      } catch (LexingException& e) {
        done = true;
      }
    }
    remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        remaining, &append_token);
    incremental_token.append(append_token);
    remaining = ConsumeIdentifierCode(remaining, &append_token);
    incremental_token.append(append_token);

    if (!got_at_least_one_value) {
      throw LexingException(fname);
    } else {
      if (token != nullptr) {
        *token = incremental_token;
      }
      return remaining;
    }
  }
}

bool VcdLexer::ConsumeBinaryVectorValueChangeStream(
    istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  string consumed;
  ConsumeWhitespaceOptionalStream(in);
  bool found = false;
  if (!in.eof() && (in.peek() == 'b' || in.peek() == 'B')) {
    consumed.push_back(in.get());
    string append_token;
    bool done_getting_values = false;
    bool found_at_least_one_value = false;
    while (!done_getting_values) {
      if (ConsumeValueNoWhitespaceStream(in, &append_token)) {
        consumed.append(append_token);
        found_at_least_one_value = true;
      } else {
        done_getting_values = true;
      }
    }
    if (found_at_least_one_value) {
      if (ConsumeWhitespaceStream(in, &append_token)) {
        consumed.append(append_token);
        if (ConsumeIdentifierCodeStream(in, &append_token)) {
          consumed.append(append_token);
          found = true;
        }
      }
    }
  }
  if (!found) {
    in.seekg(initial_pos);
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string VcdLexer::ConsumeRealVectorValueChange(
    const string& input, string* token) {
  const string fname = "ConsumeRealVectorValueChange: ";
  throw LexingException(fname + "Not yet supported.");
}

bool VcdLexer::ConsumeRealVectorValueChangeStream(
    istream& in, string* token) {
  return false;
}

string VcdLexer::ConsumeVectorValueChange(
    const string& input, string* token) {
  try {
    ConsumeBinaryVectorValueChange(input, token);
  } catch (LexingException& e) {}
  try {
    ConsumeRealVectorValueChange(input, token);
  } catch (LexingException& e) {}
  const string fname = "ConsumeVectorValueChange: ";
  throw LexingException(fname);
}

bool VcdLexer::ConsumeVectorValueChangeStream(
    istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  if(ConsumeBinaryVectorValueChangeStream(in, token)) {
    return true;
  } else {
    in.seekg(initial_pos);
    return false;
  }
}

string VcdLexer::ConsumeIdentifierCode(
    const string& input, string* token) {
  try {
    return ConsumeNonWhitespace(input, token);
  } catch (LexingException& e) {
    throw LexingException(string("ConsumeIdentifierCode: ") + e.what());
  }
}

bool VcdLexer::ConsumeIdentifierCodeStream(
    istream& in, string* token) {
  ConsumeWhitespaceOptionalStream(in);
  return ConsumeNonWhitespaceStream(in, token);
}

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
    return remaining;
  }
}

bool VcdLexer::ConsumeExactStringStream(
    const string& str, istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  bool found = true;
  for (char string_c : str) {
    if (in.eof()) {
      found = false;
      break;
    }
    char in_c;
    in.get(in_c);
    if (in_c != string_c) {
      found = false;
      break;
    }
  }
  if (!found) {
    /*
    std::stringstream ss;
    ss << "ConsumeExactString: \n"
       << "Looking for string: '" + str + "'\n"
       << PrintNextStreamLineInfo(in);
    */
    in.seekg(initial_pos);
  } else if (token != nullptr) {
    token->assign(str);
  }
  return found;
}

string VcdLexer::ConsumeTextToEnd(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string remaining = input;
  while (true) {
    try {
      remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
          remaining, &temp_token);
      incremental_token.append(temp_token);
      remaining = ConsumeNonWhitespace(remaining, &temp_token);
      incremental_token.append(temp_token);
      if (temp_token == "$end") {
        if (token != nullptr) {
          *token = incremental_token;
        }
        return remaining;
      }
    } catch (LexingException& e) {
      throw LexingException(string(e.what()) + "ConsumeTextToEnd: ");
    }
  }
  return "";  // Should never occur; added to silence compiler warning.
}

bool VcdLexer::ConsumeTextToEndStream(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  string consumed;
  bool found = false;
  while (!in.eof()) {
    char c;
    in.get(c);
    consumed.push_back(c);
    if (c == '$') {
      found = ConsumeExactStringStream("end", in);
      if (found) {
        consumed.append("end");
        break;
      }
    }
  }
  if (!found) {
    in.seekg(initial_pos);
    cout << "WARNING: ConsumeTextToEnd on full stream!\n";
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string VcdLexer::ConsumeNonWhitespace(
    const string& input, string* token) {
  string remaining =
      StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  if (remaining.empty()) {
    throw LexingException("ConsumeNonWhitespace");
  }
  size_t end_pos = 0;
  while (end_pos < remaining.length() && !isspace(remaining[end_pos])) {
    ++end_pos;
  }
  if (token != nullptr) {
    *token = remaining.substr(0, end_pos);
  }
  return remaining.substr(end_pos, string::npos);
}

bool VcdLexer::ConsumeNonWhitespaceStream(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  if (token != nullptr) {
    token->clear();
  }
  while (!in.eof() && !isspace(in.peek())) {
    char c;
    in.get(c);
    if (token != nullptr) {
      token->push_back(c);
    }
  }
  return initial_pos != in.tellg();
}

bool VcdLexer::ConsumeWhitespaceStream(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  if (token != nullptr) {
    token->clear();
  }
  while (!in.eof() && isspace(in.peek())) {
    char c;
    in.get(c);
    if (token != nullptr) {
      token->push_back(c);
    }
  }
  return initial_pos != in.tellg();
}

void VcdLexer::ConsumeWhitespaceOptionalStream(istream& in, string* token) {
  if (token != nullptr) {
    token->clear();
  }
  while (!in.eof() && isspace(in.peek())) {
    char c = in.get();
    if (token != nullptr) {
      token->push_back(c);
    }
  }
}

bool VcdLexer::ConsumeDecimalNumberStream(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  if (token != nullptr) {
    token->clear();
  }
  while (!in.eof() && isdigit(in.peek())) {
    char c = in.get();
    if (token != nullptr) {
      token->push_back(c);
    }
  }
  return initial_pos != in.tellg();
}

string VcdLexer::PeekNextLineFromStream(istream& in) {
  const int MAX_LINE_CHARS = 1024;
  char line_buffer[MAX_LINE_CHARS];
  std::streampos initial_pos = in.tellg();
  in.getline(line_buffer, MAX_LINE_CHARS);
  bool buffer_full = in.fail();
  in.clear();
  in.seekg(initial_pos);
  if (buffer_full) {
    return string(line_buffer, MAX_LINE_CHARS);
  } else {
    return string(line_buffer);
  }
}

string VcdLexer::PrintNextStreamLineInfo(istream& in) {
    string stream_line = PeekNextLineFromStream(in);

    std::stringstream ss;
    ss << "Next stream line of size " << stream_line.size()
       << " at offset " << in.tellg() << ":\n"
       << stream_line;
    return ss.str();
}
