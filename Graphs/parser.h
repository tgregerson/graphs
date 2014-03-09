#ifndef PARSER_H_
#define PARSER_H_

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "node.h"
#include "edge.h"

struct ParsedModule;

class Parser {
 public:
  typedef std::unordered_map<int, std::string> element_map;

  Parser(element_map* node_map, element_map* edge_map, int start_id = 1)
    : node_map_(node_map), edge_map_(edge_map),
      next_entity_id_(start_id) {}
  ~Parser() {}

  void ParseVerilog(std::string filename);

 private:
  struct ParsedModule {
    std::string name;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
  };

  void PopulateMapsFromParsedModules(
      const std::vector<ParsedModule>& parsed_modules,
      element_map* node_map, element_map* edge_map);

  void ParseVerilogModule(const std::vector<std::string>& module_lines,
                          ParsedModule* parsed_module);

  int next_entity_id_;
  element_map node_map;
  element_map edge_map;

};

#endif /* PARSER_H_ */
