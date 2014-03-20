#include "chaco_weight_generator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include "chaco_parser.h"
#include "edge.h"
#include "node.h"
#include "port.h"
#include "universal_macros.h"

using namespace std;

int main(int argc, char *argv[]) {

  ChacoWeightGenerator::GeneratorOptions g_options;
  g_options.resource_exclusive_nodes = true;
  g_options.num_nodes = 45087;
  g_options.num_resources = 3;

  ChacoWeightGenerator::ResourceOptions r_options1, r_options2, r_options3;
  r_options1.resource_name = "LUT";
  r_options2.resource_name = "DSP";
  r_options3.resource_name = "BRAM";

  r_options1.max_node_weight = 15;
  r_options1.min_node_weight = 5;
  r_options2.max_node_weight = 370;
  r_options2.min_node_weight = 170;
  r_options3.max_node_weight = 630;
  r_options3.min_node_weight = 230;

  // Used for Binomial Distributions only
  /*
  r_options1.mean_node_weight = 20.0;
  r_options2.mean_node_weight = 440.0;
  r_options3.mean_node_weight = 1250.0;
  r_options1.std_dev_node_weight = 4.0;
  r_options2.std_dev_node_weight = 20.0;
  r_options3.std_dev_node_weight = 30.0;
  */

  r_options1.weight_distribution_type =
      ChacoWeightGenerator::UNIFORM_WEIGHT_DIST;
      //ChacoWeightGenerator::BINOMIAL_WEIGHT_DIST;
  r_options2.weight_distribution_type =
      ChacoWeightGenerator::UNIFORM_WEIGHT_DIST;
      //ChacoWeightGenerator::BINOMIAL_WEIGHT_DIST;
  r_options3.weight_distribution_type =
      ChacoWeightGenerator::UNIFORM_WEIGHT_DIST;
      //ChacoWeightGenerator::BINOMIAL_WEIGHT_DIST;

  r_options1.node_contain_probability = 0.84;
  r_options2.node_contain_probability = 0.09;
  r_options3.node_contain_probability = 0.07;

  r_options2.resource_conversion_probability_and_multipliers.insert(
      make_pair("LUT", make_pair(0.98,make_pair(0.1, 3.0))));
  r_options3.resource_conversion_probability_and_multipliers.insert(
      make_pair("LUT", make_pair(0.95,make_pair(0.1, 2.0))));
  /*
  r_options3.resource_conversion_multipliers.insert(
      make_pair("DSP", make_pair(2.0, 3.0)));
  */

  g_options.resource_options.push_back(r_options1);
  g_options.resource_options.push_back(r_options2);
  g_options.resource_options.push_back(r_options3);

  ChacoWeightGenerator generator(g_options);
  generator.WriteWeightFile("graphs/fe_body.graph.wt");
  return 0;
}

ChacoWeightGenerator::ChacoWeightGenerator(GeneratorOptions& options)
    : gen_options_(options) {
  source_graph_ = "graphs/fe_body.graph";
  cluster_ = true;
  cluster_probability_.push_back(0.0);
  cluster_probability_.push_back(0.7);
  cluster_probability_.push_back(0.9);
  random_engine_.seed(time(NULL));
  num_resources_ = gen_options_.num_resources;
  vector<ResourceOptions>& resource_options_vec = gen_options_.resource_options;
  assert(gen_options_.num_resources > 0);
  assert(resource_options_vec.size() == gen_options_.num_resources);
  for (size_t res_id = 0; res_id < resource_options_vec.size(); ++res_id) {
    ResourceOptions& resource_options = resource_options_vec[res_id];
    if (!resource_options.Validate()) {
      printf("Failed to validate resource options for %s\n",
             resource_options.resource_name.c_str());
      exit(1);
    }
    resource_name_to_id_.insert(make_pair(resource_options.resource_name,
                                          res_id));
    resource_id_to_name_.insert(make_pair(res_id,
                                          resource_options.resource_name));
    resource_options_map_.insert(make_pair(resource_options.resource_name,
                                           resource_options));
  }

  if (gen_options_.resource_exclusive_nodes) {
    double total_probability = 0;
    for (auto& it : gen_options_.resource_options) {
      total_probability += it.node_contain_probability;
    }
    if (total_probability > 1.05 || total_probability < 0.95) {
      // Technically it should be exactly 1.0, but we allow for some rounding
      // error due to the use of floating point.
      printf("When specifying resource exclusive nodes, the sum of the resource"
             " probabilities must be approximately 1.0. Actual: %f\n",
             total_probability);
      exit(1);
    }
  }
}

bool ChacoWeightGenerator::ResourceOptions::Validate() {
  if (resource_name.empty()) {
    printf("Resource name must be specified.\n");
    return false;
  }

  if (max_node_weight < 1) {
    printf("Invalid max node weight: %d\n", max_node_weight);
    return false;
  }

  if (min_node_weight < 1) {
    printf("Invalid min node weight: %d\n", min_node_weight);
    return false;
  } else if (min_node_weight > max_node_weight) {
    printf("Min node weight cannot exceed max node weight.\n");
    return false;
  }

  if (node_contain_probability > 1.0 || node_contain_probability < 0.0) {
    printf("Invalid probability: %f\n", node_contain_probability);
    return false;
  }

  switch (weight_distribution_type) {
    case UNIFORM_WEIGHT_DIST: break;
    case BINOMIAL_WEIGHT_DIST:
      {
        double variance = std_dev_node_weight * std_dev_node_weight;
        if (variance > mean_node_weight) {
          printf("Invalid standard deviation: %f. For binomial distributions, "
                 "the std dev squared (%f) cannot exceed the mean (%f).\n",
                 std_dev_node_weight, variance, mean_node_weight);
        }
        int truncated_mean = (int)mean_node_weight;
        if (truncated_mean > max_node_weight ||
            truncated_mean < min_node_weight) {
          printf("Invalid mean node weight: %f. Must be between min (%d) and "
                 "max (%d).\n", mean_node_weight, min_node_weight,
                 max_node_weight);
        }
        if (min_node_weight != 0) {
          printf("Warning: Min Node Weight for resource %s is non-zero (%d) "
                 "and binomial distribution is selected. A non-zero minimum "
                 "weight may skew the mean and standard deviation.\n",
                 resource_name.c_str(), min_node_weight);
        }
        double bin_probability = 1.0 - (variance / mean_node_weight);
        int bin_max = (int)(mean_node_weight / bin_probability);
        if (bin_max > max_node_weight) {
          printf("Warning: Max Node Weight for resource %s is %d, which is "
                 "less than the ideal max for the binomial distribution "
                 "with the specified mean and std deviation (%d). Enforcing "
                 "this max weight will skew the distribution.\n",
                 resource_name.c_str(), max_node_weight, bin_max);
        } else if (bin_max < max_node_weight) {
          printf("Info: Max Node Weight for resource %s is %d, which is "
                 "to meet the specified mean and standard deviation for a "
                 "binomial distribution, the actual max weight will be capped "
                 "at %d.\n", resource_name.c_str(), max_node_weight, bin_max);
        }
      }
    break;
    default:
        printf("Unrecognized weight distribution type\n");
        return false;
  }

  for (auto& it : resource_conversion_probability_and_multipliers) {
    if (it.first.empty()) {
      printf("Resource conversion table has empty key\n");
      return false;
    }
    if (it.second.first > 1.0 || it.second.first < 0.0) {
      printf("Invalid resource conversion probability.\n");
      return false;
    }
    if (it.second.second.first <= 0.0 || it.second.second.second <= 0.0 ||
        it.second.second.first > it.second.second.second) {
      printf("Resource conversion specifies invalid value for %s\n",
             it.first.c_str());
      return false;
    }
  }

  return true;
}

void ChacoWeightGenerator::WriteWeightFile(const std::string& filename) {

  ofstream output_file;
  output_file.open(filename.c_str());
  assert(!output_file.bad());

  // Write header line.
  output_file << gen_options_.num_nodes << " " << gen_options_.num_resources
              << endl;

  vector<string> node_strings;

  if (gen_options_.resource_exclusive_nodes) {
    node_strings = GenerateNodeStringsExclusive();
  } else {
    printf("Error: Non-exclusive resource nodes not yet supported.\n");
    exit(1);
  }

  // Write each node line.
  for (auto& node_string : node_strings) {
    output_file << node_string << endl;
    assert(!output_file.bad());
  }

  output_file.close();
}


vector<string> ChacoWeightGenerator::GenerateNodeStringsExclusive() {
  // Assign resource types to nodes. Do this by creating a vector that is the
  // size of the total number of nodes, containing resource identifiers in
  // proportion to the resource probabilities specified in the options. Then
  // perform a random shuffle of the vector.

  vector<int> node_resource_types;
  vector<int> resource_node_counts;
  resource_node_counts.insert(resource_node_counts.begin(),
                              gen_options_.num_resources, 0);
  for (auto& it : resource_options_map_) {
    ResourceOptions& res_options = it.second;
    assert(resource_name_to_id_.count(res_options.resource_name) == 1);
    int res_id = resource_name_to_id_[res_options.resource_name];
    assert(res_id < num_resources_);
    int expected_number_of_nodes =
        (int)ceil(gen_options_.num_nodes *
        res_options.node_contain_probability);
    for (int i = 0; i < expected_number_of_nodes; ++i) {
      node_resource_types.push_back(res_id);
    }
    resource_node_counts[res_id] = expected_number_of_nodes;
    shuffle(node_resource_types.begin(), node_resource_types.end(),
            random_engine_);
  }

  // Account for the fact that the vector may not be the exact right size
  // due to rounding.
  while (node_resource_types.size() < gen_options_.num_nodes) {
    // Assuming the number of nodes is relatively large compared to the
    // undershoot, this should grow the vector consistent with its resource
    // probabilities.
    // NOTE: If the number of nodes is (within an order of magnitude of the
    // maximum integer value), doing modulo may not provide uniformity.
    assert(node_resource_types.size() != 0);
    int res_pos = random_engine_() % node_resource_types.size();
    int res_id_to_add = node_resource_types[res_pos];
    node_resource_types.push_back(res_id_to_add);
    resource_node_counts[res_id_to_add]++;
  }
  if (node_resource_types.size() > gen_options_.num_nodes) {
    for (size_t overshot_pos = gen_options_.num_nodes;
         overshot_pos < node_resource_types.size(); overshot_pos++) {
      resource_node_counts[node_resource_types[overshot_pos]]--; 
    }
    node_resource_types.resize(gen_options_.num_nodes);
  }

  if (cluster_) {
    // This attempts to use structural information from the graph
    // to give a certain probability that a node in a given resource type
    // will have at least one neighbor with the same resource type.
    Node graph = Node(9999999, "Graph");
    {
      ChacoParser parser;
      assert(parser.Parse(&graph, source_graph_.c_str()));
      assert(graph.internal_nodes().size() == gen_options_.num_nodes);
    }
    set<int> unassigned_node_ids;
    vector<int> node_id_assignment_order;
    for (auto it : graph.internal_nodes()) {
      unassigned_node_ids.insert(it.first);
      node_id_assignment_order.push_back(it.first);
    }
    shuffle(node_id_assignment_order.begin(), node_id_assignment_order.end(),
            random_engine_);
    int index = 0;
    vector<set<int>> assigned_node_ids_by_res_id;
    assigned_node_ids_by_res_id.resize(num_resources_);
    vector<set<int>> possibly_free_neighbors_by_res_id;
    possibly_free_neighbors_by_res_id.resize(num_resources_);
    assert(node_resource_types.size() == node_id_assignment_order.size());
    for (size_t i = 0; i < node_resource_types.size(); i++) {
      int res_id = node_resource_types[i];
      while (unassigned_node_ids.find(node_id_assignment_order[index]) ==
             unassigned_node_ids.end()) {
        index++;
        assert(index < node_id_assignment_order.size());
      }
      int next_node_id = node_id_assignment_order[index];
      double rand_prob = (double)rand()/(double)RAND_MAX;
      if (rand_prob < cluster_probability_[res_id] &&
          !possibly_free_neighbors_by_res_id[res_id].empty()) {
        vector<int> passed_ids;
        for (auto p_node_id : possibly_free_neighbors_by_res_id[res_id]) {
          passed_ids.push_back(p_node_id);
          if (unassigned_node_ids.find(p_node_id) !=
              unassigned_node_ids.end()) {
            next_node_id = p_node_id;
            break;
          }
        }
        for (auto passed_id : passed_ids) {
          possibly_free_neighbors_by_res_id[res_id].erase(passed_id);
        }
      }
      assigned_node_ids_by_res_id[res_id].insert(next_node_id);
      unassigned_node_ids.erase(next_node_id);
      Node* node = graph.internal_nodes().at(next_node_id);
      for (auto& port_pair : node->ports()) {
        int edge_id = port_pair.second.external_edge_id;
        Edge* edge = graph.internal_edges().at(edge_id);
        for (auto cnx_node_id : edge->connection_ids()) {
          if (unassigned_node_ids.find(cnx_node_id) !=
              unassigned_node_ids.end()) {
            possibly_free_neighbors_by_res_id[res_id].insert(cnx_node_id);
          }
        }
      }
    }
    // This should be an OK way to map from graph node id to line number,
    // because the CHACO parser assigns node ids in the same order as
    // lines.
    sort(node_id_assignment_order.begin(), node_id_assignment_order.end());
    node_resource_types.clear();
    for (size_t i = 0; i < node_id_assignment_order.size(); i++) {
      int node_id = node_id_assignment_order[i];
      for (int res_id = 0; res_id < num_resources_; res_id++) {
        if (assigned_node_ids_by_res_id[res_id].find(node_id) !=
            assigned_node_ids_by_res_id[res_id].end()) {
          node_resource_types.push_back(res_id);
          break;
        }
      }
    }
  }


  // Next, determine the weight of each resource that is assigned to each node
  // of that type. This is done by creating vectors the size of the number
  // of nodes of each resource type and filling the entries with weights
  // according to the specified weight distribution types
  // from the resource options.
  // Eventually they are matched with entry order in the vector of node resource
  // types by their corresponding type.
  vector<vector<int>> resource_per_node_weights;
  resource_per_node_weights.resize(num_resources_);
  for (int res_id = 0; res_id < num_resources_; ++res_id) {
    assert(resource_id_to_name_.count(res_id) == 1);
    const string& resource_name = resource_id_to_name_[res_id];
    assert(resource_options_map_.count(resource_name) == 1);
    const ResourceOptions& res_options = resource_options_map_[resource_name];

    switch (res_options.weight_distribution_type) {
      case UNIFORM_WEIGHT_DIST:
        {
          std::uniform_int_distribution<int> dist(res_options.min_node_weight,
                                                  res_options.max_node_weight);
          for (int i = 0; i < resource_node_counts[res_id]; ++i) {
            resource_per_node_weights[res_id].push_back(dist(random_engine_));
          }
        }
      break;
      case BINOMIAL_WEIGHT_DIST:
        {
          double variance = res_options.std_dev_node_weight *
                            res_options.std_dev_node_weight;
          double probability = 1.0 - variance / res_options.mean_node_weight;
          assert(probability >= 0.0 && probability <= 1.0);
          int bin_max = (int)(res_options.mean_node_weight / probability);
          std::binomial_distribution<int> dist(bin_max, probability);
          for (int i = 0; i < resource_node_counts[res_id]; ++i) {
            int retries = 0;
            int node_weight = dist(random_engine_);
            while (node_weight < res_options.min_node_weight ||
                   node_weight > res_options.max_node_weight) {
              node_weight = dist(random_engine_);
              retries++;
              assert_b(retries < 1000) {
                printf("\nExperienced 1000 consecutive binomial-generated "
                       "weights that were outside of the specified min-max "
                       "range. The mean, standard deviation, min, or max "
                       "weights are likely configured to a value that doesn't "
                       "make sense for a binomial distribution. "
                       "Terminating to avoid potenial infinite loop.\n\n");
              }
            }
            resource_per_node_weights[res_id].push_back(node_weight);
          }
        }
      break;
      default:
        printf("Unsupported weight distribution type selected.\n");
        exit(1);
    }
  }

  // Generate the node strings.

  vector<string> node_strings;
  // The vector is of nodes.
  // The map has resource ID as key and weight in that resource as value.
  // NOTE: This is assuming all implementations only include one type of
  // resource per implementation.
  vector<map<int,int>> node_weights;
  node_weights.resize(gen_options_.num_nodes);
  for (int node_num = 0; node_num < gen_options_.num_nodes; ++node_num) {
    int res_id = node_resource_types[node_num];
    assert(!resource_per_node_weights[res_id].empty());
    int res_weight = resource_per_node_weights[res_id].back();
    resource_per_node_weights[res_id].pop_back();
    node_weights[node_num].insert(make_pair(res_id, res_weight));
  }
  for (auto it : resource_per_node_weights) {
    assert(it.empty());
  }

  // Perform resource conversions and add them as additional implementation
  // options.
  for (auto& node_implementations : node_weights) {
    for (auto impl_it = node_implementations.begin();
         impl_it != node_implementations.end(); ) { // Note lack of ++
      const int res_id = impl_it->first;
      const int res_weight = impl_it->second;
      const string& res_name = resource_id_to_name_[res_id];
      assert(resource_options_map_.count(res_name) == 1);
      const ResourceOptions& res_options = resource_options_map_[res_name];
      bool added_impl = false;
      for (auto& conv_entry :
           res_options.resource_conversion_probability_and_multipliers) {
        double conv_rand = (double)rand() / RAND_MAX;
        if (conv_rand <= conv_entry.second.first) {
          assert(resource_name_to_id_.count(conv_entry.first) == 1);
          const int conv_id = resource_name_to_id_[conv_entry.first];
          double mult = (double)rand() / RAND_MAX;
          mult *= (conv_entry.second.second.second -
                   conv_entry.second.second.first);
          mult += conv_entry.second.second.first;
          int conv_weight = (int)ceil(res_weight * mult);
          if (node_implementations.count(conv_id) == 0) {
            // A conversion exists to an implementation we don't already have.
            // Add it.
            // This insert invalidates impl_it!
            node_implementations.insert(make_pair(conv_id, conv_weight));
            impl_it = node_implementations.begin();
            added_impl = true;
          } else if (conv_weight < node_implementations[conv_id]) {
            // If there is a cheaper conversion possible, use it.
            node_implementations[conv_id] = conv_weight;
          }
        }
      }
      if (!added_impl) {
        impl_it++;
      }
    }
  }

  // Write node strings.
  for (auto& node_implementations : node_weights) {
    ostringstream node_ss;
    for (auto impl_it = node_implementations.begin();
         impl_it != node_implementations.end(); impl_it++) {
      for (int res_id = 0; res_id < num_resources_; res_id++) {
        if (res_id != 0 || impl_it != node_implementations.begin()) {
          node_ss << " ";
        }
        if (impl_it->first == res_id) {
          node_ss << impl_it->second;
        } else {
          node_ss << 0;
        }
      }
    }
    node_strings.push_back(node_ss.str());
  }
  return node_strings;
}
