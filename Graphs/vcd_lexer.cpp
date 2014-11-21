/*
 * vcd_lexer.cpp
 *
 *  Created on: Nov 14, 2014
 *      Author: gregerso
 */


#include "vcd_lexer.h"

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
      remaining = ConsumeDeclarationCommand(remaining, &temp_token);
      incremental_token.append(temp_token);
    } catch (LexingException& e) {
      declarations_done = true;
    }
  }

  bool simulations_done = false;
  while (!simulations_done) {
    try {
      remaining = ConsumeSimulationCommand(remaining, &temp_token);
      incremental_token.append(temp_token);
    } catch (LexingException& e) {
      simulations_done = true;
    }
  }

  if (token != nullptr) {
    *token = incremental_token;
  }
  return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(remaining);
}

string VcdLexer::ConsumeDeclarationCommand(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    remaining = ConsumeDeclarationKeyword(remaining, &temp_token);
    incremental_token.append(temp_token);
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

string VcdLexer::ConsumeSimulationCommand(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeSimulationValueCommand(remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeComment(remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeSimulationTime(remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeValueChange(remaining, token));
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeSimulationCommand: ");
  }
}

string VcdLexer::ConsumeSimulationValueCommand(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
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
      remaining = ConsumeValueChange(remaining, &temp_token);
      incremental_token.append(" " + temp_token);
    } catch (LexingException& e) {
      done = true;
    }
  }
  remaining = ConsumeExactString("$end", remaining, &temp_token);
  incremental_token.append(" " + temp_token);
  if (token != nullptr) {
    *token = incremental_token;
  }
  return remaining;
}

string VcdLexer::ConsumeComment(
    const string& input, string* token) {
  string incremental_token;
  string temp_token;
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$comment", remaining, &temp_token));
    incremental_token.append(temp_token);
    remaining = ConsumeTextToEnd(remaining, &temp_token);
    incremental_token.append(temp_token);
    if (token != nullptr) {
      *token = temp_token;
    }
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(remaining);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeComment: ");
  }
}

string VcdLexer::ConsumeDeclarationKeyword(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$comment", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$date", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$enddefinitions", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$scope", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$timescale", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$upscope", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$var", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$version", remaining, token));
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeDeclarationKeyword: ");
  }
}

string VcdLexer::ConsumeSimulationKeyword(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$dumpall", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$dumpoff", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$dumpon", remaining, token));
  } catch (LexingException& e) {}
  try {
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
        ConsumeExactString("$dumpvars", remaining, token));
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeSimulationKeyword: ");
  }
}

string VcdLexer::ConsumeSimulationTime(
    const string& input, string* token) {
  string remaining = StructuralNetlistLexer::ConsumeWhitespaceIfPresent(input);
  if (remaining.size() < 2 || remaining[0] != '#') {
    throw LexingException("ConsumeSimulationTime: ");
  } else {
    try {
      string temp_token;
          StructuralNetlistLexer::ConsumeUnbasedImmediate(
              remaining.substr(1, string::npos), &temp_token);
      if (token != nullptr) {
        *token = "#" + temp_token;
      }
      return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(remaining);
    } catch (LexingException& e) {
      throw LexingException(string(e.what()) + "ConsumeSimulationTime: ");
    }
  }
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
    remaining =
        StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
            remaining, &temp_token);
    incremental_token.append(temp_token);
    remaining = ConsumeIdentifierCode(remaining, &temp_token);
    incremental_token.append(temp_token);
    if (token != nullptr) {
      *token = incremental_token;
    }
    return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(remaining);
  } catch (LexingException& e) {
    throw LexingException(string(e.what()) + "ConsumeScalarValue");
  }
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
    string incremental_token = string(1, remaining[0]);
    string append_token;
    while (!done) {
      try {
        remaining = ConsumeValueNoWhitespace(remaining, &append_token);
        incremental_token.append(append_token);
      } catch (LexingException& e) {
        done = true;
      }
    }
    if (incremental_token.size() <= 1) {
      throw LexingException(fname);
    } else {
      if (token != nullptr) {
        *token = incremental_token;
      }
      return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(remaining);
    }
  }
}

string VcdLexer::ConsumeRealVectorValueChange(
    const string& input, string* token) {
  const string fname = "ConsumeRealVectorValueChange: ";
  throw LexingException(fname + "Not yet supported.");
}

string VcdLexer::ConsumeVectorValueChange(
    const string& input, string* token) {
  const string fname = "ConsumeVectorValueChange: ";
  try {
    ConsumeBinaryVectorValueChange(input, token);
  } catch (LexingException& e) {}
  try {
    ConsumeRealVectorValueChange(input, token);
  } catch (LexingException& e) {}
  throw LexingException(fname);
}

string VcdLexer::ConsumeIdentifierCode(
    const string& input, string* token) {
  try {
    return ConsumeNonWhitespace(input, token);
  } catch (LexingException& e) {
    throw LexingException(string("ConsumeIdentifierCode: ") + e.what());
  }
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
  size_t end_pos = 0;
  while (end_pos < remaining.length() && !isspace(remaining[end_pos])) {
    ++end_pos;
  }
  if (token != nullptr) {
    *token = remaining.substr(0, end_pos);
  }
  return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(
      remaining.substr(end_pos, string::npos));
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
        return StructuralNetlistLexer::ConsumeWhitespaceIfPresent(remaining);
      }
    } catch (LexingException& e) {
      throw LexingException(string(e.what()) + "ConsumeTextToEnd: ");
    }
  }
  return "";  // Should never occur; added to silence compiler warning.
}
