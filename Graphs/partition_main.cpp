#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
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
#include "processed_netlist_parser.h"
#include "testbench_generator.h"
#include "xml_config_reader.h"

using namespace std;

char* input_filename;
ofstream log_file;

void RepartitionKway(int num_ways, int cur_lev, Node* graph,
    const vector<set<int>>& starting_partitions,
    PartitionEngineKlfm::Options& options,
    vector<int>* results_this_run,
    vector<double>* rms_devs_this_run,
    ostream& os);

void PrintPreamble(ostream& os) {
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

  if (argc < 4 || argc > 6) {
    printf("\nIncorrect number of arguments: %d\n", argc);
    printf("Use: partition <graph> <config.xml> num_runs "
           "[result_output_file] [result_log_file]\n\n");
    exit(1);
  }

  ofstream output_file;
  ofstream log_file;

  input_filename = argv[1];
  string config_file_path = argv[2];
  int num_runs = atoi(argv[3]);
  if (argc > 4) {
    string output_filename = argv[4];
    if (!output_filename.empty()) {
      output_file.open(output_filename.c_str(), ios_base::out | ios_base::app);
      assert(output_file.is_open() && output_file.good());
    }
  }
  if (argc > 5) {
    string log_filename = argv[5];
    if (!log_filename.empty()) {
      log_file.open(log_filename.c_str(), ios_base::out | ios_base::app);
      assert(output_file.is_open() && output_file.good());
    }
  }
  bool k_way = true;
  int num_ways = 32;

  ostream& os = output_file.is_open() ? output_file : cout;
  ostream& ls = log_file.is_open() ? log_file : cout;
  PrintPreamble(os);

  string input_filename_str = input_filename;
  bool is_ntl = input_filename_str.find(".ntl") != string::npos;
  map<int, string> edge_id_name_map;

  Node* graph;
  graph = new Node(9999999, "Top-Level Graph");
  {
    string input_filename_str = input_filename;
    if (is_ntl) {
      printf("Invoking Processed Netlist Parser\n");
      // TODO k_way currently not supported for hypergraphs.
      k_way = false;
      ProcessedNetlistParser parser;
      parser.Parse(graph, input_filename, &edge_id_name_map);
    } else {
      printf("Invoking Chaco Parser\n");
      ChacoParser parser;
      assert(parser.Parse(graph, input_filename));
    }
  }

  PartitionerConfig config;
  {
    XmlConfigReader config_reader;
    config = config_reader.ReadFile(config_file_path.c_str());
  }
  config.ValidateOrDie(graph);

  {
    Preprocessor preprocessor(config);
    preprocessor.ProcessGraph(graph);
  }

  PartitionEngineKlfm::Options options;
  options.PopulateFromPartitionerConfig(config);
  options.num_runs = num_runs;

  config.PrintPreprocessorOptions(os);
  options.Print(os);

  vector<int> total_weights = graph->SelectedWeightVector();
  ls << "Inital Graph Weight after Preprocessing: ";
  for (auto it : total_weights) {
    ls << it << " ";
  }
  ls << endl;
  ls << "Percent capacity: ";
  for (int i = 0; i < total_weights.size(); i++) {
    ls << int(100 * (double(total_weights[i]) /
                     double(config.device_resource_capacities[i]))) << "% ";
  }
  ls << endl;

  ls << "Create partitioner" << endl;
  vector<PartitionSummary> summaries;
  {
    PartitionEngineKlfm klfm_partitioner(graph, options, os);

    if (!k_way) {
      delete graph;
    }

    ls << "Execute partitioner" << endl;
    klfm_partitioner.Execute(&summaries);
  }
  ls << "Partitioning Complete" << endl;
  PrintPreamble(os);

  if (k_way) {
    vector<vector<int>> costs_by_run;
    vector<vector<double>> rms_devs_by_run;
    for (int result_num = 0; result_num < summaries.size(); result_num++) {
      vector<int> results_this_run;
      vector<double> rms_devs_this_run;
      os << endl << "Executing K-Way Partitioning for Result " << result_num
         << endl;
      RepartitionKway(num_ways, 4, graph,
          summaries[result_num].partition_node_ids, options, &results_this_run,
          &rms_devs_this_run, os);
      costs_by_run.push_back(results_this_run);
      rms_devs_by_run.push_back(rms_devs_this_run);
    }
    for (int i = 0; i < summaries.size(); i++) {
      costs_by_run[i].insert(costs_by_run[i].begin(), summaries[i].total_cost);
      for (int j = 1; j < costs_by_run[i].size(); j++) {
        costs_by_run[i][j] += costs_by_run[i][j-1]; 
      }
      rms_devs_by_run[i].insert(rms_devs_by_run[i].begin(),
          summaries[i].rms_resource_deviation);
    }
    os << "Mincosts:" << endl;
    for (int i = 0; i < costs_by_run[0].size(); i++) {
      int min_val = costs_by_run[0][i];
      double min_val_rms_dev = rms_devs_by_run[0][i];
      for (int j = 0; j < costs_by_run.size(); j++) {
        if (costs_by_run[j][i] < min_val) {
          min_val = costs_by_run[j][i];
          min_val_rms_dev = rms_devs_by_run[j][i];
        }
      }
      os << min_val << " " << min_val_rms_dev << endl;
    }
    delete graph;
  }

  set<string> cutset_edge_names;
  for (auto it : summaries) {
    if (!it.partition_edge_names.empty()) {
      os << "Printing edge names from summary." << endl;
      for (auto edge_name : it.partition_edge_names) {
        cutset_edge_names.insert(clean_edge_string(edge_name));
      }
    } else if (is_ntl) {
      os << "Printing edge names from id map." << endl;
      for (auto edge_id : it.partition_edge_ids) {
        if (edge_id_name_map.find(edge_id) == edge_id_name_map.end()) {
          os << "WARNING: Could not find name for edge " << edge_id << endl;
        } else {
          cutset_edge_names.insert(edge_id_name_map[edge_id]);
        }
      }
    }
  }

  TestbenchGenerator::GenerateVerilog("", cutset_edge_names);
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
    for (int part_num = 0; part_num < starting_partitions.size(); part_num++) {
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
  for (int part_num = 0; part_num < starting_partitions.size(); part_num++) {
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
