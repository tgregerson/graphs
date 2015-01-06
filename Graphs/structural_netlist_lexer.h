/*
 * structural_netlist_lexer.h
 *
 *  Created on: Sep 4, 2014
 *      Author: gregerso
 */

#ifndef STRUCTURAL_NETLIST_LEXER_H_
#define STRUCTURAL_NETLIST_LEXER_H_

#include <iostream>
#include <string>
#include <utility>
#include <vector>

class StructuralNetlistLexer {
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
  // otherwise stated, the Consume methods throw ParsingException if they
  // cannot consume the requested token.

  // Whitespace is a sequential combination of spaces, newlines, and tabs.
  // DOES NOT THROW EXCEPTION IF NOTHING CAN BE CONSUMED.
  static std::string ConsumeWhitespaceIfPresent(const std::string& input,
                                                std::string* token = nullptr);
  static bool ConsumeWhitespaceStream(std::istream& input,
                                      std::string* token = nullptr);

  // Identifier = SimpleIdentifier || EscapedIdentifier
  static std::string ConsumeIdentifier(const std::string& input,
                                       std::string* token);
  static bool ConsumeIdentifierStream(std::istream& input, std::string* token);

  // IdentifierList = Identifier[, Identifier]*
  static std::string ConsumeIdentifierList(const std::string& input,
                                           std::string* token);
  static bool ConsumeIdentifierListStream(std::istream& input, std::string* token);

  // SimpleIdentifier = (_ || a-z)(_ || a-z || 0-9 || $)*
  static std::string ConsumeSimpleIdentifier(const std::string& input,
                                             std::string* token);
  static bool ConsumeSimpleIdentifierStream(std::istream& input, std::string* token);

  // EscapedIdentifier = \\.+<single whitespace character>
  static std::string ConsumeEscapedIdentifier(const std::string& input,
                                              std::string* token);
  static bool ConsumeEscapedIdentifierStream(std::istream& input, std::string* token);

  // Connection = ConnectedElement || .Identifier(ConnectedElement)
  static std::string ConsumeConnection(
      const std::string& input,
      std::string* token);
  static bool ConsumeConnectionStream(std::istream& input, std::string* token);

  // ConnectionList = Connection[, Connection]*
  static std::string ConsumeConnectionList(
      const std::string& input,
      std::string* token);
  static bool ConsumeConnectionListStream(std::istream& input, std::string* token);

  // ConnectedElement = Immediate || (Identifier[BitRange] ||
  //                    {ConnectedElementList}
  static std::string ConsumeConnectedElement(const std::string& input,
                                             std::string* token);
  static bool ConsumeConnectedElementStream(std::istream& input,
                                            std::string* token);

  // ConnectedElementList = ConnectedElement[, ConnectedElement]*
  static std::string ConsumeConnectedElementList(
      const std::string& input,
      std::string* token);
  static bool ConsumeConnectedElementListStream(
      std::istream& input, std::string* token);

  // ParameterConnectedElement = Immediate || Identifier
  static std::string ConsumeParameterConnectedElement(const std::string& input,
                                                      std::string* token);
  static bool ConsumeParameterConnectedElementStream(
      std::istream& input, std::string* token);

  // ParameterConnection = ParameterConnectedElement ||
  //                       .Identifier(ParameterConnectedElement)
  static std::string ConsumeParameterConnection(const std::string& input,
                                                std::string* token);
  static bool ConsumeParameterConnectionStream(
      std::istream& input, std::string* token);

  // ParameterList = ParameterConnection[, ParameterConnection]*
  static std::string ConsumeParameterList(const std::string& input,
                                          std::string* token);
  static bool ConsumeParameterListStream(
      std::istream& input, std::string* token);

  // ModuleParameters = #(ParameterList)
  static std::string ConsumeModuleParameters(const std::string& input,
                                             std::string* token);
  static bool ConsumeModuleParametersStream(
      std::istream& input, std::string* token);

  // Immediate = BinaryImmediate || DecimalImmediate || HexImmediate ||
  //             OctalImmediate || StringLiteral
  static std::string ConsumeImmediate(const std::string& input,
                                      std::string* token);
  static bool ConsumeImmediateStream(std::istream& input, std::string* token);

  // BinaryImmediate = (UnbasedImmedate)'b(0-1)+
  static std::string ConsumeBinaryImmediate(const std::string& input,
                                            std::string* token);
  static bool ConsumeBinaryImmediateStream(
      std::istream& input, std::string* token);

  // OctalImmediate = (UnbasedImmediate)'o(0-7)+
  static std::string ConsumeOctalImmediate(const std::string& input,
                                           std::string* token);
  static bool ConsumeOctalImmediateStream(
      std::istream& input, std::string* token);

  // DecimalImmediate = UnbasedImmediate'd(0-9)+
  static std::string ConsumeDecimalImmediate(const std::string& input,
                                             std::string* token);
  static bool ConsumeDecimalImmediateStream(
      std::istream& input, std::string* token);

  // HexImmediate = (UnbasedImmediate)'h(0-9 || a-f || A-F)+
  static std::string ConsumeHexImmediate(const std::string& input,
                                         std::string* token);
  static bool ConsumeHexImmediateStream(
      std::istream& input, std::string* token);

  // UnbasedImmediate = (0-9)+
  static std::string ConsumeUnbasedImmediate(const std::string& input,
                                             std::string* token);
  static bool ConsumeUnbasedImmediateStream(
      std::istream& input, std::string* token);

  // StringLiteral = ".*"
  static std::string ConsumeStringLiteral(const std::string& input,
                                          std::string* token);
  static bool ConsumeStringLiteralStream(
      std::istream& input, std::string* token);

  // BitRange = \[UnbasedImmediate\] || \[UnbasedImmediate:UnbasedImmediate\]
  static std::string ConsumeBitRange(const std::string& input,
                                     std::string* token);
  static bool ConsumeBitRangeStream(std::istream& input, std::string* token);

  // Char = any non-whitespace character
  static std::string ConsumeChar(const std::string& input,
                                 std::string* token, char c);
  static bool ConsumeCharStream(
      std::istream& input, std::string* token, char c);

  // Consumes an element wrapped by 'open' and 'close' using 'consumer' to
  // consume the inner token. The string that is returned contains only
  // the inner token, not the wrappers.
  static std::string ConsumeWrappedElement(
      const std::string& input,
      std::string* token,
      std::string (*consumer)(const std::string&, std::string*),
      char open, char close);
  static bool ConsumeWrappedElementStream(
      std::istream& input,
      std::string* token,
      bool (*consumer)(std::istream&, std::string*),
      char open, char close);

  // Connection is extracted as a pair of strings, where the first element is
  // the connection name (if present) and the second element is the connected
  // element.
  static std::pair<std::string,std::string> ExtractConnectionFromConnection(
      const std::string& connection);

  static std::pair<std::string,std::string> ExtractParameterConnectionFromParameterConnection(
      const std::string& connection);

  static std::vector<std::string> ExtractIdentifiersFromIdentifierList(
      const std::string& identifier_list);

  static std::vector<std::pair<std::string,std::string>> ExtractConnectionsFromConnectionList(
      const std::string& connection_list);

  static std::vector<std::string> ExtractConnectedElementsFromConnectedElementList(
      const std::string& connected_element_list);

  static std::vector<std::string> ExtractConnectedElementsFromConnectedElement(
      const std::string& connected_element);

  static std::vector<std::pair<std::string,std::string>> ExtractParameterConnectionsFromParameterList(
      const std::string& plist);

  static std::vector<std::pair<std::string,std::string>> ExtractParameterConnectionsFromModuleParameters(
      const std::string& module_parameters);

  static std::pair<int, int> ExtractBitRange(const std::string& bit_range);

  // Trims the first and last char from 'input' and any additional leading
  // whitespace.
  static std::string ExtractInner(const std::string& input);

  // First value is number of bits, second is the immedate value, aligned to
  // the least significant digit.
  static std::pair<int,uint64_t> VlogImmediateToUint64 (const std::string& str);

  struct NetDescriptor {
    std::string raw_name;
    std::string base_name;
    bool uses_index{false};
    int bit_high{0};
    int bit_low{0};

    void Print() {
      std::cout << "RAW NAME: " << raw_name << "\n";
      std::cout << "BASE NAME: " << base_name << "\n";
      std::cout << "BIT HIGH: " << bit_high << "\n";
      std::cout << "BIT LOW: " << bit_low << "\n";
      std::cout << "USES INDEX: " << (uses_index ? "TRUE" : "FALSE") << "\n";
    }
  };

  static NetDescriptor ExtractDescriptorFromIdentifier(const std::string& str);
};



#endif /* STRUCTURAL_NETLIST_LEXER_H_ */
