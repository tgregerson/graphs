#ifndef PROCESSED_NETLIST_PARSER_H_
#define PROCESSED_NETLIST_PARSER_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

struct ParsedModule {
  std::string type_name;
  std::string instance_name;
  std::vector<std::pair<std::string, double>> connected_edge_names_and_entropies;
};

class Node;

class NtlParser {
 public:
  // 'ver' specifies NTL version number in use.
  explicit NtlParser(double ver = 1.0) : ver_(ver) {}

  // Opens the file 'filename', reads it in NTL format, and populates the
  // internal nodes and edges of 'graph' with the contents.
  //
  // To conserve memory, edges are only given numerical IDs.
  // If 'edge_id_to_name' is non-null, it will be populated with mappings
  // from edge IDs to their instance names from the netlist.
  void Parse(Node* graph, const char* filename,
             std::map<int, std::string>* edge_id_to_name);

 private:
  void PopulateGraph(Node* graph,
                     std::map<int, std::string>* edge_id_to_name);

  void PopulateModuleTypeImplementationsMap();

  // Prints the implementation map for debug purposes.
  void PrintImplementationMap();

  std::vector<ParsedModule> parsed_modules_;
  std::map<std::string,std::vector<std::vector<int>>> module_type_implementations_map_;

  const double ver_;
};


#endif // PROCESSED_NETLIST_PARSER_H_
