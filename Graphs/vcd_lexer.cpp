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

namespace vcd_lexer {

unique_ptr<char> global_ws_buffer_ =
    move(unique_ptr<char>(new char[kWsBufferSize]));
unique_ptr<char> global_value_buffer_ =
    move(unique_ptr<char>(new char[kValueBufferSize]));
unique_ptr<char> global_time_buffer_ =
    move(unique_ptr<char>(new char[kTimeBufferSize]));

string ConsumeVcdDefinitionsString(
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
      remaining = ConsumeDeclarationCommandString(remaining, &temp_token);
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
      remaining = ConsumeSimulationCommandString(remaining, &temp_token);
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

string ConsumeDeclarationCommandString(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    remaining = ConsumeDeclarationKeywordString(remaining, &incremental_token);
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

string ConsumeSimulationCommandString(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    return ConsumeValueChangeString(remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeSimulationTimeString(remaining, token);
  } catch (LexingException& e) {}
  try {
    ConsumeSimulationValueCommandString(remaining, token);
  } catch (LexingException& e) {}
  try {
    return ConsumeCommentString(remaining, token);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeSimulationCommand: ");
  }
}

string ConsumeSimulationValueCommandString(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string ws_temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    remaining = ConsumeSimulationKeywordString(remaining, &temp_token);
    incremental_token.append(temp_token);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeSimulationValueCommand: ");
  }
  bool done = false;
  while (!done) {
    try {
      remaining = ConsumeValueChangeString(
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

string ConsumeCommentString(
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

string ConsumeDeclarationKeywordString(
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

string ConsumeSimulationKeywordString(
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

string ConsumeSimulationTimeString(
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

string ConsumeValueNoWhitespaceString(
    const string& input, string* token) {
  if (input.empty()) {
    throw LexingException("ConsumeValueNoWhitespace: Empty");
  } else if (!(input[0] == '0' || input[0] == '1' || input[0] == 'x' ||
               input[0] == 'X' || input[0] == 'z' || input[0] == 'Z')) {
    throw LexingException("ConsumeValueNoWhitespace: Invalid char: " +
                          string(&(input[0]), 1));
  } else {
    if (token != nullptr) {
      *token = string(1, input[0]);
    }
    return input.substr(1, string::npos);
  }
}

bool ConsumeValueNoWhitespace(istream& in, string* token) {
  if (!in.eof()) {
    char c = (char)in.peek();
    if (c == '0' || c == '1' || c == 'x' ||
        c == 'z' || c == 'X' || c == 'Z') {
        if (token != nullptr) {
          token->resize(1);
          token[0] = c;
        }
        in.ignore();
        return true;
    }
  }
  return false;
}

bool ConsumeValueNoWhitespace(istream& in, char** buffer_pos) {
  if (!in.eof()) {
    char c = (char)in.peek();
    if (c == '0' || c == '1' || c == 'x' ||
        c == 'z' || c == 'X' || c == 'Z') {
      **buffer_pos = c;
      ++(*buffer_pos);
      in.ignore();
      return true;
    }
  }
  return false;
}

bool ConsumeValueNoWhitespace(
    FILE* in, string* token) {
  if (token != nullptr) {
    token->resize(1);
  }
  char c = (char)fgetc(in);
  if (c == '0' || c == '1' || c == 'x' ||
      c == 'z' || c == 'X' || c == 'Z') {
    if (token != nullptr) {
      token[0] = c;
    }
    return true;
  }
  if (c != EOF) {
    ungetc(c, in);
  }
  return false;
}

// TODO add buffer max as a parameter.
bool ConsumeValueNoWhitespace(
    FILE* in, char** buffer_pos) {
  char c = (char)fgetc(in);
  if (c == '0' || c == '1' || c == 'x' ||
      c == 'z' || c == 'X' || c == 'Z') {
    //assert(*buffer_pos < global_value_buffer_.get() + kValueBufferSize);
    **buffer_pos = c;
    ++(*buffer_pos);
    return true;
  }
  if (c != EOF) {
    ungetc(c, in);
  }
  return false;
}

string ConsumeValueChangeString(
    const string& input, string* token) {
  string error_msg;
  try {
    return ConsumeScalarValueChangeString(input, token);
  } catch (LexingException& e) {
    error_msg.append(e.what());
  }
  try {
    return ConsumeVectorValueChangeString(input, token);
  } catch (LexingException& e) {
    throw LexingException("ConsumeValueChange: " + error_msg +
                          string(" / ") + e.what());
  }
}

string ConsumeScalarValueChangeString(
    const string& input, string* token) {
  string remaining;
  string incremental_token;
  string temp_token;
  try {
    remaining =
        StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
    remaining = ConsumeValueNoWhitespaceString(remaining, &temp_token);
    incremental_token.append(temp_token);
    remaining = ConsumeIdentifierCodeString(remaining, &temp_token);
    incremental_token.append(temp_token);
    if (token != nullptr) {
      *token = incremental_token;
    }
    return remaining;
  } catch (LexingException& e) {
    throw LexingException("ConsumeScalarValue: " + string(e.what()));
  }
}

string ConsumeBinaryVectorValueChangeString(
    const string& input, string* token) {
  const string fname = "ConsumeBinaryVectorValueChange: ";
  string remaining =
      StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  if (remaining.empty() ||
      (remaining[0] != 'b' && remaining[0] != 'B')) {
    throw LexingException(string(fname) + "Invalid starting char");
  } else {
    bool done = false;
    bool got_at_least_one_value = false;
    string incremental_token = string(1, remaining[0]);
    remaining = remaining.substr(1, string::npos);
    string append_token;
    while (!done) {
      try {
        remaining = ConsumeValueNoWhitespaceString(remaining, &append_token);
        incremental_token.append(append_token);
        got_at_least_one_value = true;
      } catch (LexingException& e) {
        done = true;
      }
    }
    remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        remaining, &append_token);
    incremental_token.append(append_token);
    remaining = ConsumeIdentifierCodeString(remaining, &append_token);
    incremental_token.append(append_token);

    if (!got_at_least_one_value) {
      throw LexingException(string(fname) + "Found no values.");
    } else {
      if (token != nullptr) {
        *token = incremental_token;
      }
      return remaining;
    }
  }
}

string ConsumeRealVectorValueChangeString(
    const string& input, string* token) {
  const string fname = "ConsumeRealVectorValueChange: ";
  throw LexingException(fname + "Not yet supported.");
}

string ConsumeVectorValueChangeString(
    const string& input, string* token) {
  string error_msg;
  try {
    return ConsumeBinaryVectorValueChangeString(input, token);
  } catch (LexingException& e) {
    error_msg.append(e.what());
  }
  try {
    return ConsumeRealVectorValueChangeString(input, token);
  } catch (LexingException& e) {
    throw LexingException("ConsumeVectorValueChange: " +
                          error_msg + string(" / ") + e.what());
  }
}

string ConsumeIdentifierCodeString(
    const string& input, string* token) {
  try {
    return ConsumeNonWhitespace(input, token);
  } catch (LexingException& e) {
    throw LexingException(string("ConsumeIdentifierCode: ") + e.what());
  }
}

string ConsumeExactString(
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

bool ConsumeExactString(
    const string& str, istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  bool found = true;
  for (char string_c : str) {
    if (in.eof()) {
      found = false;
      break;
    }
    char in_c = (char)in.get();
    if (in_c != string_c) {
      found = false;
      break;
    }
  }
  if (!found) {
    in.seekg(initial_pos);
  } else if (token != nullptr) {
    token->assign(str);
  }
  return found;
}

bool ConsumeExactString(
    const string& str, FILE* in, string* token) {
  const off_t initial_pos = ftello(in);
  bool found = true;
  for (char string_c : str) {
    if (feof(in)) {
      found = false;
      break;
    }
    char in_c = (char)fgetc(in);
    if (in_c != string_c) {
      found = false;
      break;
    }
  }
  if (!found) {
    fseeko(in, initial_pos, SEEK_SET);
  } else if (token != nullptr) {
    token->assign(str);
  }
  return found;
}

string ConsumeTextToEnd(
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

bool ConsumeTextToEnd(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  string consumed;
  bool found = false;
  char c = (char)in.get();
  while (EOF != c) {
    consumed.push_back(c);
    if (c == '$') {
      found = ConsumeExactString("end", in);
      if (found) {
        consumed.append("end");
        break;
      }
    }
    c = (char)in.get();
  }
  if (!found) {
    in.seekg(initial_pos);
    cout << "WARNING: ConsumeTextToEnd on full stream!\n";
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

bool ConsumeTextToEnd(
    FILE* in, string* token) {
  const off_t initial_pos = ftello(in);
  string consumed;
  bool found = false;
  char c = (char)fgetc(in);
  while (EOF != c) {
    consumed.push_back(c);
    if (c == '$') {
      found = ConsumeExactString("end", in);
      if (found) {
        consumed.append("end");
        break;
      }
    }
    c = (char)fgetc(in);
  }
  if (!found) {
    fseeko(in, initial_pos, SEEK_SET);
    cout << "WARNING: ConsumeTextToEnd on full stream!\n";
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

string ConsumeNonWhitespace(
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

bool ConsumeNonWhitespace(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  if (token != nullptr) {
    token->resize(0);
  }
  while (!in.eof() && !isspace(in.peek())) {
    char c = (char)in.get();
    if (token != nullptr) {
      token->push_back(c);
    }
  }
  return initial_pos != in.tellg();
}

bool ConsumeNonWhitespace(
    istream& in, char** buffer_pos) {
  char* initial_buffer_pos = *buffer_pos;
  while (!in.eof() && !isspace(in.peek())) {
    **buffer_pos = (char)in.get();
    ++(*buffer_pos);
  }
  return initial_buffer_pos != *buffer_pos;
}

bool ConsumeNonWhitespace(
    FILE* in, string* token) {
  char c = (char)fgetc(in);
  string consumed;
  bool found = false;
  while (!isspace(c) && c != EOF) {
    consumed.push_back(c);
    c = (char)fgetc(in);
    found = true;
  }
  if (isspace(c)) {
    ungetc(c, in);
  }
  if (found && token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

bool ConsumeNonWhitespace(
    FILE* in, char** buffer_pos) {
  char* initial_buffer_pos = *buffer_pos;
  char c = (char)fgetc(in);
  while (!isspace(c) && c != EOF) {
    **buffer_pos = c;
    ++(*buffer_pos);
    c = (char)fgetc(in);
  }
  if (isspace(c)) {
    ungetc(c, in);
  }
  return initial_buffer_pos != *buffer_pos;
}

bool ConsumeWhitespace(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  if (token != nullptr) {
    token->resize(0);
  }
  while (!in.eof() && isspace(in.peek())) {
    char c = (char)in.get();
    if (token != nullptr) {
      token->push_back(c);
    }
  }
  return initial_pos != in.tellg();
}

bool ConsumeWhitespace(FILE* in, string* token) {
  const off_t initial_pos = ftello(in);
  if (token != nullptr) {
    token->resize(0);
  }
  char c = (char)fgetc(in);
  while (isspace(c)) {
    if (token != nullptr) {
      token->push_back(c);
    }
    c = (char)fgetc(in);
  }
  if (!isspace(c) && c != EOF) {
    ungetc(c, in);
  }
  return initial_pos != ftello(in);
}

bool ConsumeWhitespace(
    istream& in, char** buffer_pos) {
  char* initial_buffer_pos = *buffer_pos;
  while (!in.eof() && isspace(in.peek())) {
    **buffer_pos = (char)in.get();
    ++(*buffer_pos);
  }
  return initial_buffer_pos != *buffer_pos;
}

bool ConsumeWhitespace(
    FILE* in, char** buffer_pos) {
  char* initial_buffer_pos = *buffer_pos;
  char c = (char)fgetc(in);
  while (isspace(c)) {
    **buffer_pos = c;
    ++(*buffer_pos);
    c = (char)fgetc(in);
  }
  if (!isspace(c) && c != EOF) {
    ungetc(c, in);
  }
  return initial_buffer_pos != *buffer_pos;
}

void ConsumeWhitespaceOptional(istream& in) {
  while (!in.eof() && isspace(in.peek())) {
    in.get();
  }
}

void ConsumeWhitespaceOptional(FILE* in) {
  char c = (char)fgetc(in);
  while (isspace(c)) {
    c = (char)fgetc(in);
  }
  if (c != EOF && !isspace(c)) {
    ungetc(c, in);
  }
}

void ConsumeWhitespaceOptional(istream& in, string* token) {
  assert(token != nullptr);
  token->resize(0);
  while (!in.eof() && isspace(in.peek())) {
    token->push_back((char)in.get());
  }
}

void ConsumeWhitespaceOptional(
    FILE* in, string* token) {
  assert(token != nullptr);
  token->resize(0);
  char c = (char)fgetc(in);
  while (isspace(c)) {
    token->push_back(c);
    c = (char)fgetc(in);
  }
  if (c != EOF && !isspace(c)) {
    ungetc(c, in);
  }
}

void ConsumeWhitespaceOptional(
    istream& in, char** buffer_pos) {
  while (!in.eof() && isspace(in.peek())) {
    **buffer_pos = (char)in.get();
    ++(*buffer_pos);
  }
}

void ConsumeWhitespaceOptional(
    FILE* in, char** buffer_pos) {
  char c = (char)fgetc(in);
  while (isspace(c)) {
    **buffer_pos = c;
    ++(*buffer_pos);
    c = (char)fgetc(in);
  }
  if (c != EOF && !isspace(c)) {
    ungetc(c, in);
  }
}

bool ConsumeDecimalNumber(istream& in, string* token) {
  std::streampos initial_pos = in.tellg();
  if (token != nullptr) {
    token->resize(0);
  }
  while (!in.eof() && isdigit(in.peek())) {
    char c = (char)in.get();
    if (token != nullptr) {
      token->push_back(c);
    }
  }
  return initial_pos != in.tellg();
}

bool ConsumeDecimalNumber(
    istream& in, char** buffer) {
  char* initial_buffer_pos = *buffer;
  while (!in.eof() && isdigit(in.peek())) {
    **buffer = (char)in.get();
    ++(*buffer);
  }
  return initial_buffer_pos != *buffer;
}

bool ConsumeDecimalNumber(FILE* in, string* token) {
  bool found = false;
  char c = (char)fgetc(in);
  while (isdigit(c)) {
    found = true;
    if (token != nullptr) {
      token->push_back(c);
    }
    c = (char)fgetc(in);
  }
  if (!isdigit(c) && c != EOF) {
    ungetc(c, in);
  }
  return found;
}

bool ConsumeDecimalNumber(
    FILE* in, char** buffer) {
  char* initial_buffer_pos = *buffer;
  char c = (char)fgetc(in);
  while (isdigit(c)) {
    **buffer = c;
    ++(*buffer);
    c = (char)fgetc(in);
  }
  if (!isdigit(c) && c != EOF) {
    ungetc(c, in);
  }
  return initial_buffer_pos != *buffer;
}

}  // vcd_lexer
