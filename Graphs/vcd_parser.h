#ifndef VCD_PARSER_H_
#define VCD_PARSER_H_

#include <istream>
#include <fstream>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vcd_lexer.h"

// Structs for parsing VCD tokens
namespace vcd_parser {

// TODO Move entropy related stuff into separate class once mature.
enum class FourValueLogic {
  ZERO,
  ONE,
  X,
  Z,
};

struct BitEntropyInfo {
  void Update(char value_char, long long time_since_last_update) {
    switch (cur_val) {
      case FourValueLogic::ZERO:
        num_0 += time_since_last_update;
        break;
      case FourValueLogic::ONE:
        num_1 += time_since_last_update;
        break;
      case FourValueLogic::X:
        num_x += time_since_last_update;
        break;
      case FourValueLogic::Z:
        num_z += time_since_last_update;
        break;
      default: assert(false);
    }
    switch (value_char) {
      case '0':
        cur_val = FourValueLogic::ZERO;
        break;
      case '1':
        cur_val = FourValueLogic::ONE;
        break;
      case 'x':
      case 'X':
        cur_val = FourValueLogic::X;
        break;
      case 'z':
      case 'Z':
        cur_val = FourValueLogic::Z;
        break;
      default: assert(false);
    }
  }

  double p_0() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_0) / double(total);
    }
  }
  double p_1() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_1) / double(total);
    }
  }
  double p_x() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_x) / double(total);
    }
  }
  double p_z() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_z) / double(total);
    }
  }

  long long int total_time() const {
    return num_0 + num_1 + num_x + num_z;
  }

  FourValueLogic cur_val{FourValueLogic::X};
  long long int num_0{0};
  long long int num_1{0};
  long long int num_x{0};
  long long int num_z{0};
};

struct SignalEntropyInfo {
  long long last_update_time{0};
  unsigned long long width;
  std::string orig_name;
  std::vector<BitEntropyInfo> bit_info;
};

namespace vcd_token {

struct IdentifierCode {
  std::string code;
};

struct VectorValueChange {
  enum class RadixType {
    BinaryNumber,
    RealNumber,
  };
  RadixType radix;
  char radix_char;
  std::string number_string;
  IdentifierCode identifier_code;
};

struct Value {
  char value;
};

struct ScalarValueChange {
  Value value;
  IdentifierCode identifier_code;
};

struct ValueChange {
  enum class ValueChangeType {
    ScalarValueChange,
    VectorValueChange
  };
  ValueChangeType type;
  ScalarValueChange scalar_value_change;
  VectorValueChange vector_value_change;
};

struct SimulationTime {
  unsigned long long time;
};

struct SimulationKeyword {
  std::string keyword;
};

struct DeclarationKeyword {
  std::string keyword;
};

struct SimulationValueCommand {
  std::string simulation_keyword;
  std::vector<ValueChange> value_changes;
};

struct Comment {
  std::string comment_text;
};

struct SimulationCommand {
  enum class SimulationCommandType {
    SimulationValueCommand,
    CommentCommand,
    TimeCommand,
    ValueChangeCommand
  };
  SimulationCommandType type;
  SimulationValueCommand simulation_value_command;
  Comment comment;
  SimulationTime simulation_time;
  ValueChange value_change;
};

struct Variable {
  enum class VariableType {
    EventType,
    IntegerType,
    ParameterType,
    RealType,
    RegType,
    Supply0Type,
    Supply1Type,
    TimeType,
    TriType,
    TriandType,
    TriorType,
    TriregType,
    Tri0Type,
    Tri1Type,
    WandType,
    WireType,
    WorType,
  };
  VariableType type;
  unsigned long long int width;
  std::string orig_name;
  std::string code_name;
};

struct DeclarationCommand {
  DeclarationKeyword declaration_keyword;
  std::string command_text;
};

struct VcdDefinitions {
  std::vector<DeclarationCommand> declaration_commands;
  std::vector<SimulationCommand> simulation_commands;
};
}  // namespace vcd_token

void ParseIdentifierCode(const std::string& token,
                         vcd_token::IdentifierCode* identifier_code,
                         bool check_token = true);
template <typename T>
bool TryParseIdentifierCodeFromInput(
    T& in, vcd_token::IdentifierCode* identifier_code);

void ParseValue(const std::string& token,
                vcd_token::Value* value,
                bool check_token = true);
template <typename T>
bool TryParseValueFromInput(T& in, vcd_token::Value* value);

void ParseVectorValueChange(const std::string& token,
                            vcd_token::VectorValueChange* vvc,
                            bool check_token = true);
template <typename T>
bool TryParseVectorValueChangeFromInput(
    T& in, vcd_token::VectorValueChange* vvc);


void ParseScalarValueChange(const std::string& token,
                            vcd_token::ScalarValueChange* svc,
                            bool check_token = true);
template <typename T>
bool TryParseScalarValueChangeFromInput(T& in, vcd_token::ScalarValueChange* svc);

void ParseValueChange(const std::string& token,
                      vcd_token::ValueChange* vc,
                      bool check_token = true);
template <typename T>
bool TryParseValueChangeFromInput(T& in, vcd_token::ValueChange* vc);

void ParseSimulationTime(const std::string& token,
                         vcd_token::SimulationTime* st,
                         bool check_token = true);
template <typename T>
bool TryParseSimulationTimeFromInput(T& in, vcd_token::SimulationTime* st);

void ParseSimulationKeyword(const std::string& token,
                            vcd_token::SimulationKeyword* sk,
                            bool check_token = true);
template <typename T>
void ParseSimulationKeywordFromInput(T& in, vcd_token::SimulationKeyword* sk);

void ParseDeclarationKeyword(const std::string& token,
                             vcd_token::DeclarationKeyword* dk,
                             bool check_token = true);
template <typename T>
bool TryParseDeclarationKeywordFromInput(
    T& in, vcd_token::DeclarationKeyword* dk);

void ParseSimulationValueCommand(const std::string& token,
                                 vcd_token::SimulationValueCommand* svc,
                                 bool check_token = true);
template <typename T>
bool TryParseSimulationValueCommandFromInput(
    T& in, vcd_token::SimulationValueCommand* svc);

void ParseComment(const std::string& token,
                  vcd_token::Comment* comment,
                  bool check_token = true);
template <typename T>
bool TryParseCommentFromInput(T& in, vcd_token::Comment* comment);

void ParseSimulationCommand(const std::string& token,
                            vcd_token::SimulationCommand* sc,
                            bool check_token = true);
template <typename T>
bool TryParseSimulationCommandFromInput(
    T& in, vcd_token::SimulationCommand* sc);

void ParseDeclarationCommand(const std::string& token,
                             vcd_token::DeclarationCommand* dc,
                             bool check_token = true);
template <typename T>
bool TryParseDeclarationCommandFromInput(
    T& in, vcd_token::DeclarationCommand* dc);

bool TryParseVarBody(const std::string& body, vcd_token::Variable* var);

template <typename T>
void ParseVcdDefinitions(T& in, vcd_token::VcdDefinitions* vd);

void CheckParseToken(
      const std::string& token, void* token_struct_ptr, bool check_token,
      std::string (*lex)(const std::string&, std::string*));

// Template function definitions
template <typename T>
bool TryParseIdentifierCodeFromInput(
    T& in, vcd_token::IdentifierCode* identifier_code) {
  return vcd_lexer::ConsumeIdentifierCode(in, &(identifier_code->code));
}

template <typename T>
bool TryParseValueFromInput(T& in, vcd_token::Value* value) {
  vcd_lexer::ConsumeWhitespaceOptional(in);
  char* c_ptr = &(value->value);
  return vcd_lexer::ConsumeValueNoWhitespace(in, &c_ptr);
}

template <typename T>
bool TryParseVectorValueChangeFromInput(
    T& in, vcd_token::VectorValueChange* vvc) {
  std::string token;
  vcd_lexer::ConsumeWhitespaceOptional(in);
  char c = fhelp::GetChar(in);
  if (c == 'b' || c == 'B') {
    vvc->radix = vcd_token::VectorValueChange::RadixType::BinaryNumber;
  } else if (c == 'r' || c == 'R') {
    vvc->radix = vcd_token::VectorValueChange::RadixType::RealNumber;
  } else {
    fhelp::UngetChar(c, in);
    return false;
  }
  vvc->radix_char = c;
  if (vcd_lexer::ConsumeNonWhitespace(in, &(vvc->number_string))) {
    // For performance reasons, we do not try to recover from consuming a
    // valid vector value but failing to consume an identifier code.
    assert(TryParseIdentifierCodeFromInput(in, &(vvc->identifier_code)));
    return true;
  } else {
    fhelp::UngetChar(c, in);
    return false;
  }
}

template <typename T>
bool TryParseScalarValueChangeFromInput(T& in, vcd_token::ScalarValueChange* svc) {
  if (TryParseValueFromInput(in, &(svc->value))) {
    if (TryParseIdentifierCodeFromInput(in, &(svc->identifier_code))) {
      return true;
    } else {
      fhelp::UngetChar(svc->value.value, in);
    }
  }
  return false;
}

template <typename T>
bool TryParseValueChangeFromInput(T& in, vcd_token::ValueChange* vc) {
  if (TryParseScalarValueChangeFromInput(in, &(vc->scalar_value_change))) {
    vc->type = vcd_token::ValueChange::ValueChangeType::ScalarValueChange;
    return true;
  } else if (TryParseVectorValueChangeFromInput(
      in, &(vc->vector_value_change))) {
    vc->type = vcd_token::ValueChange::ValueChangeType::VectorValueChange;
    return true;
  }
  return false;
}

template <typename T>
bool TryParseSimulationTimeFromInput(T& in, vcd_token::SimulationTime* st) {
  vcd_lexer::ConsumeWhitespaceOptional(in);
  char* const initial_buffer_pos = vcd_lexer::global_time_buffer_.get();
  char* buffer_pos = initial_buffer_pos;
  char c = fhelp::GetChar(in);
  if ('#' == c) {
    if (vcd_lexer::ConsumeDecimalNumber(in, &buffer_pos)) {
      *buffer_pos = '\0';
      st->time = strtoull(initial_buffer_pos, nullptr, 10);
      return true;
    }
  }
  fhelp::UngetChar(c, in);
  return false;
}

template <typename T>
void ParseSimulationKeywordFromInput(T& in, vcd_token::SimulationKeyword* sk) {
  assert(vcd_lexer::ConsumeSimulationKeyword(in, &(sk->keyword)));
}

template <typename T>
bool TryParseDeclarationKeywordFromInput(
    T& in, vcd_token::DeclarationKeyword* dk) {
  return vcd_lexer::ConsumeDeclarationKeyword(in, &(dk->keyword));
}

template <typename T>
bool TryParseSimulationValueCommandFromInput(
    T& in, vcd_token::SimulationValueCommand* svc) {
  if (vcd_lexer::ConsumeSimulationKeyword(in, &(svc->simulation_keyword))) {
    vcd_token::ValueChange vc;
    while (TryParseValueChangeFromInput(in, &vc)) {
      svc->value_changes.push_back(vc);
    }
    vcd_lexer::ConsumeWhitespaceOptional(in);
    if (!vcd_lexer::ConsumeExactString("$end", in) && !fhelp::IsEof(in)) {
      std::cout << svc->simulation_keyword << std::endl;
      std::cout << fhelp::PeekNextLine(in) << std::endl;
      assert(false);
    }
    return true;
  } else {
    return false;
  }
}

template <typename T>
bool TryParseCommentFromInput(T& in, vcd_token::Comment* comment) {
  return vcd_lexer::ConsumeComment(in, &(comment->comment_text));
}

template <typename T>
bool TryParseSimulationCommandFromInput(T& in, vcd_token::SimulationCommand* sc) {
  if (TryParseValueChangeFromInput(in, &(sc->value_change))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::ValueChangeCommand;
  } else if (TryParseSimulationTimeFromInput(in, &(sc->simulation_time))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::TimeCommand;
  } else if (TryParseSimulationValueCommandFromInput(
      in, &(sc->simulation_value_command))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::SimulationValueCommand;
  } else if (TryParseCommentFromInput(in, &(sc->comment))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::CommentCommand;
  } else {
    return false;
  }
  return true;
}

template <typename T>
bool TryParseDeclarationCommandFromInput(
    T& in, vcd_token::DeclarationCommand* dc) {
  if (TryParseDeclarationKeywordFromInput(in, &(dc->declaration_keyword))) {
    assert(vcd_lexer::ConsumeTextToEnd(in, &(dc->command_text)));
    return true;
  }
  return false;
}

template <typename T>
void ParseVcdDefinitions(T& in, vcd_token::VcdDefinitions* vd) {
  const auto initial_pos = fhelp::GetPosition(in);
  fhelp::SeekToEnd(in);
  const auto end_pos = fhelp::GetPosition(in);
  fhelp::SeekToPosition(in, initial_pos);

  long long last_print_point = 0;
  long long cur_line = 0;
  bool definitions_done = false;
  std::cout << "---------------Starting parse definitions--------------\n";
  vcd_token::DeclarationCommand dc;
  while (!fhelp::IsEof(in) && !definitions_done) {
    assert(TryParseDeclarationCommandFromInput(in, &dc));
    vd->declaration_commands.push_back(dc);
    if (dc.declaration_keyword.keyword == "$enddefinitions") {
      definitions_done = true;
    }
    ++cur_line;
    if (cur_line - last_print_point > 100000) {
      std::cout << "Parsed  " << fhelp::GetPosition(in) << " of "
                << end_pos << " bytes." << std::endl;
      last_print_point = cur_line;
    }
  }
  std::cout << "---------------Starting parse sim commands----------------\n";
  while (!fhelp::IsEof(in)) {
    vd->simulation_commands.emplace_back();
    TryParseSimulationCommandFromInput(in, &(vd->simulation_commands.back()));
    ++cur_line;
    if (cur_line - last_print_point > 100000) {
      std::cout << "Parsed  " << fhelp::GetPosition(in) << " of "
                << end_pos << " bytes." << std::endl;
      last_print_point = cur_line;
    }
    vcd_lexer::ConsumeWhitespaceOptional(in);
  }
  std::cout << "---------------Done parsing----------------\n";
}

inline void ProcessValueChange(
    const vcd_token::ValueChange& vc, long long cur_time,
    std::unordered_map<std::string, SignalEntropyInfo>& entropy_data,
    const std::unordered_set<std::string>& ignored_codes) {
  if (vc.type ==
      vcd_token::ValueChange::ValueChangeType::ScalarValueChange) {
    if (ignored_codes.find(vc.scalar_value_change.identifier_code.code) !=
        ignored_codes.end()) {
      return;
    }
    SignalEntropyInfo& sig_info = entropy_data.at(
        vc.scalar_value_change.identifier_code.code);
    long long time_diff = cur_time - sig_info.last_update_time;
    sig_info.last_update_time = cur_time;
    assert(sig_info.width == 1);
    sig_info.bit_info.at(0).Update(
        vc.scalar_value_change.value.value, time_diff);
  } else if (vc.type ==
              vcd_token::ValueChange::ValueChangeType::VectorValueChange){
    if (ignored_codes.find(vc.vector_value_change.identifier_code.code) !=
        ignored_codes.end()) {
      return;
    }
    const vcd_token::VectorValueChange& vvc = vc.vector_value_change;
    assert(vvc.radix ==
            vcd_token::VectorValueChange::RadixType::BinaryNumber);
    SignalEntropyInfo& sig_info =
        entropy_data.at(vvc.identifier_code.code);
    long long time_diff = cur_time - sig_info.last_update_time;
    sig_info.last_update_time = cur_time;
    assert(sig_info.width > 1);
    // According to VCD spec, extend with 0 unless X or Z.
    char extend_char;
    switch (vvc.number_string.at(0)) {
      case 'x':
      case 'X':
        extend_char = 'X';
        break;
      case 'z':
      case 'Z':
        extend_char = 'Z';
        break;
      default:
        extend_char = '0';
        break;
    }
    // Need to index in reverse order since char 0 of string is msb.
    for (size_t i = 0; i < vvc.number_string.length(); ++i) {
      sig_info.bit_info.at(i).Update(
          vvc.number_string.at(vvc.number_string.length() - i - 1),
          time_diff);
    }
    for (size_t i = vvc.number_string.length(); i < sig_info.width; ++i) {
      sig_info.bit_info.at(i).Update(extend_char, time_diff);
    }
  }
}

template <typename T>
void EntropyFromVcdDefinitions(T& in, std::ostream& os) {
  const auto initial_pos = fhelp::GetPosition(in);
  fhelp::SeekToEnd(in);
  const auto end_pos = fhelp::GetPosition(in);
  fhelp::SeekToPosition(in, initial_pos);

  std::unordered_map<std::string, SignalEntropyInfo> entropy_data;
  std::unordered_set<std::string> ignored_codes;

  long long last_print_point = 0;
  long long cur_line = 0;
  bool definitions_done = false;
  std::cout << "---------------Starting parse definitions--------------\n";
  vcd_token::DeclarationCommand dc;
  vcd_token::Variable var;
  while (!fhelp::IsEof(in) && !definitions_done) {
    assert(TryParseDeclarationCommandFromInput(in, &dc));
    if (dc.declaration_keyword.keyword == "$enddefinitions") {
      definitions_done = true;
    } else if (dc.declaration_keyword.keyword == "$var") {
      if (!TryParseVarBody(dc.command_text, &var)) {
        std::cout << dc.declaration_keyword.keyword << std::endl;
        std::cout << dc.command_text << std::endl;
        assert(false);
      }
      SignalEntropyInfo e_info;
      switch (var.type) {
        // Net types
        case vcd_token::Variable::VariableType::WireType:
        case vcd_token::Variable::VariableType::RegType:
        case vcd_token::Variable::VariableType::Supply0Type:
        case vcd_token::Variable::VariableType::Supply1Type:
        case vcd_token::Variable::VariableType::TriType:
        case vcd_token::Variable::VariableType::Tri0Type:
        case vcd_token::Variable::VariableType::Tri1Type:
        case vcd_token::Variable::VariableType::TriandType:
        case vcd_token::Variable::VariableType::TriorType:
        case vcd_token::Variable::VariableType::TriregType:
        case vcd_token::Variable::VariableType::WandType:
        case vcd_token::Variable::VariableType::WorType:
          e_info.orig_name = var.orig_name;
          e_info.width = var.width;
          e_info.bit_info.resize(e_info.width);
          entropy_data.insert(make_pair(var.code_name, e_info));
          break;
        // Non-net recognized types
        case vcd_token::Variable::VariableType::EventType:
        case vcd_token::Variable::VariableType::IntegerType:
        case vcd_token::Variable::VariableType::ParameterType:
        case vcd_token::Variable::VariableType::RealType:
        case vcd_token::Variable::VariableType::TimeType:
          ignored_codes.insert(var.code_name);
          break;
        default:
          assert(false);
          break;
      }
    }
    ++cur_line;
    if (cur_line - last_print_point > 100000) {
      std::cout << "Parsed  "
                << (fhelp::GetPosition(in) >> 20) << " of "
                << (end_pos >> 20) << " MB." << std::endl;
      last_print_point = cur_line;
    }
  }
  std::cout << "Tracking entropy for " << entropy_data.size() << " signals\n";
  std::cout << "---------------Starting parse sim commands----------------\n";
  vcd_token::SimulationCommand sc;
  long long cur_time = 0;
  while (!fhelp::IsEof(in)) {
    if (!TryParseSimulationCommandFromInput(in, &sc)) {
      std::cout << "Done processing simulations" << std::endl;
      std::cout << fhelp::PeekNextLine(in);
      break;
    }
    switch (sc.type) {
      case vcd_token::SimulationCommand::SimulationCommandType::TimeCommand:
        cur_time = sc.simulation_time.time;
        break;
      case vcd_token::SimulationCommand::SimulationCommandType::ValueChangeCommand:
        ProcessValueChange(sc.value_change, cur_time, entropy_data, ignored_codes);
        break;
      case vcd_token::SimulationCommand::SimulationCommandType::SimulationValueCommand:
        for (const vcd_token::ValueChange& vc :
             sc.simulation_value_command.value_changes) {
          ProcessValueChange(vc, cur_time, entropy_data, ignored_codes);
        }
        break;
      default: break;
    }
    ++cur_line;
    if (cur_line - last_print_point > 10000000ULL) {
      std::cout << "Parsed  "
                << (fhelp::GetPosition(in) >> 20) << " of "
                << (end_pos >> 20) << " MB." << std::endl;
      last_print_point = cur_line;
    }
    vcd_lexer::ConsumeWhitespaceOptional(in);
  }

  // Update all signals with last value.
  for (auto entropy_pair : entropy_data) {
    long long time_diff = cur_time - entropy_pair.second.last_update_time;
    for (int i = 0; i < entropy_pair.second.width; ++i) {
      entropy_pair.second.bit_info.at(i).Update('0', time_diff);
    }
  }

  std::cout << "---------------Done parsing----------------\n";
  std::cout << "End time: " << cur_time << std::endl;
  for (auto entropy_pair : entropy_data) {
    SignalEntropyInfo& sig_info = entropy_pair.second;
    for (int i = 0; i < sig_info.width; ++i) {
      BitEntropyInfo& bit_info = sig_info.bit_info.at(i);
      os << sig_info.orig_name << "[" << i << "] "
         << bit_info.p_0() << " "
         << bit_info.p_1() << " "
         << bit_info.p_x() << " "
         << bit_info.p_z() << " "
         << "(" << bit_info.total_time() << ") "
         << std::endl;
    }
  }
}

}  // namespace vcd_parser

#endif /* VCD_PARSER_H_ */
