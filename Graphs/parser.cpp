#include "parser.h"

using std::string;
using std::unordered_map;
using std::vector;

void Parser::PopulateMapsFromParsedModules(
    const std::vector<ParsedModule>& parsed_modules,
    element_map* node_map, element_map* edge_map) {

  unordered_map<string, int> edge_name_to_id;

  for (auto& module : parsed_modules) {
    node_map->insert(make_pair(next_element_id++, module.name));
    for (auto& input_name : module.input_names) {
      if (edge_map->count(input_name) == 0) {
        edge_name_to_id.insert(make_pair(input_name, next_element_id++));
      }
    }
    for (auto& output_name : module.output_names) {
      if (edge_map->count(output_name) == 0) {
        edge_name_to_id.insert(make_pair(output_name, next_element_id++));
      }
    }
  }
  for (auto& it : edge_name_to_id) {
    edge_map->insert(make_pair(it.second, it.first));
  }
}

void Parser::ParseVerilog(string filename) {

  vector<vector<string>> modules_as_text;
  vector<ParsedModule> parsed_modules;

  // TODO: Open file, loop through extracting modules as vectors of lines,
  // close file.

  for (auto& module_text : modules_as_text) {
    ParsedModule parsed_module;
    ParseVerilogModule(module_text, &parsed_module);
    ParsedModules.push_back(parsed_module);
  }

  PopulateMapsFromParsedModules(parsed_modules, &node_map, &edge_map);
}

void Parser::ParseVerilogModule(const vector<string>& module_lines,
                                ParsedModule* parsed_module) {

}
