#ifndef CHACO_WEIGHT_GENERATOR_H_
#define CHACO_WEIGHT_GENERATOR_H_

#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

class ChacoWeightGenerator {
 public:
  class GeneratorOptions;

  typedef enum {
    UNIFORM_WEIGHT_DIST,
    BINOMIAL_WEIGHT_DIST,
  } WeightDistributionType;

  ChacoWeightGenerator(GeneratorOptions& options);
  ~ChacoWeightGenerator() {}

  void WriteWeightFile(const std::string& filename);

  class ResourceOptions {
   public:
    bool Validate();

    std::string resource_name;
    size_t max_node_weight;
    // NOTE: The min_node_weight is the lowest weight of this resource a node
    // may have IFF it has the resource, so it must be greater than zero.
    size_t min_node_weight;
    // Required for Binomial. Ignored for Uniform.
    double mean_node_weight;
    // Required for Binomial. Ignored for Uniform.
    double std_dev_node_weight;
    WeightDistributionType weight_distribution_type;
    double node_contain_probability;
    // If the resource can be converted to a different type, that type will
    // have an entry in this map. KEY: Name of other resource. VALUE: A 
    // constant that the weight of this resource must be muliplied by to
    // get the weight in the other resource. The first double is the minimum
    // multiplier and the second one is the maximum multiplier.
    std::map<std::string, std::pair<double,std::pair<double,double>>>
        resource_conversion_probability_and_multipliers;
  };

  class GeneratorOptions {
   public:
    bool resource_exclusive_nodes;
    size_t num_nodes;
    size_t num_resources;
    std::vector<ResourceOptions> resource_options;
  };

 private:
  bool cluster_;
  std::vector<double> cluster_probability_;
  std::string source_graph_;
  // Generate a vector containing strings for each line entry in the weight
  // file.
  std::vector<std::string> GenerateNodeStringsExclusive();

  size_t num_resources_;
  GeneratorOptions gen_options_;
  std::default_random_engine random_engine_;
  std::map<std::string, ResourceOptions> resource_options_map_;
  // NOTE: resource IDs are constrained to 0~num_resources.
  std::map<std::string, int> resource_name_to_id_;
  std::map<int, std::string> resource_id_to_name_;
};

#endif /* CHACO_WEIGHT_GENERATOR_H_ */
