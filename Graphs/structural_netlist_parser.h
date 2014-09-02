#ifndef STRUCTURAL_NETLIST_PARSER_H_
#define STRUCTURAL_NETLIST_PARSER_H_

#include <cassert>

#include <exception>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

// TODO: Move this to its own header.
enum class ModuleType {
  BUFG,
  CARRY4,
  DSP48E1,
  FDCE,
  FDPE,
  FDRE,
  FDSE,
  GND,
  IBUF,
  LUT1,
  LUT2,
  LUT3,
  LUT4,
  LUT5,
  LUT6,
  OBUF,
  MUXF7,
  MUXF8,
  RAMB18E1,
  RAMB36E1,
  RAM64M,
  SRL16E,
  VCC,
  UNSUPPORTED
};

inline ModuleType StringToModuleType(const std::string& str) {
  if (str == "BUFG") {
    return ModuleType::BUFG;
  } else if (str == "CARRY4") {
    return ModuleType::CARRY4;
  } else if (str == "DSP48E1") {
    return ModuleType::DSP48E1;
  } else if (str == "FDCE") {
    return ModuleType::FDCE;
  } else if (str == "FDPE") {
    return ModuleType::FDRE;
  } else if (str == "FDRE") {
    return ModuleType::FDRE;
  } else if (str == "FDSE") {
    return ModuleType::FDSE;
  } else if (str == "GND") {
    return ModuleType::GND;
  } else if (str == "IBUF") {
    return ModuleType::IBUF;
  } else if (str == "LUT1") {
    return ModuleType::LUT1;
  } else if (str == "LUT2") {
    return ModuleType::LUT2;
  } else if (str == "LUT3") {
    return ModuleType::LUT3;
  } else if (str == "LUT4") {
    return ModuleType::LUT4;
  } else if (str == "LUT5") {
    return ModuleType::LUT5;
  } else if (str == "LUT6") {
    return ModuleType::LUT6;
  } else if (str == "OBUF") {
    return ModuleType::OBUF;
  } else if (str == "MUXF7") {
    return ModuleType::MUXF7;
  } else if (str == "MUXF8") {
    return ModuleType::MUXF8;
  } else if (str == "RAMB18E1") {
    return ModuleType::RAMB18E1;
  } else if (str == "RAMB36E1") {
    return ModuleType::RAMB36E1;
  } else if (str == "RAM64M") {
    return ModuleType::RAM64M;
  } else if (str == "SRL16E") {
    return ModuleType::SRL16E;
  } else if (str == "VCC") {
    return ModuleType::VCC;
  } else {
    return ModuleType::UNSUPPORTED;
  }
}


struct VlogNet {
  VlogNet(const std::string& name, int w = 1, int hi = 0, int lo = 0)
    : name(name), width(w), bit_hi(hi), bit_lo(lo) {}
  std::string name;
  int width;
  int bit_hi;
  int bit_lo;
  // Included for debug reasons.
  std::string src_line;
};

struct VlogModule {
  VlogModule(const std::string& type, const std::string& inst)
    : type_name(type), instance_name(inst) {}
  std::string type_name;
  std::string instance_name;
  std::set<std::string> connected_net_names;
  // Included for debug reasons.
  std::string src_line;
  std::string src_line_post_strip;
  std::string src_connection_list;

  // Extended fields
  ModuleType type{ModuleType::UNSUPPORTED};
  std::map<std::string, std::string> named_parameters;
  std::vector<std::string> ordered_parameters;
  // TODO: Map may be the wrong data structure; may need a multimap after
  // expanding buses.
  std::map<std::string, std::string> named_connections;
  std::vector<std::string> ordered_connections;
};

class StructuralNetlistParser {
 public:
  StructuralNetlistParser() {}
  ~StructuralNetlistParser() {}

  // Scans through map of nets and identifies busses. Finds these bus
  // connections in the module map, and replaces those connections with
  // expanded single-bit connections.
  void ExpandModuleBusConnections(
      const std::map<std::string, VlogNet>& nets,
      std::map<std::string, VlogModule>* module_container);

  // Populates a map, module name -> VlogModule, from a list of lines. Determines
  // if the line contains a module instantiation if it doesn't start with a set
  // of keywords. This assumes that a module instantiation is not split between
  // lines.
  void PopulateModules(std::map<std::string, VlogModule>* modules,
                       const std::list<std::string>& lines);

  // Populates a map, net name -> VlogNet, from a list of lines. Determines
  // if the line contains a net declaration if it starts with a net
  // keyword. This assumes that a net declaration is not split between lines.
  void PopulateNets(std::map<std::string, VlogNet>* nets,
                    const std::list<std::string>& lines);

  // Prints debug information about a VlogModule.
  void PrintModule(const VlogModule& module);

  // Prints a module in the .NTL format used by Processed Netlist Parser.
  void PrintModuleNtlFormat(const VlogModule& module,
                            const std::map<std::string, VlogNet>& nets,
                            std::ostream& output_stream);

  // Prints a net in the .NTL format used by Processed Netlist Parser.
  void PrintNet(const VlogNet& net);

  // Prints command-line invokation format and exit.
  void PrintHelpAndDie();

  // Takes a list of strings in the form of lines read from the netlist.
  // Concatenates lines such that all strings end with a semi-colon.
  // Exception: 'endmodule' does not have a terminating semi-colon.
  std::list<std::string> RawLinesToSemicolonTerminated(
      const std::list<std::string>& raw_lines);

  // Returns a string made from 'str' with leading and trailing whitespace
  // trimmed.
  std::string trimLeadingTrailingWhiteSpace(const std::string& str);

  // Looks for a parameter list, in the format of #(a, b, ... c) in the input
  // string and strips it out if found.
  std::string StripModuleParameters(const std::string& my_str);

  // Remove connections from a given net name from modules. Useful for removing
  // known global nets.
  void RemoveNetConnectionFromModules(
      std::map<std::string, VlogModule>* modules,
      const std::set<std::string>& nets_to_remove);

  // Remove connections from a given net name from modules if it contains a
  // substring. Useful for removing 'UNCONNECTED' nets.
  void RemoveNetConnectionFromModulesSubstring(
      std::map<std::string, VlogModule>* modules,
      const std::set<std::string>& nets_to_remove_substring);

 private:
  class ParsingException : public std::exception {
   public:
    ParsingException(const std::string& msg) : msg_(msg) {}

    const char* what() const throw () {
      return msg_.c_str();
    }

   private:
    const std::string msg_;
  };



  // Identifiers may consist of either simple or escaped identifiers. In the
  // case of simple identifiers, we want to remove all surrounding whitespace.
  // For escaped identifiers, the first whitespace after the identifier is
  // actually considered part of the identifier, so we keep just that
  // whitespace and remove the rest.
  std::string CleanIdentifier(const std::string& str);

  // Helper fn to return the position of the first closing brack after the
  // start position or dying if none is found.
  size_t FindClosingBracketOrDie(const std::string& str, size_t start_pos);

  // If 'str' is in the form of 'a:b', where 'a' and 'b' are both decimal
  // integers, returns true and populates 'range' such that the first element
  // is the larger of 'a' and 'b' and the second is the smaller. Returns
  // false if 'str' is not in the valid range format, and value of 'range'
  // is undefined.
  bool ParseRangeIfValid(
      const std::string& str, std::pair<int, int>* range);

  // If 'connection' contains a range, i.e. '[a:b]', returns a set of strings
  // with that bit range separated into strings for each element in the range.
  // Works recursively if the string contains multiple ranges. If it contains
  // no ranges, returns the string itself.
  std::set<std::string> SplitConnectionByRange(const std::string& connection,
                                               size_t start_pos = 0);

  std::string StringWithoutWhitespace(const std::string& my_str);

  // Returns a vector of strings, produced by tokenizing 'str' using the
  // deliminators in 'delim'.
  std::vector<std::string> StringTokens(
      const std::string& str, const std::string delim = "\t\n\r ");

  // Gets the first token from 'str' using whitespace as deliminators.
  std::string FirstStringToken(const std::string& str);

  // Returns true if the first whitespace delimintated token in 'src' is one
  // of the strings in 'any_of'.
  bool StartsWith(const std::string& src,
                  const std::vector<std::string>& any_of);

  // Returns the largest possible substring surrounded by '(' and ')'.
  std::string GetParenWrappedSubstring(const std::string& str);

  // Separates a list of net connections from a module's connection list into
  // individual strings. Removes the .name() wrapping for connection by name.
  // Assumes that all lists will be entirely either connection by name or by
  // position.
  //
  // Note: A concatenated group of several nets, i.e. {a, b, c} counts as one
  // connection.
  std::vector<std::string> SplitConnectionList(const std::string& str);

  // Takes a bus-type connection composed of concatenated connections and
  // returns a vector containing the names of the component signals.
  std::vector<std::string> ConcatenatedComponents(
      const std::string& connection);

  // Parses a module connection list, in the form of the string 'str', and
  // returns the connection names as a set of strings.
  std::set<std::string> ParseConnectedNetNames(const std::string& str,
                                               bool debug = false);

  // Parses a string 'str' containing a module instantiation, and returns it
  // as a VlogModule.
  VlogModule VlogModuleFromLine(const std::string& str);

  // Parses a string 'str' containing one or more net declarations, and returns
  // them as a vector of VlogNets.
  std::vector<VlogNet> VlogNetsFromLine(const std::string& str);

  // Scans through the map of nets and returns the names of those that have
  // a width > 1.
  std::set<std::string> GetBusNames(
      const std::map<std::string, VlogNet>& net_container);

  // Appends an explicit range to the name of 'net' based on its bitwidth.
  std::string AddBusRange(const VlogNet& net);

  // The Consume methods return a copy of 'input' with the first token
  // removed. If 'token' is non-null, also copy the token to it. Whitespace
  // is greedily consumed before and after the token, but not included in
  // 'token' unless it is syntactically signficant to the token. Unless
  // otherwise stated, the Consume methods throw ParsingException if they
  // cannot consume the requested token.
  
  // Whitespace is a sequential combination of spaces, newlines, and tabs.
  // DOES NOT THROW EXCEPTION IF NOTHING CAN BE CONSUMED.
  static std::string ConsumeWhitespaceIfPresent(const std::string& input,
                                                std::string* token);

  // Identifier = SimpleIdentifier || EscapedIdentifier
  static std::string ConsumeIdentifier(const std::string& input,
                                       std::string* token);

  // IdentifierList = Identifier[, Identifier]*
  static std::string ConsumeIdentifierList(const std::string& input,
                                           std::string* token);

  // SimpleIdentifier = (_ || a-z)(_ || a-z || 0-9 || $)*
  static std::string ConsumeSimpleIdentifier(const std::string& input,
                                             std::string* token);

  // EscapedIdentifier = \\.+<single whitespace character>
  static std::string ConsumeEscapedIdentifier(const std::string& input,
                                              std::string* token);

  // Connection = ConnectedElement || .Identifier(ConnectedElement)
  static std::string ConsumeConnection(
      const std::string& input,
      std::string* token);

  // ConnectionList = Connection[, Connection]*
  static std::string ConsumeConnectionList(
      const std::string& input,
      std::string* token);

  // ConnectedElement = Immediate || (Identifier[BitRange] ||
  //                    {ConnectedElementList}
  static std::string ConsumeConnectedElement(const std::string& input,
                                             std::string* token);

  // ConnectedElementList = ConnectedElement[, ConnectedElement]*
  static std::string ConsumeConnectedElementList(
      const std::string& input,
      std::string* token);

  // ParameterConnectedElement = Immediate || Identifier
  static std::string ConsumeParameterConnectedElement(const std::string& input,
                                                      std::string* token);

  // ParameterConnection = ParameterConnectedElement ||
  //                       .Identifier(ParameterConnectedElement)
  static std::string ConsumeParameterConnection(const std::string& input,
                                                std::string* token);

  // ParameterList = ParameterConnection[, ParameterConnection]*
  static std::string ConsumeParameterList(const std::string& input,
                                          std::string* token);

  // ModuleParameters = #(ParameterList)
  static std::string ConsumeModuleParameters(const std::string& input,
                                             std::string* token);

  // Immediate = BinaryImmediate || DecimalImmediate || HexImmediate ||
  //             OctalImmediate || StringLiteral
  static std::string ConsumeImmediate(const std::string& input,
                                      std::string* token);

  // BinaryImmediate = (UnbasedImmedate)'b(0-1)+
  static std::string ConsumeBinaryImmediate(const std::string& input,
                                            std::string* token);

  // OctalImmediate = (UnbasedImmediate)'o(0-7)+
  static std::string ConsumeOctalImmediate(const std::string& input,
                                           std::string* token);

  // DecimalImmediate = UnbasedImmediate'd(0-9)+
  static std::string ConsumeDecimalImmediate(const std::string& input,
                                             std::string* token);

  // HexImmediate = (UnbasedImmediate)'h(0-9 || a-f || A-F)+
  static std::string ConsumeHexImmediate(const std::string& input,
                                         std::string* token);

  // UnbasedImmediate = (0-9)+
  static std::string ConsumeUnbasedImmediate(const std::string& input,
                                             std::string* token);

  // StringLiteral = ".*"
  static std::string ConsumeStringLiteral(const std::string& input,
                                          std::string* token);

  // BitRange = \[UnbasedImmediate:UnbasedImmediate\] 
  static std::string ConsumeBitRange(const std::string& input,
                                     std::string* token);


  // Char = any non-whitespace character
  static std::string ConsumeChar(const std::string& input,
                                 std::string* token, char c);

  // Consumes an element wrapped by 'open' and 'close' using 'consumer' to
  // consume the inner token. The string that is returned contains only
  // the inner token, not the wrappers.
  static std::string ConsumeWrappedElement(
      const std::string& input,
      std::string* token,
      std::string (*consumer)(const std::string&, std::string*),
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
};

#endif
