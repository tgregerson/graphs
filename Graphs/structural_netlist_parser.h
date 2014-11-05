#ifndef STRUCTURAL_NETLIST_PARSER_H_
#define STRUCTURAL_NETLIST_PARSER_H_

#include <cassert>
#include <cstdint>

#include <exception>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class FunctionalEdge;
class FunctionalNode;

class VlogNet {
 public:
  VlogNet(const std::string& name, int w = 1, int hi = 0, int lo = 0)
    : name(name), width(w), bit_hi(hi), bit_lo(lo) {}
  std::string name;
  int width;
  int bit_hi;
  int bit_lo;
  // Included for debug reasons.
  std::string src_line;
};

class VlogModule {
 public:
  VlogModule(const std::string& type, const std::string& inst)
    : type_name(type), instance_name(inst) {}

  double ComputeEntropy() const;

  std::string type_name;
  std::string instance_name;
  std::set<std::string> connected_net_names;
  // Included for debug reasons.
  std::string src_line;
  std::string src_line_post_strip;
  std::string src_connection_list;

  // Extended fields
  std::map<std::string, std::string> named_parameters;
  std::vector<std::string> ordered_parameters;
  // TODO: Map may be the wrong data structure; may need a multimap after
  // expanding buses.
  std::map<std::string, std::string> named_connections;
  std::vector<std::string> ordered_connections;
};

class StructuralNetlistParser {
 public:
  typedef std::pair<std::string,std::string> NamedConnection;
  typedef std::map<std::string,std::string> NamedConnectionMap;
  typedef std::multimap<std::string,std::string> NamedConnectionMultiMap;

  StructuralNetlistParser() {}
  ~StructuralNetlistParser() {}

  // Scans through map of nets and identifies busses. Finds these bus
  // connections in the module map, and replaces those connections with
  // expanded single-bit connections.
  void ExpandModuleBusConnections(
      const std::map<std::string, VlogNet>& nets,
      std::map<std::string, VlogModule>* module_container);
  void ExpandBusConnections(
      const std::map<std::string, VlogNet>& nets,
      NamedConnectionMultiMap* connections);

  // Populates a map, module name -> VlogModule, from a list of lines. Determines
  // if the line contains a module instantiation if it doesn't start with a set
  // of keywords. This assumes that a module instantiation is not split between
  // lines.
  void PopulateModules(std::map<std::string, VlogModule>* modules,
                       const std::list<std::string>& lines);
  void PopulateFunctionalNodes(
      const std::map<std::string, FunctionalEdge*>& edges,
      const std::map<std::string, FunctionalEdge*>& wires,
      std::map<std::string, FunctionalNode*>* nodes,
      const std::list<std::string>& lines);

  // Populates a map, net name -> VlogNet, from a list of lines. Determines
  // if the line contains a net declaration if it starts with a net
  // keyword. This assumes that a net declaration is not split between lines.
  void PopulateNets(std::map<std::string, VlogNet>* nets,
                    const std::list<std::string>& lines);
  void PopulateFunctionalEdges(std::map<std::string, FunctionalEdge*>* edges,
                    const std::list<std::string>& lines);

  // Adds ports to 'edges' based on the connections in 'nodes'.
  void PopulateFunctionalEdgePorts(
      const std::map<std::string, FunctionalNode*>& nodes,
      std::map<std::string, FunctionalEdge*>* edges,
      std::map<std::string, FunctionalEdge*>* wires);

  // Prints debug information about a VlogModule.
  void PrintModule(const VlogModule& module);

  // Prints a module in the .NTL format used by Processed Netlist Parser.
  void PrintModuleNtlFormat(const VlogModule& module,
                            const std::map<std::string, VlogNet>& nets,
                            std::ostream& output_stream);

  // Prints a module in the Extended NTL format.
  void PrintFunctionalNodeXNtlFormat(FunctionalNode* node,
                                     std::map<std::string, FunctionalEdge*>* edges,
                                     std::map<std::string, FunctionalEdge*>* wires,
                                     std::map<std::string, FunctionalNode*>* nodes,
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
  void RemoveWires(
      std::map<std::string, FunctionalNode*>* nodes,
      std::map<std::string, FunctionalEdge*>* wires,
      const std::set<std::string>& wires_to_remove);

  // Remove connections from a given net name from modules if it contains a
  // substring. Useful for removing 'UNCONNECTED' nets.
  void RemoveNetConnectionFromModulesSubstring(
      std::map<std::string, VlogModule>* modules,
      const std::set<std::string>& nets_to_remove_substring);
  void RemoveWiresBySubstring(
      std::map<std::string, FunctionalNode*>* nodes,
      std::map<std::string, FunctionalEdge*>* wires,
      const std::set<std::string>& substrings);

  // Removes nodes with no connections.
  void RemoveUnconnectedNodes(std::map<std::string, FunctionalNode*>* nodes);

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
  FunctionalNode* FunctionalNodeFromLine(
      const std::map<std::string, FunctionalEdge*>& edges,
      const std::map<std::string, FunctionalEdge*>& wires,
      const std::string& str);

  // Parses a string 'str' containing one or more net declarations, and returns
  // them as a vector of VlogNets.
  std::vector<VlogNet> VlogNetsFromLine(const std::string& str);
  // Caller takes ownership of pointers.
  std::vector<FunctionalEdge*> FunctionalEdgesFromLine(const std::string& str);

  // Scans through the map of nets and returns the names of those that have
  // a width > 1.
  std::set<std::string> GetBusNames(
      const std::map<std::string, VlogNet>& net_container);
  std::set<std::string> GetBusNames(
      const std::map<std::string, FunctionalEdge*>& edges);

  // Appends an explicit range to the name of 'net' based on its bitwidth.
  std::string AddBusRange(const VlogNet& net);

};

#endif
