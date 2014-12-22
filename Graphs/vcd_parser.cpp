#include "vcd_parser.h"

#include <stdexcept>
#include <utility>

#include "universal_macros.h"

using namespace std;

namespace vcd_parser {
using namespace vcd_lexer;

void ParseIdentifierCode(
    const string& token,
    vcd_token::IdentifierCode* identifier_code,
    bool check_token) {
  CheckParseToken(token, identifier_code, check_token,
                  ConsumeIdentifierCodeString);
  identifier_code->code = token;
}

void ParseValue(
    const string& token,
    vcd_token::Value* value,
    bool check_token) {
  CheckParseToken(token, value, check_token,
                  ConsumeValueNoWhitespaceString);
  value->value = token.at(0);
}

void ParseVectorValueChange(
    const string& token,
    vcd_token::VectorValueChange* vvc,
    bool check_token) {
  CheckParseToken(token, vvc, check_token, ConsumeVectorValueChangeString);
  string remaining;
  switch (token.at(0)) {
    case 'b': // Fall-through intended
    case 'B':
      vvc->radix = vcd_token::VectorValueChange::RadixType::BinaryNumber;
      break;
    case 'r': // Fall-through intended
    case 'R': // Fall-through intended
      vvc->radix = vcd_token::VectorValueChange::RadixType::RealNumber;
      break;
    default: throw std::invalid_argument("Invalid Radix\n");
  }
  vvc->radix_char = token.at(0);
  remaining = ConsumeNonWhitespace(token.substr(1, string::npos),
                                             &(vvc->number_string));
  string id_code_token;
  ConsumeIdentifierCodeString(remaining, &id_code_token);
  ParseIdentifierCode(id_code_token, &(vvc->identifier_code), check_token);
}

void ParseScalarValueChange(
    const string& token,
    vcd_token::ScalarValueChange* svc,
    bool check_token) {
  CheckParseToken(token, svc, check_token, ConsumeScalarValueChangeString);
  ParseValue(token.substr(0, 1), &(svc->value), check_token);
  ParseIdentifierCode(token.substr(1, string::npos), &(svc->identifier_code),
                      check_token);
}

void ParseValueChange(
    const string& token,
    vcd_token::ValueChange* vc,
    bool check_token) {
  CheckParseToken(token, vc, check_token, ConsumeValueChangeString);
  try {
    ConsumeScalarValueChangeString(token, nullptr);
    vc->type = vcd_token::ValueChange::ValueChangeType::ScalarValueChange;
    ParseScalarValueChange(token, &(vc->scalar_value_change), check_token);
  } catch (std::exception& e) {
    vc->type = vcd_token::ValueChange::ValueChangeType::VectorValueChange;
    ParseVectorValueChange(token, &(vc->vector_value_change), check_token);
  }
}

void ParseSimulationTime(
    const string& token,
    vcd_token::SimulationTime* st,
    bool check_token) {
  CheckParseToken(token, st, check_token, ConsumeSimulationTimeString);
  st->time = strtoull(token.substr(1, string::npos).c_str(), nullptr, 10);
}

void ParseSimulationKeyword(
    const string& token,
    vcd_token::SimulationKeyword* sk,
    bool check_token) {
  CheckParseToken(token, sk, check_token, ConsumeSimulationKeywordString);
  sk->keyword = token;
}

void ParseDeclarationKeyword(
    const string& token,
    vcd_token::DeclarationKeyword* dk,
    bool check_token) {
  CheckParseToken(token, dk, check_token, ConsumeDeclarationKeywordString);
  dk->keyword = token;
}

void ParseSimulationValueCommand(
    const string& token,
    vcd_token::SimulationValueCommand* svc,
    bool check_token) {
  CheckParseToken(token, svc, check_token,
                  ConsumeSimulationValueCommandString);
  string cur_token;
  string remaining = ConsumeSimulationKeywordString(token, &cur_token);
  svc->simulation_keyword = cur_token;
  while (true) {
    try {
      remaining = ConsumeValueChangeString(remaining, &cur_token);
      vcd_token::ValueChange vc;
      ParseValueChange(cur_token, &vc, check_token);
      svc->value_changes.push_back(vc);
    } catch (std::exception& e) {
      return;
    }
  }
}

void ParseComment(
    const string& token,
    vcd_token::Comment* comment,
    bool check_token) {
  CheckParseToken(token, comment, check_token, ConsumeCommentString);
  comment->comment_text = token;
}

void ParseSimulationCommand(
    const string& token,
    vcd_token::SimulationCommand* sc,
    bool check_token) {
  CheckParseToken(token, sc, check_token, ConsumeSimulationCommandString);
  try {
    ConsumeSimulationValueCommandString(token, nullptr);
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::SimulationValueCommand;
    ParseSimulationValueCommand(
        token, &(sc->simulation_value_command), check_token);
    return;
  } catch (std::exception& e) {}
  try {
    ConsumeCommentString(token, nullptr);
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::CommentCommand;
    ParseComment(token, &(sc->comment), check_token);
    return;
  } catch (std::exception& e) {}
  try {
    ConsumeSimulationTimeString(token, nullptr);
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::TimeCommand;
    ParseSimulationTime(token, &(sc->simulation_time), check_token);
    return;
  } catch (std::exception& e) {}
  try {
    ConsumeValueChangeString(token, nullptr);
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::ValueChangeCommand;
    ParseValueChange(token, &(sc->value_change), check_token);
    return;
  } catch (std::exception& e) {}
  throw std::invalid_argument("Invalid SimulationCommand\n");
}

void ParseDeclarationCommand(
    const string& token,
    vcd_token::DeclarationCommand* dc,
    bool check_token) {
  CheckParseToken(token, dc, check_token, ConsumeDeclarationCommandString);
  string cur_token;
  string remaining = ConsumeDeclarationKeywordString(token, &cur_token);
  ParseDeclarationKeyword(cur_token, &(dc->declaration_keyword), check_token);
  dc->command_text = ConsumeTextToEnd(remaining, &cur_token);
}

bool TryParseVarBody(const string& body, vcd_token::Variable* var) {
  std::string remaining = ConsumeWhitespaceOptional(body);
  string type_string;
  remaining = vcd_lexer::ConsumeNonWhitespace(remaining, &type_string);
  if (type_string == "wire") {
    var->type = vcd_token::Variable::VariableType::WireType;
  } else if (type_string == "reg") {
    var->type = vcd_token::Variable::VariableType::RegType;
  } else if (type_string == "event") {
    var->type = vcd_token::Variable::VariableType::EventType;
  } else if (type_string == "integer") {
    var->type = vcd_token::Variable::VariableType::IntegerType;
  } else if (type_string == "parameter") {
    var->type = vcd_token::Variable::VariableType::ParameterType;
  } else if (type_string == "real") {
    var->type = vcd_token::Variable::VariableType::RealType;
  } else if (type_string == "supply0") {
    var->type = vcd_token::Variable::VariableType::Supply0Type;
  } else if (type_string == "supply1") {
    var->type = vcd_token::Variable::VariableType::Supply1Type;
  } else if (type_string == "time") {
    var->type = vcd_token::Variable::VariableType::TimeType;
  } else if (type_string == "tri") {
    var->type = vcd_token::Variable::VariableType::TriType;
  } else if (type_string == "triand") {
    var->type = vcd_token::Variable::VariableType::TriandType;
  } else if (type_string == "trior") {
    var->type = vcd_token::Variable::VariableType::TriorType;
  } else if (type_string == "tri0") {
    var->type = vcd_token::Variable::VariableType::Tri0Type;
  } else if (type_string == "tri1") {
    var->type = vcd_token::Variable::VariableType::Tri1Type;
  } else if (type_string == "wand") {
    var->type = vcd_token::Variable::VariableType::WandType;
  } else if (type_string == "wor") {
    var->type = vcd_token::Variable::VariableType::WorType;
  } else {
    cout << type_string << endl;
    assert(false);
  }
  try {
    remaining = ConsumeWhitespaceOptional(remaining);
    string width_string;
    remaining = ConsumeDecimalNumber(remaining, &width_string);
    var->width = strtoull(width_string.c_str(), nullptr, 10);
    remaining = ConsumeWhitespaceOptional(remaining);
    remaining = ConsumeNonWhitespace(remaining, &(var->code_name));
    remaining = ConsumeWhitespaceOptional(remaining);
    remaining = ConsumeNonWhitespace(remaining, &(var->orig_name));
    remaining = ConsumeWhitespaceOptional(remaining);
    try {
      ConsumeExactString("$end", remaining);
      return true;
    } catch (std::exception& e) {
      string bit_range;
      remaining = ConsumeNonWhitespace(remaining, &bit_range);
      var->orig_name.append(bit_range);
      remaining = ConsumeWhitespaceOptional(remaining);
    }
    ConsumeExactString("$end", remaining);
    return true;
  } catch (std::exception& e) {
    return false;
  }
}

void ParseVcdDefinitions(
    const string& token,
    vcd_token::VcdDefinitions* vd,
    bool check_token) {
  cout << "Check parse\n";
  CheckParseToken(token, vd, check_token, ConsumeVcdDefinitionsString);
  string remaining = token;
  string cur_token;
  bool definitions_done = false;
  int num_definitions = 0;
  cout << "Starting definitions\n";
  while (!definitions_done) {
    remaining = ConsumeDeclarationCommandString(remaining, &cur_token);
    vcd_token::DeclarationCommand dc;
    ParseDeclarationCommand(cur_token, &dc, check_token);
    vd->declaration_commands.push_back(dc);
    if (dc.declaration_keyword.keyword == "$enddefinitions") {
      definitions_done = true;
    }
    if (remaining.empty()) {
      throw std::exception();
    }
    ++num_definitions;
    if (num_definitions % 50 == 0) {
      cout << num_definitions << " definitions\n";
    }
  }
  int num_sim_commands = 0;
  cout << "Starting sim commands\n";
  while (!remaining.empty()) {
    remaining = ConsumeSimulationCommandString(remaining, &cur_token);
    vcd_token::SimulationCommand sc;
    ParseSimulationCommand(cur_token, &sc, check_token);
    vd->simulation_commands.push_back(sc);
    ++num_sim_commands;
    if (num_sim_commands % 1000 == 0) {
      cout << "Processed " << num_sim_commands << " sim commands. "
           << remaining.size() << " bytes left to process" << endl;
    }
  }
}

void CheckParseToken(
    const string& token, void* token_struct_ptr, bool check_token,
    string (*lex)(const string&, string*)) {
  if (token_struct_ptr == nullptr) {
    throw std::invalid_argument("Null pointer\n");
  }
  if (check_token) {
    string post_lex_token;
    string remaining = lex(token, &post_lex_token);
    if (!remaining.empty() || post_lex_token != token) {
      throw std::invalid_argument("Failed to parse token\n");
    }
  }
}

}  // namespace vcd_parser
