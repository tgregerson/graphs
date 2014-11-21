/*
 * vcd_lexer.h
 *
 *  Created on: Nov 14, 2014
 *      Author: gregerso
 */

#ifndef VCD_LEXER_H_
#define VCD_LEXER_H_

#include <exception>
#include <string>

class VcdLexer {
 public:
  class LexingException : public std::exception {
   public:
    LexingException(const std::string& msg) : msg_(msg) {}

    const char* what() const throw () {
      return msg_.c_str();
    }

   private:
    const std::string msg_;
  };

  // The Consume methods return a copy of 'input' with the first token
  // removed. If 'token' is non-null, also copy the token to it. Whitespace
  // is greedily consumed before and after the token, but not included in
  // 'token' unless it is syntactically significant to the token. Unless
  // otherwise stated, the Consume methods throw LexingException if they
  // cannot consume the requested token.

  // VcdDefinitions = [DeclarationCommand]* [SimulationCommand]*
  static std::string ConsumeVcdDefinitions(
      const std::string& input, std::string* token = nullptr);

  // DeclarationCommand = DeclarationKeyword TextToEnd
  static std::string ConsumeDeclarationCommand(
      const std::string& input, std::string* token = nullptr);

  // SimulationCommand = SimulationValueCommand | Comment | SimulationTime |
  //                     ValueChange
  static std::string ConsumeSimulationCommand(
      const std::string& input, std::string* token = nullptr);

  // SimulationValueCommand = SimulationKeyword ValueChange* $end
  static std::string ConsumeSimulationValueCommand(
      const std::string& input, std::string* token = nullptr);

  // Comment = $comment [.*] $end
  static std::string ConsumeComment(
      const std::string& input, std::string* token = nullptr);

  // DeclarationKeyword = $comment | $date | $enddefinitions | $scope |
  //                      $timescale | $upscope | $var | $version
  static std::string ConsumeDeclarationKeyword(
      const std::string& input, std::string* token = nullptr);

  // SimulationKeyword = $dumpall | $dumpoff | $dumpon | $dumpvars
  static std::string ConsumeSimulationKeyword(
      const std::string& input, std::string* token = nullptr);

  // SimulationTime = # DecimalNumber
  static std::string ConsumeSimulationTime(
      const std::string& input, std::string* token = nullptr);

  // Value = 0 | 1 | x | X | z | Z
  static std::string ConsumeValueNoWhitespace(
      const std::string& input, std::string* token = nullptr);

  // ValueChange = ScalarValueChange | VectorValueChange
  static std::string ConsumeValueChange(
      const std::string& input, std::string* token = nullptr);

  // ScalarValueChange = Value IdentifierCode
  static std::string ConsumeScalarValueChange(
      const std::string& input, std::string* token = nullptr);

  // BinaryVectorValueChange = (b | B) BinaryNumber IdentifierCode
  static std::string ConsumeBinaryVectorValueChange(
      const std::string& input, std::string* token = nullptr);

  // RealVectorValueChange = (r | R) RealNumber IdentifierCode
  static std::string ConsumeRealVectorValueChange(
      const std::string& input, std::string* token = nullptr);

  // VectorValueChange = BinaryVectorValueChange | RealVectorValueChange
  static std::string ConsumeVectorValueChange(
      const std::string& input, std::string* token = nullptr);

  // IdentifierCode = Non-whitespace characters
  static std::string ConsumeIdentifierCode(
      const std::string& input, std::string* token = nullptr);

  // ExactString = str
  static std::string ConsumeExactString(
      const std::string& str, const std::string& input,
      std::string* token = nullptr);

  // Tag = $tag_name
  static std::string ConsumeTag(
      const std::string& tag_name, const std::string& input,
      std::string* token = nullptr);

  // TextToEnd = [text] $end
  static std::string ConsumeTextToEnd(
      const std::string& input, std::string* token = nullptr);

  static std::string ConsumeNonWhitespace(
      const std::string& input, std::string* token = nullptr);
};

#endif /* VCD_LEXER_H_ */
