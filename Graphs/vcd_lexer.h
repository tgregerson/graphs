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

  // ExactString = str
  static std::string ConsumeExactString(
      const std::string& str, const std::string& input,
      std::string* token = nullptr);

  // Tag = $tag_name
  static std::string ConsumeTag(
      const std::string& tag_name, const std::string& input,
      std::string* token = nullptr);

  static std::string ConsumeNonWhitespace(
      const std::string& input, std::string* token = nullptr);

  static std::string ConsumeVar(
      const std::string& input, std::string* token = nullptr);


};




#endif /* VCD_LEXER_H_ */
