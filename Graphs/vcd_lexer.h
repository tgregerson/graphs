/*
 * vcd_lexer.h
 *
 *  Created on: Nov 14, 2014
 *      Author: gregerso
 */

#ifndef VCD_LEXER_H_
#define VCD_LEXER_H_

#include <cassert>
#include <cstdio>

#include <exception>
#include <iostream>
#include <istream>
#include <string>
#include <memory>

#include "file_helpers.h"

namespace vcd_lexer {

class LexingException : public std::exception {
  public:
  LexingException(const std::string& msg) : msg_(msg) {}

  const char* what() const throw () {
    return msg_.c_str();
  }

  private:
  const std::string msg_;
};

// Buffers used for static buffer-based lexing methods
constexpr int kTimeBufferSize = 4 * 1024;
constexpr int kValueBufferSize = 64 * 1024;
constexpr int kWsBufferSize = 4 * 1024;

extern std::unique_ptr<char> global_ws_buffer_ ;
extern std::unique_ptr<char> global_value_buffer_;
extern std::unique_ptr<char> global_time_buffer_;

// The Consume methods return a copy of 'input' with the first token
// removed. If 'token' is non-null, also copy the token to it. Whitespace
// is greedily consumed before and after the token, but not included in
// 'token' unless it is syntactically significant to the token. Unless
// otherwise stated, the Consume methods throw LexingException if they
// cannot consume the requested token.

// VcdDefinitions = [DeclarationCommand]* [SimulationCommand]*
std::string ConsumeVcdDefinitionsString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeVcdDefinitions(std::istream& in, std::string* token = nullptr);

// DeclarationCommand = DeclarationKeyword TextToEnd
std::string ConsumeDeclarationCommandString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeDeclarationCommand(T& in, std::string* token = nullptr);

// SimulationCommand = SimulationValueCommand | Comment | SimulationTime |
//                     ValueChange
std::string ConsumeSimulationCommandString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeSimulationCommand(T& in, std::string* token = nullptr);

// SimulationValueCommand = SimulationKeyword ValueChange* $end
std::string ConsumeSimulationValueCommandString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeSimulationValueCommand(T& in, std::string* token = nullptr);

// Comment = $comment [.*] $end
std::string ConsumeCommentString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeComment(T& in, std::string* token = nullptr);

// DeclarationKeyword = $comment | $date | $enddefinitions | $scope |
//                      $timescale | $upscope | $var | $version
std::string ConsumeDeclarationKeywordString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeDeclarationKeyword(T& in, std::string* token = nullptr);

// SimulationKeyword = $dumpall | $dumpoff | $dumpon | $dumpvars
std::string ConsumeSimulationKeywordString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeSimulationKeyword(T& in, std::string* token = nullptr);

// SimulationTime = # DecimalNumber
std::string ConsumeSimulationTimeString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeSimulationTime(T& in, std::string* token = nullptr);
template <typename T>
bool ConsumeSimulationTimeBuffer(T& in, std::string* token = nullptr);

// Value = 0 | 1 | x | X | z | Z
std::string ConsumeValueNoWhitespaceString(
    const std::string& input, std::string* token = nullptr);
bool ConsumeValueNoWhitespace(
    std::istream& in, std::string* token = nullptr);
bool ConsumeValueNoWhitespace(
    std::istream& in, char** buffer_pos);
bool ConsumeValueNoWhitespace(
    FILE* in, std::string* token);
bool ConsumeValueNoWhitespace(
    FILE* in, char** buffer_pos);

// ValueChange = ScalarValueChange | VectorValueChange
std::string ConsumeValueChangeString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeValueChange(T& in, std::string* token = nullptr);
template <typename T>
bool ConsumeValueChangeBuffer(T& in, std::string* token = nullptr);

// ScalarValueChange = Value IdentifierCode
std::string ConsumeScalarValueChangeString(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeScalarValueChange(T& in, std::string* token);
template <typename T>
bool ConsumeScalarValueChangeBuffer(T& in, char** buffer_pos);

// BinaryVectorValueChange = (b | B) BinaryNumber IdentifierCode
std::string ConsumeBinaryVectorValueChange(
    const std::string& input, std::string* token = nullptr);
template <typename T>
bool ConsumeBinaryVectorValueChange(T& in, std::string* token = nullptr);
template <typename T>
bool ConsumeBinaryVectorValueChange(T& in, char** buffer_pos);

// RealVectorValueChange = (r | R) RealNumber IdentifierCode
std::string ConsumeRealVectorValueChangeString(
    const std::string& input, std::string* token = nullptr);
template <typename T, typename U>
bool ConsumeRealVectorValueChange(T& in, U* out);

// VectorValueChange = BinaryVectorValueChange | RealVectorValueChange
std::string ConsumeVectorValueChangeString(
    const std::string& input, std::string* token = nullptr);
template <typename T, typename U>
bool ConsumeVectorValueChange(T& in, U* out);

// IdentifierCode = Non-whitespace characters
std::string ConsumeIdentifierCodeString(
    const std::string& input, std::string* token = nullptr);
template <typename T, typename U>
bool ConsumeIdentifierCode(T& in, U* out);

// ExactString = str
std::string ConsumeExactString(
    const std::string& str, const std::string& input,
    std::string* token = nullptr);
bool ConsumeExactString(
    const std::string& str, std::istream& in,
    std::string* token = nullptr);
bool ConsumeExactString(
    const std::string& str, FILE* in,
    std::string* token = nullptr);

// TextToEnd = [text] $end
std::string ConsumeTextToEnd(
    const std::string& input, std::string* token = nullptr);
bool ConsumeTextToEnd(
    std::istream& in, std::string* token = nullptr);
bool ConsumeTextToEnd(
    FILE* in, std::string* token = nullptr);

std::string ConsumeNonWhitespace(
    const std::string& input, std::string* token = nullptr);
bool ConsumeNonWhitespace(
    std::istream& in, std::string* token = nullptr);
bool ConsumeNonWhitespace(
    std::istream& in, char** buffer_pos);
bool ConsumeNonWhitespace(
    FILE* in, std::string* token);
bool ConsumeNonWhitespace(
    FILE* in, char** buffer_pos);

bool ConsumeWhitespace(
    std::istream& in, std::string* token = nullptr);
bool ConsumeWhitespace(
    FILE* in, std::string* token = nullptr);
bool ConsumeWhitespace(
    std::istream& in, char** buffer_pos);
bool ConsumeWhitespace(
    FILE* in, char** buffer_pos);

std::string ConsumeWhitespaceOptional(const std::string& input);
void ConsumeWhitespaceOptional(std::istream& in);
void ConsumeWhitespaceOptional(std::istream& in, std::string* token);
void ConsumeWhitespaceOptional(std::istream& in, char** buffer_pos);
void ConsumeWhitespaceOptional(FILE* in);
void ConsumeWhitespaceOptional(FILE* in, std::string* token);
void ConsumeWhitespaceOptional(FILE* in, char** buffer_pos);

std::string ConsumeDecimalNumber(
    const std::string& in, std::string* token = nullptr);
bool ConsumeDecimalNumber(
    std::istream& in, std::string* token = nullptr);
bool ConsumeDecimalNumber(
    std::istream& in, char** buffer_pos);
bool ConsumeDecimalNumber(
    FILE* in, std::string* token = nullptr);
bool ConsumeDecimalNumber(
    FILE* in, char** buffer_pos);

std::string ConsumeUnsignedDecimalNumber(
    const std::string& in, std::string* token = nullptr);
bool ConsumeUnsignedDecimalNumber(
    std::istream& in, std::string* token = nullptr);
bool ConsumeUnsignedDecimalNumber(
    std::istream& in, char** buffer_pos);
bool ConsumeUnsignedDecimalNumber(
    FILE* in, std::string* token = nullptr);
bool ConsumeUnsignedDecimalNumber(
    FILE* in, char** buffer_pos);


// Template function definitions
template <typename T>
bool ConsumeVcdDefinitions(T& in, std::string* token) {
  std::string consumed;
  ConsumeWhitespaceOptional(in);

  const auto cur_pos = fhelp::GetPosition(in);
  fhelp::SeekToEnd(in);
  const auto max_pos = fhelp::GetPosition(in);
  fhelp::SeekToPosition(in, cur_pos);

  long int last_print_point = 0;

  std::string append_token;
  bool declarations_done = false;
  int num_lines = 0;
  std::cout << "-----------Start Lexing Declarations------------" << std::endl;
  while (!declarations_done) {
    ++num_lines;
    if (num_lines - last_print_point > 1000000LL) {
      last_print_point = num_lines;
      std::cout << "Cur pos: " << fhelp::GetPosition(in) << " of "
                << max_pos << std::endl;
    }
    char* buffer_pos = global_ws_buffer_.get();
    ConsumeWhitespaceOptional(in, &buffer_pos);
    if (buffer_pos != global_ws_buffer_.get()) {
      if (token != nullptr) {
        consumed.append(global_ws_buffer_.get(),
                        buffer_pos - global_ws_buffer_.get());
      }
    }
    if (ConsumeDeclarationCommand(in, &append_token)) {
      if (token != nullptr) {
        consumed.append(append_token);
      }
    } else {
      declarations_done = true;
    }
  }

  std::cout << "-----------Done Lexing Declarations------------" << std::endl;

  bool simulations_done = false;
  std::cout << "-----------Start Lexing Simulations------------" << std::endl;
  while (!simulations_done) {
    ++num_lines;
    if (num_lines - last_print_point > 1000000LL) {
      last_print_point = num_lines;
      std::cout << "Cur pos: " << fhelp::GetPosition(in) << " of "
                << max_pos << std::endl;
    }
    char* buffer_pos = global_ws_buffer_.get();
    ConsumeWhitespaceOptional(in, &buffer_pos);
    if (buffer_pos != global_ws_buffer_.get()) {
      if (token != nullptr) {
        consumed.append(global_ws_buffer_.get(),
                        buffer_pos - global_ws_buffer_.get());
      }
    }
    if (ConsumeSimulationCommand(in, &append_token)) {
      if (token != nullptr) {
        consumed.append(append_token);
      }
    } else {
      simulations_done = true;
    }
  }
  std::cout << "-----------Done Lexing Simulations------------" << std::endl;

  if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return true;
}

template <typename T>
bool ConsumeDeclarationCommand(T& in, std::string* token) {
  std::string consumed;
  bool found = false;
  if (ConsumeDeclarationKeyword(in, &consumed)) {
    std::string append_token;
    if (ConsumeTextToEnd(in, &append_token)) {
      consumed.append(append_token);
      found = true;
    }
  }
  if (found && token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

template <typename T>
bool ConsumeSimulationCommand(T& in, std::string* token) {
  return ConsumeValueChangeBuffer(in, token) ||
         ConsumeSimulationTimeBuffer(in, token) ||
         ConsumeSimulationValueCommand(in, token) ||
         ConsumeComment(in, token);
}

template <typename T>
bool ConsumeSimulationValueCommand(T& in, std::string* token) {
  const auto initial_pos = fhelp::GetPosition(in);
  std::string consumed;
  bool found = false;
  if (ConsumeSimulationKeyword(in, &consumed)) {
    bool done_processing_value_changes = false;
    std::string append_token;
    while (!done_processing_value_changes) {
      if (ConsumeWhitespace(in, &append_token)) {
        consumed.append(append_token);
        if (ConsumeValueChangeBuffer(in, &append_token)) {
          consumed.append(append_token);
        } else {
          done_processing_value_changes = true;
        }
      } else {
        done_processing_value_changes = true;
      }
    }
    if (ConsumeExactString("$end", in)) {
      consumed.append("$end");
      found = true;
    }
  }
  if (!found) {
    fhelp::SeekToPosition(in, initial_pos);
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

template <typename T>
bool ConsumeComment(T& in, std::string* token) {
  const auto initial_pos = fhelp::GetPosition(in);
  std::string consumed;
  bool found = false;
  ConsumeWhitespaceOptional(in);
  if (ConsumeExactString("$comment", in, &consumed)) {
    std::string append_token;
    if (ConsumeTextToEnd(in, &append_token)) {
      consumed.append(append_token);
      found = true;
    }
  }
  if (!found) {
    fhelp::SeekToPosition(in, initial_pos);
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

template <typename T>
bool ConsumeDeclarationKeyword(T& in, std::string* token) {
  ConsumeWhitespaceOptional(in);
  return ConsumeExactString("$var", in, token) ||
         ConsumeExactString("$comment", in, token) ||
         ConsumeExactString("$date", in, token) ||
         ConsumeExactString("$enddefinitions", in, token) ||
         ConsumeExactString("$scope", in, token) ||
         ConsumeExactString("$timescale", in, token) ||
         ConsumeExactString("$upscope", in, token) ||
         ConsumeExactString("$version", in, token);
}

template <typename T>
bool ConsumeSimulationKeyword(T& in, std::string* token) {
  ConsumeWhitespaceOptional(in);
  return ConsumeExactString("$dumpall", in, token) ||
         ConsumeExactString("$dumpoff", in, token) ||
         ConsumeExactString("$dumpon", in, token) ||
         ConsumeExactString("$dumpvars", in, token);
}

template <typename T>
bool ConsumeSimulationTime(T& in, std::string* token) {
  ConsumeWhitespaceOptional(in);
  const auto initial_pos = fhelp::GetPosition(in);
  std::string consumed;
  bool found = false;
  if (fhelp::PeekChar(in) == '#') {
    consumed.push_back(fhelp::GetChar(in));
    std::string append_token;
    if (ConsumeDecimalNumber(in, &append_token)) {
      consumed.append(append_token);
      found = true;
    }
  }
  if (!found) {
    fhelp::SeekToPosition(in, initial_pos);
  } else if (token != nullptr) {
    token->assign(std::move(consumed));
  }
  return found;
}

template <typename T>
bool ConsumeSimulationTimeBuffer(T& in, std::string* token) {
  ConsumeWhitespaceOptional(in);
  bool found = false;
  char* number_buffer = &(global_time_buffer_.get()[1]);
  char c = fhelp::GetChar(in);
  if (c == '#') {
    global_time_buffer_.get()[0] = '#';
    found = ConsumeDecimalNumber(in, &number_buffer);
  }
  if (!found && c != EOF) {
    fhelp::UngetChar(c, in);
  } else if (token != nullptr) {
    token->assign(global_time_buffer_.get(),
                  number_buffer - global_time_buffer_.get());
  }
  return found;
}

template <typename T>
bool ConsumeValueChange(T& in, std::string* token) {
  return ConsumeScalarValueChange(in, token) ||
         ConsumeVectorValueChange(in, token);
}

template <typename T>
bool ConsumeValueChangeBuffer(T& in, std::string* token) {
  char* buffer_pos = global_value_buffer_.get();
  bool found = ConsumeScalarValueChangeBuffer(in, &buffer_pos) ||
               ConsumeVectorValueChange(in, &buffer_pos);
  if (found && token != nullptr) {
      token->assign(global_value_buffer_.get(),
                    buffer_pos - global_value_buffer_.get());
  }
  return found;
}

template <typename T>
bool ConsumeScalarValueChange(T& in, std::string* token) {
  bool found = false;
  ConsumeWhitespaceOptional(in);
  char c;
  char* c_ptr = &c;
  if (ConsumeValueNoWhitespace(in, &c_ptr)) {
    std::string append_token;
    found = ConsumeIdentifierCode(in, &append_token);
    if (found) {
      if (token != nullptr) {
        token->resize(0);
        token->push_back(c);
        token->append(append_token);
      }
    } else {
      fhelp::UngetChar(c, in);
    }
  }
  return found;
}

template <typename T>
bool ConsumeScalarValueChangeBuffer(T& in, char** buffer_pos) {
  bool found = false;
  ConsumeWhitespaceOptional(in);
  if (ConsumeValueNoWhitespace(in, buffer_pos)) {
    found = ConsumeIdentifierCode(in, buffer_pos);
  }
  return found;
}

template <typename T>
bool ConsumeBinaryVectorValueChange(T& in, std::string* token) {
  ConsumeWhitespaceOptional(in);
  std::string consumed;
  bool found = false;
  char c = fhelp::GetChar(in);
  if (c == 'b' || c == 'B') {
    consumed.push_back(c);
    bool done_getting_values = false;
    bool found_at_least_one_value = false;
    std::string append_token;
    while (!done_getting_values) {
      if (ConsumeValueNoWhitespace(in, &append_token)) {
        found_at_least_one_value = true;
        consumed.append(append_token);
      } else {
        done_getting_values = true;
      }
    }
    if (found_at_least_one_value) {
      if (ConsumeWhitespace(in, &append_token)) {
        consumed.append(append_token);
        if (ConsumeIdentifierCode(in, &append_token)) {
          consumed.append(append_token);
          found = true;
        }
      }
    }
    if (found) {
      if (token != nullptr) {
        token->assign(std::move(consumed));
      }
    } else {
      for (size_t i = consumed.size() - 1; i >= 0; --i) {
        fhelp::UngetChar(consumed[i], in);
      }
    }
  } else {
    fhelp::UngetChar(c, in);
  }
  return found;
}

template <typename T>
bool ConsumeBinaryVectorValueChange(T& in, char** buffer_pos) {
  ConsumeWhitespaceOptional(in);
  char* initial_buffer_pos = *buffer_pos;
  bool found = false;
  char c = fhelp::GetChar(in);
  if (c == 'b' || c == 'B') {
    assert(*buffer_pos < global_value_buffer_.get() + kValueBufferSize);
    **buffer_pos = c;
    ++(*buffer_pos);
    bool done_getting_values = false;
    bool found_at_least_one_value = false;
    while (!done_getting_values) {
      if (ConsumeValueNoWhitespace(in, buffer_pos)) {
        found_at_least_one_value = true;
      } else {
        done_getting_values = true;
      }
    }
    if (found_at_least_one_value) {
      found = ConsumeWhitespace(in, buffer_pos) &&
              ConsumeIdentifierCode(in, buffer_pos);
    }
    if (!found) {
      while (*buffer_pos > initial_buffer_pos) {
        --(*buffer_pos);
        fhelp::UngetChar(**buffer_pos, in);
      }
    }
  } else {
    fhelp::UngetChar(c, in);
  }
  return found;
}

template <typename T, typename U>
bool ConsumeRealVectorValueChange(T& in, U* token) {
  std::cout << "Real values unsupported\n";
  return false;
}

template <typename T, typename U>
bool ConsumeVectorValueChange(T& in, U* out) {
  return ConsumeBinaryVectorValueChange(in, out);
}

template <typename T, typename U>
bool ConsumeIdentifierCode(T& in, U* out) {
  ConsumeWhitespaceOptional(in);
  return ConsumeNonWhitespace(in, out);
}

}  // namespace vcd_lexer

#endif /* VCD_LEXER_H_ */
