#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "id_manager.h"
#include "chaco_parser.h"
#include "node.h"
#include "partition_engine.h"
#include "partition_engine_klfm.h"
#include "preprocessor.h"
#include "ntl_parser.h"
#include "testbench_generator.h"
#include "tclap/CmdLine.h"
#include "xml_config_reader.h"

using namespace std;

class KlfmRunConfig {
 public:
  KlfmRunConfig()
    : num_runs(1),
      num_ways(2),
      graph_file_type(kChacoGraph),
      export_initial_sol_only(false),
      sol_scip_format(true),
      sol_gurobi_format(false)
    {}
  enum GraphFileType {
    kChacoGraph,
    kNtlGraph,
    kXntlGraph,
  };
  PartitionerConfig partitioner_config;
  int num_runs;
  int num_ways;
  string graph_filename;
  GraphFileType graph_file_type;
  string result_filename;
  string log_filename;
  string testbench_filename;
  string initial_sol_base_filename;
  string final_sol_base_filename;
  bool export_initial_sol_only;
  bool sol_scip_format;
  bool sol_gurobi_format;
};

void print_usage_and_exit();

KlfmRunConfig ConfigFromCommandLineOptions(int argc, char* argv[]);

void RepartitionKway(int num_ways, int cur_lev, Node* graph,
    const vector<set<int>>& starting_partitions,
    PartitionEngineKlfm::Options& options,
    vector<int>* results_this_run,
    vector<double>* rms_devs_this_run,
    ostream& os);

void PrintPreamble(ostream& os, const string& input_filename) {
  time_t now = time(NULL);
  os << ctime(&now) << endl;
  os << input_filename << endl << endl;
}

string clean_edge_string(const string& my_str) {
  size_t pos = my_str.find("_split_");
  if (pos != string::npos) {
    string new_str = my_str;
    new_str.resize(pos);
    return new_str;
  }
  return my_str;
}

int main(int argc, char *argv[]) {
  xmlKeepBlanksDefault(0);

  if (argc == 1) {
    print_usage_and_exit();
  }

  KlfmRunConfig run_config = ConfigFromCommandLineOptions(argc, argv);

  ofstream result_file;
  ofstream log_file;

  if (!run_config.result_filename.empty()) {
    result_file.open(run_config.result_filename.c_str(),
                     ios_base::out | ios_base::app);
    assert(result_file.is_open() && result_file.good());
  }
  if (!run_config.log_filename.empty()) {
    log_file.open(run_config.log_filename.c_str(),
                  ios_base::out | ios_base::app);
    assert(log_file.is_open() && log_file.good());
  }
  ostream& rs = result_file.is_open() ? result_file : cout;
  ostream& ls = log_file.is_open() ? log_file : cout;

  PrintPreamble(rs, run_config.graph_filename);

  map<int, string> edge_id_name_map;
  Node* graph;
  const int kAlot = 1000000000;
  graph = new Node(kAlot, "Top-Level Graph");
  {
    if (run_config.graph_file_type == KlfmRunConfig::kNtlGraph ||
        run_config.graph_file_type == KlfmRunConfig::kXntlGraph) {
      ls << "Invoking Processed Netlist Parser" << endl;
      double ver = 1.0;
      if (run_config.graph_file_type == KlfmRunConfig::kXntlGraph) {
        ver = 2.0;
      }
      run_config.num_ways = 2;
      NtlParser parser(ver);
      parser.Parse(graph, run_config.graph_filename.c_str(), &edge_id_name_map);
    } else {
      ls << "Invoking Chaco Parser" << endl;
      ChacoParser parser;
      assert(parser.Parse(graph, run_config.graph_filename.c_str()));
    }
  }
  run_config.partitioner_config.ValidateOrDie(graph);

  {
    Preprocessor preprocessor(run_config.partitioner_config);
    preprocessor.ProcessGraph(graph);
  }

  PartitionEngineKlfm::Options options;
  options.PopulateFromPartitionerConfig(run_config.partitioner_config);
  options.num_runs = run_config.num_runs;
  options.initial_sol_base_filename = run_config.initial_sol_base_filename;
  options.final_sol_base_filename = run_config.final_sol_base_filename;
  options.export_initial_sol_only = run_config.export_initial_sol_only;
  options.sol_scip_format = run_config.sol_scip_format;
  options.sol_gurobi_format = run_config.sol_gurobi_format;

  run_config.partitioner_config.PrintPreprocessorOptions(rs);
  options.Print(rs);

  vector<int> total_weights = graph->SelectedWeightVector();
  ls << "Inital Graph Weight after Preprocessing: ";
  for (auto it : total_weights) {
    ls << it << " ";
  }
  ls << endl;
  ls << "Percent capacity: ";
  for (size_t i = 0; i < total_weights.size(); i++) {
    ls << int(100 * (double(total_weights[i]) /
                     double(run_config.partitioner_config.device_resource_capacities[i]))) << "% ";
  }
  ls << endl;

  bool k_way = run_config.num_ways > 2;
  ls << "Create partitioner" << endl;
  vector<PartitionSummary> summaries;
  {
    PartitionEngineKlfm klfm_partitioner(graph, options, rs);

    if (!k_way) {
      delete graph;
    }

    ls << "Execute partitioner" << endl;
    klfm_partitioner.Execute(&summaries);
  }
  ls << "Partitioning Complete" << endl;
  PrintPreamble(rs, run_config.graph_filename);

  if (k_way) {
    vector<vector<int>> costs_by_run;
    vector<vector<double>> rms_devs_by_run;
    for (size_t result_num = 0; result_num < summaries.size(); result_num++) {
      vector<int> results_this_run;
      vector<double> rms_devs_this_run;
      rs << endl << "Executing K-Way Partitioning for Result " << result_num
         << endl;
      RepartitionKway(run_config.num_ways, 4, graph,
          summaries[result_num].partition_node_ids, options, &results_this_run,
          &rms_devs_this_run, rs);
      costs_by_run.push_back(results_this_run);
      rms_devs_by_run.push_back(rms_devs_this_run);
    }
    for (size_t i = 0; i < summaries.size(); i++) {
      costs_by_run[i].insert(costs_by_run[i].begin(), summaries[i].total_cost);
      for (size_t j = 1; j < costs_by_run[i].size(); j++) {
        costs_by_run[i][j] += costs_by_run[i][j-1]; 
      }
      rms_devs_by_run[i].insert(rms_devs_by_run[i].begin(),
          summaries[i].rms_resource_deviation);
    }
    rs << "Mincosts:" << endl;
    for (size_t i = 0; i < costs_by_run[0].size(); i++) {
      int min_val = costs_by_run[0][i];
      double min_val_rms_dev = rms_devs_by_run[0][i];
      for (size_t j = 0; j < costs_by_run.size(); j++) {
        if (costs_by_run[j][i] < min_val) {
          min_val = costs_by_run[j][i];
          min_val_rms_dev = rms_devs_by_run[j][i];
        }
      }
      rs << min_val << " " << min_val_rms_dev << endl;
    }
    delete graph;
  }

  if (!run_config.testbench_filename.empty()) {
    set<string> cutset_edge_names;
    for (auto it : summaries) {
      if (!it.partition_edge_names.empty()) {
        for (auto edge_name : it.partition_edge_names) {
          cutset_edge_names.insert(clean_edge_string(edge_name));
        }
      } else if (run_config.graph_file_type == KlfmRunConfig::kNtlGraph) {
        for (auto edge_id : it.partition_edge_ids) {
          if (edge_id_name_map.find(edge_id) == edge_id_name_map.end()) {
            rs << "WARNING: Could not find name for edge " << edge_id << endl;
          } else {
            cutset_edge_names.insert(edge_id_name_map[edge_id]);
          }
        }
      }
    }

    TestbenchGenerator::GenerateVerilog(
        run_config.testbench_filename, cutset_edge_names);
  }
  return 0;
}

void RepartitionKway(int num_ways, int cur_lev, Node* graph,
    const vector<set<int>>& starting_partitions,
    PartitionEngineKlfm::Options& options,
    vector<int>* results_this_run,
    vector<double>* rms_devs_this_run,
    ostream& os) {
  options.num_runs = 1;
  options.enable_print_output = false;
  vector<set<int>> next_level_partitions;
  Node graph_copy(graph);
  // Remove all edges that span the previous partition from the graph.
  set<int> edges_to_remove;
  for (auto edge_pair : graph_copy.internal_edges()) {
    // Currently only supports graphs, not hypergraphs.
    assert(edge_pair.second->connection_ids().size() == 2);
    int home_part = -1;
    int first_node = *(edge_pair.second->connection_ids().begin());
    for (size_t part_num = 0; part_num < starting_partitions.size(); part_num++) {
      if (starting_partitions[part_num].find(first_node) !=
          starting_partitions[part_num].end()) {
        home_part = part_num;
        break;
      }
    }
    assert(home_part >= 0);
    bool remove = false;
    for (auto node_id : edge_pair.second->connection_ids()) {
      if (starting_partitions[home_part].find(node_id) ==
          starting_partitions[home_part].end()) {
        // Edge spans partitions.
        remove = true;
        break;
      }
    }
    if (remove) {
      edges_to_remove.insert(edge_pair.first);
      for (auto node_id : edge_pair.second->connection_ids()) {
        graph_copy.internal_nodes().at(node_id)->RemoveConnection(
            edge_pair.first);
      }
      delete edge_pair.second;
    }
  }
  for (auto edge_id : edges_to_remove) {
    graph_copy.internal_edges().erase(edge_id);
  }

  // Bipartiion all starting partitions.
  vector<PartitionSummary> all_summaries;
  for (size_t part_num = 0; part_num < starting_partitions.size(); part_num++) {
    Node* starting_graph = new Node(-1, "");
    auto& partition_node_id_set = starting_partitions[part_num];
    for (auto node_id : partition_node_id_set) {
      Node* move_node = graph_copy.internal_nodes().at(node_id);
      starting_graph->AddInternalNode(node_id, move_node);
      graph_copy.internal_nodes().erase(node_id);
      for (auto& port_pair : move_node->ports()) {
        int edge_id = port_pair.second.external_edge_id;
        if (graph_copy.internal_edges().find(edge_id) !=
            graph_copy.internal_edges().end()) {
          Edge* move_edge = graph_copy.internal_edges().at(edge_id);
          starting_graph->AddInternalEdge(edge_id, move_edge);
          graph_copy.internal_edges().erase(edge_id);
        }
      }
    }
    vector<PartitionSummary> my_summary;
    cout << "Create partitioner " << part_num << "/" << cur_lev << endl;
    {
      PartitionEngineKlfm klfm_partitioner(starting_graph, options, os);
      cout << "Execute partitioner" << endl;
      klfm_partitioner.Execute(&my_summary);
    }
    cout << "Partitioner " << (part_num + 1) << "/" << (cur_lev/2) << " Complete" << endl;
    all_summaries.push_back(my_summary[0]);
    delete starting_graph;
  }
  vector<set<int>> next_level_starting_partitions;
  int total_cost = 0;
  double rms_sum = 0;
  for (auto& it : all_summaries) {
    next_level_starting_partitions.push_back(it.partition_node_ids[0]);
    next_level_starting_partitions.push_back(it.partition_node_ids[1]);
    total_cost += it.total_cost;
    rms_sum += it.rms_resource_deviation;
  }
  double rms_avg = rms_sum / all_summaries.size();
  os << "RESULT: Total cost at level " << cur_lev << ": " << total_cost << endl;
  results_this_run->push_back(total_cost);
  rms_devs_this_run->push_back(rms_avg);

  int next_lev = cur_lev * 2;
  if (next_lev <= num_ways) {
    RepartitionKway(num_ways, next_lev, graph, next_level_starting_partitions,
        options, results_this_run, rms_devs_this_run, os);
  }
}

KlfmRunConfig ConfigFromCommandLineOptions(int argc, char* argv[]) {
  // Parse command-line arguments
  TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

  vector<TCLAP::Arg*> input_file_args;
  TCLAP::ValueArg<string> chaco_input_file_flag(
      "c", "chaco", "CHACO-format input file name", false, "", "string");
  input_file_args.push_back(&chaco_input_file_flag);

  TCLAP::ValueArg<string> ntl_input_file_flag(
      "n", "ntl", "NTL-format input file name", false, "", "string");
  input_file_args.push_back(&ntl_input_file_flag);

  cmd.xorAdd(input_file_args);

  TCLAP::ValueArg<string> config_input_file_flag(
      "x", "config", "Config XML-format input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<int> num_runs_flag(
      "r", "nruns", "Number of runs to execute", false, 1, "int", cmd);

  TCLAP::ValueArg<int> num_ways_flag(
      "w", "nways", "Number of ways to partition", false, 2, "int", cmd);

  TCLAP::ValueArg<string> result_output_file_flag(
      "o", "resultfile", "Output file for results", false, "", "string",
      cmd);

  TCLAP::ValueArg<string> log_output_file_flag(
      "l", "logfile", "Output file for log messages", false, "", "string",
      cmd);

  TCLAP::ValueArg<string> testbench_output_file_flag(
      "t", "export-testbench",
      "Output file for cutset monitoring testbench skeleton", false, "",
      "string", cmd);

  TCLAP::ValueArg<string> export_initial_sol_output_file_flag(
      "i", "export-initial-sol", "Base output filename for initial solution",
      false, "", "string", cmd);

  TCLAP::ValueArg<string> export_final_sol_output_file_flag(
      "f", "export-final-sol", "Base output filename for final solution", false,
      "", "string", cmd);

  TCLAP::SwitchArg initial_sol_only_switch(
      "s", "initial-sol-only", "Quit after exporting initial solution", cmd,
      false);

  TCLAP::SwitchArg sol_scip_format_switch(
      "", "sol-scip-format", "Write solution in SCIP's .SOL format", cmd,
      false);

  TCLAP::SwitchArg sol_gurobi_format_switch(
      "", "sol-gurobi-format", "Write solution in Gurobi's .MST format", cmd,
      false);

  cmd.parse(argc, argv);

  KlfmRunConfig run_config;

  if (chaco_input_file_flag.isSet()) {
    run_config.graph_file_type = KlfmRunConfig::kChacoGraph;
    run_config.graph_filename = chaco_input_file_flag.getValue();
  } else {
    run_config.graph_file_type = KlfmRunConfig::kNtlGraph;
    run_config.graph_filename = ntl_input_file_flag.getValue();
  }

  PartitionerConfig config;
  {
    XmlConfigReader config_reader;
    config = config_reader.ReadFile(config_input_file_flag.getValue().c_str());
  }
  run_config.partitioner_config = config;

  run_config.num_runs = num_runs_flag.getValue();
  run_config.num_ways = num_ways_flag.getValue();
  run_config.result_filename = result_output_file_flag.getValue();
  run_config.log_filename = log_output_file_flag.getValue();
  run_config.testbench_filename = testbench_output_file_flag.getValue();
  run_config.initial_sol_base_filename =
      export_initial_sol_output_file_flag.getValue();
  run_config.final_sol_base_filename =
      export_final_sol_output_file_flag.getValue();
  run_config.export_initial_sol_only = initial_sol_only_switch.isSet();
  if (run_config.export_initial_sol_only &&
      run_config.initial_sol_base_filename.empty()) {
    cout << "Must provide a filename to export initial SOL";
    exit(1);
  }
  run_config.sol_gurobi_format = sol_gurobi_format_switch.isSet();
  run_config.sol_scip_format = sol_scip_format_switch.isSet() ||
                               !run_config.sol_gurobi_format;
  return run_config;
}

void print_usage_and_exit() {
  cout << "Usage: partition_main REQUIRED_ARGS [OPTIONS*]" << endl
       << endl
       << "REQUIRED_ARGS: " << endl
       << "(--chaco chaco_graph_input_file_path | "
       << "--ntl ntl_graph_input_file_path)" << endl
       << "--config config_xml_input_file_path" << endl
       << endl
       << "OPTIONS:" << endl
       << "--help" << endl
       << "--nruns              int_val               (default: 1)" << endl
       << "--nways              int_val               (default: 2)" << endl
       << "--resultfile         output_file_path      (default: std::out)" << endl
       << "--logfile            output_file_path      (default: std::out)" << endl
       << "--export-testbench   output_file_path      (default: none)" << endl
       << "--export-initial-sol output_file_base_path (default: none)" << endl
       << "--export-final-sol   output_file_base_path (default: none)" << endl
       << "--initial-sol-only                         (default: false)" << endl
       << "--sol-scip-format                          (default: true*)" << endl
       << "                                            *If no other sol format" << endl
       << "--sol-gurobi-format                        (default: false)" << endl
       << endl;
  exit(1);
}
