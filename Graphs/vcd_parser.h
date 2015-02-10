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

#include "signal_entropy_info.h"
#include "vcd_lexer.h"

// Structs for parsing VCD tokens
namespace vcd_parser {
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

struct ScopeDeclaration {
  enum class ScopeType {
    BeginType,
    ForkType,
    FunctionType,
    ModuleType,
    TaskType,
  };
  ScopeType type;
  std::string identifier;
};

struct VariableDeclaration {
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
  // Must be signed, because negative indicies are allowed.
  long long int bit_low{0};
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

bool TryParseVarBody(
    const std::string& body, vcd_token::VariableDeclaration* var);

bool TryParseScopeBody(
    const std::string& body, vcd_token::ScopeDeclaration* sd);

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
    if (vcd_lexer::ConsumeUnsignedDecimalNumber(in, &buffer_pos)) {
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

template <typename T> void ProcessValueChange(
    const vcd_token::ValueChange& vc, long long cur_time, T& entropy_data,
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
    sig_info.CurrentTimeSlice().bit_info.at(0).Update(
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
      sig_info.CurrentTimeSlice().bit_info.at(i).Update(
          vvc.number_string.at(vvc.number_string.length() - i - 1),
          time_diff);
    }
    for (size_t i = vvc.number_string.length(); i < sig_info.width; ++i) {
      sig_info.CurrentTimeSlice().bit_info.at(i).Update(extend_char, time_diff);
    }
  }
}

inline void UpdateDownScope(std::vector<std::string>& cur_scope,
                            const std::string& new_scope) {
  cur_scope.push_back(new_scope);
}

inline void UpdateUpScope(std::vector<std::string>& cur_scope) {
  assert(!cur_scope.empty());
  cur_scope.pop_back();
}

template <typename T>
void UpdateAllSignals(long long current_time, T& entropy_data) {
  for (auto& entropy_pair : entropy_data) {
    long long time_diff = current_time - entropy_pair.second.last_update_time;
    for (size_t i = 0; i < entropy_pair.second.width; ++i) {
      BitEntropyInfo& bit =
          entropy_pair.second.CurrentTimeSlice().bit_info.at(i);
      bit.Update(bit.cur_val_char, time_diff);
    }
    entropy_pair.second.last_update_time = current_time;
  }
}

template <typename T, typename U>
void EntropyFromVcdDefinitions(
    T& in, U* entropy_data, bool write_to_output, std::ostream& os,
    long long max_mb, long long int interval_pico) {
  const auto initial_pos = fhelp::GetPosition(in);
  fhelp::SeekToEnd(in);
  const auto end_pos = fhelp::GetPosition(in);
  const auto end_pos_mb = end_pos >> 20;
  fhelp::SeekToPosition(in, initial_pos);

  const size_t omit_scope_levels = 2;
  const size_t max_depth = -1;

  std::unordered_set<std::string> ignored_codes;

  std::set<std::string> used_scope_prefixes;
  std::vector<std::string> scope_prefixes_by_code;

  long long last_print_point = 0;
  long long cur_line = 0;
  bool definitions_done = false;
  std::cout << "---------------Starting parse definitions--------------\n";
  vcd_token::DeclarationCommand dc;
  vcd_token::VariableDeclaration var;
  std::vector<std::string> cur_scope;
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
        case vcd_token::VariableDeclaration::VariableType::WireType:
        case vcd_token::VariableDeclaration::VariableType::RegType:
        case vcd_token::VariableDeclaration::VariableType::Supply0Type:
        case vcd_token::VariableDeclaration::VariableType::Supply1Type:
        case vcd_token::VariableDeclaration::VariableType::TriType:
        case vcd_token::VariableDeclaration::VariableType::Tri0Type:
        case vcd_token::VariableDeclaration::VariableType::Tri1Type:
        case vcd_token::VariableDeclaration::VariableType::TriandType:
        case vcd_token::VariableDeclaration::VariableType::TriorType:
        case vcd_token::VariableDeclaration::VariableType::TriregType:
        case vcd_token::VariableDeclaration::VariableType::WandType:
        case vcd_token::VariableDeclaration::VariableType::WorType:
          // Do not track entropy for signals exceeding max depth.
          if (max_depth < 0 || cur_scope.size() <= max_depth) {
            e_info.unscoped_orig_name.clear();
            std::string scope_prefix;
            for (size_t scope_level = omit_scope_levels;
                scope_level < cur_scope.size(); ++scope_level) {
              // TODO
              // Experimenting with using '/' here. Used to be '.'
              scope_prefix.append(cur_scope[scope_level] + "/");
            }
            if (used_scope_prefixes.find(scope_prefix) ==
                used_scope_prefixes.end()) {
              e_info.scope_prefix_code = scope_prefixes_by_code.size();
              scope_prefixes_by_code.push_back(scope_prefix);
            }
            e_info.unscoped_orig_name = var.orig_name;
            e_info.width = var.width;
            e_info.bit_low = var.bit_low;
            e_info.CurrentTimeSlice().bit_info.resize(e_info.width);
            entropy_data->insert(make_pair(var.code_name, e_info));
            break;
          }
        // Fall-through intended
        // Non-net recognized types
        case vcd_token::VariableDeclaration::VariableType::EventType:
        case vcd_token::VariableDeclaration::VariableType::IntegerType:
        case vcd_token::VariableDeclaration::VariableType::ParameterType:
        case vcd_token::VariableDeclaration::VariableType::RealType:
        case vcd_token::VariableDeclaration::VariableType::TimeType:
          ignored_codes.insert(var.code_name);
          break;
        default:
          assert(false);
          break;
      }
    } else if (dc.declaration_keyword.keyword == "$scope") {
      vcd_token::ScopeDeclaration sd;
      assert(TryParseScopeBody(dc.command_text, &sd));
      UpdateDownScope(cur_scope, sd.identifier);
    } else if (dc.declaration_keyword.keyword == "$upscope") {
      UpdateUpScope(cur_scope);
    }
    ++cur_line;
    if (cur_line - last_print_point > 100000) {
      std::cout << "Parsed  "
                << (fhelp::GetPosition(in) >> 20) << " of "
                << end_pos_mb << " MB." << std::endl;
      last_print_point = cur_line;
    }
  }
  std::cout << "Tracking entropy for " << entropy_data->size() << " signals\n";
  std::cout << "---------------Starting parse sim commands----------------\n";
  vcd_token::SimulationCommand sc;
  long long cur_time = 0;
  long long timeslice_switch_time = interval_pico;
  while (!fhelp::IsEof(in)) {
    if (!TryParseSimulationCommandFromInput(in, &sc)) {
      std::cout << "Done processing simulations" << std::endl;
      std::cout << fhelp::PeekNextLine(in);
      break;
    }
    switch (sc.type) {
      case vcd_token::SimulationCommand::SimulationCommandType::TimeCommand:
        if (timeslice_switch_time > 0 &&
            sc.simulation_time.time > timeslice_switch_time) {
          UpdateAllSignals(timeslice_switch_time, *entropy_data);
          for (auto& entropy_pair : *entropy_data) {
            entropy_pair.second.AdvanceTimeSlice(timeslice_switch_time);
          }
          timeslice_switch_time += interval_pico;
        }
        cur_time = sc.simulation_time.time;
        break;
      case vcd_token::SimulationCommand::SimulationCommandType::ValueChangeCommand:
        ProcessValueChange(sc.value_change, cur_time, *entropy_data, ignored_codes);
        break;
      case vcd_token::SimulationCommand::SimulationCommandType::SimulationValueCommand:
        for (const vcd_token::ValueChange& vc :
             sc.simulation_value_command.value_changes) {
          ProcessValueChange(vc, cur_time, *entropy_data, ignored_codes);
        }
        break;
      default: break;
    }
    ++cur_line;
    if (cur_line - last_print_point > 10000000ULL) {
      long long pos_mb = fhelp::GetPosition(in) >> 20;
      std::cout << "Parsed  " << pos_mb << " of "
                << end_pos_mb << " MB." << std::endl;
      last_print_point = cur_line;
      if (max_mb > 0 && pos_mb >= max_mb) {
        std::cout << "Finishing early due to file size\n";
        break;
      }
    }
    vcd_lexer::ConsumeWhitespaceOptional(in);
  }

  std::cout << "---------------Done parsing----------------\n";
  std::cout << "End time: " << cur_time << std::endl;

  // Update all signals with last value.
  UpdateAllSignals(cur_time, *entropy_data);

  assert(!entropy_data->empty());
  if (interval_pico > 0) {
    // For backwards compatibility with previous entropy files, only print
    // the number of slices in multi-slice entropy files.
    os << entropy_data->begin()->second.time_slices.size() << std::endl;
  }
  for (auto& entropy_pair : *entropy_data) {
    assert(entropy_pair.second.last_update_time != 0);
    SignalEntropyInfo& sig_info = entropy_pair.second;
    sig_info.CurrentTimeSlice().end_time = cur_time;
  }
  if (write_to_output) {
    std::cout << "---------------Writing Output----------------\n";
    for (auto& entropy_pair : *entropy_data) {
      SignalEntropyInfo& sig_info = entropy_pair.second;
      for (int i = 0; i < sig_info.width; ++i) {
        if (sig_info.scope_prefix_code >= 0) {
          os << scope_prefixes_by_code.at(sig_info.scope_prefix_code);
        }
        os << sig_info.unscoped_orig_name << "[" << (i + sig_info.bit_low) << "] ";
        if (interval_pico <= 0) {
          BitEntropyInfo& bit_info = sig_info.CurrentTimeSlice().bit_info.at(i);
          assert(bit_info.total_time() > 0);
          os << bit_info.num_0 << " "
             << bit_info.num_1 << " "
             << bit_info.num_x << " "
             << bit_info.num_z << " "
             << std::endl;
        } else {
          for (auto& slice : sig_info.time_slices) {
            BitEntropyInfo& bit_info = slice.bit_info.at(i);
            os << bit_info.num_0 << " "
               << bit_info.num_1 << " "
               << bit_info.num_x << " "
               << bit_info.num_z << " ";
          }
          os << std::endl;
        }
      }
    }
  }
}

}  // namespace vcd_parser

#endif /* VCD_PARSER_H_ */
