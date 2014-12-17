#ifndef VCD_PARSER_H_
#define VCD_PARSER_H_

#include <istream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    RadixType radix{RadixType::BinaryNumber};
    char radix_char{'b'};
    std::string number_string;
    IdentifierCode identifier_code;
  };

  struct Value {
    char value{'0'};
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
    unsigned long long time{0ULL};
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

  struct DeclarationCommand {
    DeclarationKeyword declaration_keyword;
    std::string command_text;
  };

  struct VcdDefinitions {
    std::vector<DeclarationCommand> declaration_commands;
    std::vector<SimulationCommand> simulation_commands;
  };
}  // namespace vcd_token

class VcdParser {
 public:
  VcdParser(const std::string& vcd_filename,
            const std::set<std::string>& signals)
    : vcd_filename_(vcd_filename), raw_monitored_signals_(signals) {
    for (const std::string& signal : raw_monitored_signals_) {
      name_fixed_monitored_signals_.insert(ConvertToVcdSignalName(signal));
    }
  }


  // Parses input VCD file. Parses nets in 'signals'. Writes status messages
  // to cout if 'echo_status'.
  void Parse(bool echo_status);

  static void WriteDumpVars(const std::string& signal_filename,
                            const std::string& output_filename,
                            const std::string& scope);

 private:
  void SkipToEndToken(std::ifstream& input_file);

  void ParseScope(std::ifstream& input_file,
                  const std::string& parent_scope);

  void ParseVar(std::ifstream& input_file);

  long long ParseDumpAll(std::ifstream& input_file);

  void PrintStats();

  void CheckSignalsExistOrDie();

  // Converts a signal name to the format used by the VCD file. Bit selects are
  // removed and stored in the second part of the pair. If the signal name was
  // escaped, removes the leading '\\' and the trailing space.
  std::pair<std::string, int> ConvertToVcdSignalName(
      const std::string& raw_signal_name);

  // Removes the leading '\' character that is present in signal names, but
  // not printed in var names by ModelSim.
  static std::string StripLeadingEscapeChar(const std::string& str);

  const std::string vcd_filename_;

  bool echo_status_;
  std::map<std::pair<std::string, int>, std::string>
      modified_signal_name_to_identifier_code_;
  std::unordered_map<std::string, std::vector<char>> identifier_code_to_values_;

  // These are the signals that were requested for monitoring using their raw
  // names from the parsed verilog file. VCD does not properly support escaped
  // variable names, so these names may need to be modified to match the
  // identifiers used in the VCD.
  std::set<std::string> raw_monitored_signals_;

  // Contains the monitored signals, with escaped signal names modified to
  // match the requirements of the VCD format. Bit selects are removed from
  // the name and included in the second element of the pair.
  std::set<std::pair<std::string, int>> name_fixed_monitored_signals_;

  std::set<std::string> monitored_identifier_codes_;

  std::set<std::pair<std::string, int>> all_vcd_identifier_names_;
};

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
  // TODO Might want to use a buffer here for performance reasons.
  vcd_lexer::ConsumeWhitespaceOptional(in);
  char c = fhelp::GetChar(in);
  if ('#' == c) {
    std::string time;
    if (vcd_lexer::ConsumeDecimalNumber(in, &time)) {
      st->time = strtoull(time.c_str(), nullptr, 10);
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
    assert(vcd_lexer::ConsumeExactString("$end", in));
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
  if (TryParseSimulationValueCommandFromInput(
      in, &(sc->simulation_value_command))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::SimulationValueCommand;
  } else if (TryParseCommentFromInput(in, &(sc->comment))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::CommentCommand;
  } else if (TryParseSimulationTimeFromInput(in, &(sc->simulation_time))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::TimeCommand;
  } else if (TryParseValueChangeFromInput(in, &(sc->value_change))) {
    sc->type =
        vcd_token::SimulationCommand::SimulationCommandType::ValueChangeCommand;
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
  vcd_token::SimulationCommand sc;
  while (!fhelp::IsEof(in)) {
    TryParseSimulationCommandFromInput(in, &sc);
    vd->simulation_commands.push_back(sc);
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

}  // namespace vcd_parser

#endif /* VCD_PARSER_H_ */
