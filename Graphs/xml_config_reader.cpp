#include "xml_config_reader.h"

#include <cassert>
#include <cstring>

#include "universal_macros.h"

using namespace std;

PartitionerConfig XmlConfigReader::ReadFile(const char* filename) {
  xmlDocPtr doc;
  xmlParserCtxtPtr context; //Used for validation

  context = xmlNewParserCtxt();
  assert(context != NULL);

  doc = xmlCtxtReadFile(context, filename, NULL, XML_PARSE_DTDVALID);
  assert_b(doc != NULL) {
    printf("Error parsing XML file: %s", filename);
  }
  assert_b(context->valid != 0) {
    printf("XML validation failed for file %s.\n", filename);
    xmlFreeDoc(doc);
    xmlFreeParserCtxt(context);
  }
      
  xmlNodePtr rootNode = xmlDocGetRootElement(doc);
  assert_b(rootNode != NULL) {
    printf("Couldn't find XML root node in XML file %s", filename);
    xmlFreeDoc(doc);
    xmlFreeParserCtxt(context);
  }

  PartitionerConfig config = PopulatePartitionerConfig(rootNode);
  xmlFreeDoc(doc);
  xmlFreeParserCtxt(context);
  return config;
}

PartitionerConfig XmlConfigReader::PopulatePartitionerConfig(
    xmlNodePtr rootNodePtr) {
  assert(rootNodePtr->children != NULL);

  PartitionerConfig partitioner_config;
  xmlNodePtr curNodePtr = NULL;

  for (curNodePtr = ChildNonComment(rootNodePtr); curNodePtr;
       curNodePtr = NextNonComment(curNodePtr)) {
    if (curNodePtr->type == XML_ELEMENT_NODE) {
      if (!strcmp((char*)(curNodePtr->name), "general_configuration")) {
        PopulateGeneralConfiguration(&partitioner_config, curNodePtr);
      } else if (!strcmp((char*)(curNodePtr->name),
                         "preprocessor_configuration")) {
        PopulatePreprocessorConfiguration(&partitioner_config, curNodePtr);
      } else if (!strcmp((char*)(curNodePtr->name), "klfm_configuration")) {
        PopulateKlfmConfiguration(&partitioner_config, curNodePtr);
      } else if (!strcmp((char*)(curNodePtr->name),
                         "postprocessor_configuartion")) {
        PopulatePostprocessorConfiguration(&partitioner_config, curNodePtr);
      } else {
        assert_b(false) {
          printf("Encountered unsupported XML tag in partitioner config: %s\n",
                (char*)curNodePtr->name);
        }
      }
    } else {
      assert_b(false) {
        printf("Unexpected node type while parsing XML file.\n");
      }
    }
  }
  return partitioner_config;
}

void XmlConfigReader::PopulateGeneralConfiguration(
    PartitionerConfig* partitioner_config, xmlNodePtr myNodePtr) {
  assert(partitioner_config != NULL);
  assert(myNodePtr != NULL);
  assert(myNodePtr->type == XML_ELEMENT_NODE);
  assert(!strcmp((char*)(myNodePtr->name), "general_configuration"));
  assert(myNodePtr->children != NULL);
  for (myNodePtr = ChildNonComment(myNodePtr); myNodePtr != NULL;
       myNodePtr = NextNonComment(myNodePtr)) {
    assert(myNodePtr->name != NULL);
    if (!strcmp((char*)(myNodePtr->name),"use_ratio_as_quality_metric")) {
      partitioner_config->use_ratio_in_partition_quality = true;
    } else if (!strcmp((char*)(myNodePtr->name),"device_resource")) {
      int res_id = -1;
      int res_capacity = -1;
      int res_ratio_weight = -1;
      double res_imbalance = 0.0;
      xmlNodePtr childNodePtr = ChildNonComment(myNodePtr);
      assert(childNodePtr != NULL);
      for (; childNodePtr != NULL;
           childNodePtr = NextNonComment(childNodePtr)) {
        if (!strcmp((char*)childNodePtr->name, "resource_id")) {
          assert(childNodePtr->children != NULL);
          res_id = atoi((char*)(childNodePtr->children->content));
          if (res_id >= (int)partitioner_config->device_resource_capacities.size()) {
            partitioner_config->device_resource_capacities.resize(res_id + 1);
          }
          if (res_id >=
              (int)partitioner_config->device_resource_max_imbalances.size()) {
            partitioner_config->device_resource_max_imbalances.resize(res_id+1);
          }
          if (res_id >=
              (int)partitioner_config->device_resource_ratio_weights.size()) {
            partitioner_config->device_resource_ratio_weights.resize(res_id+1);
          }
        } else if (!strcmp((char*)childNodePtr->name, "resource_capacity")) {
          assert(childNodePtr->children != NULL);
          res_capacity = atoi((char*)(childNodePtr->children->content));
        } else if (!strcmp((char*)childNodePtr->name,
                   "resource_ratio_weight")) {
          assert(childNodePtr->children != NULL);
          res_ratio_weight = atoi((char*)(childNodePtr->children->content));
        } else if (!strcmp((char*)childNodePtr->name, "max_imbalance")) {
          assert(childNodePtr->children != NULL);
          res_imbalance = atof((char*)(childNodePtr->children->content));
        } else {
          assert_b(false) {
            printf("Unknown resource element: %s\nDid you run verification "
                   "with the DTD?\n", childNodePtr->name);
          }
        }
      }
      assert(res_id >= 0 && res_capacity >= 0 && res_ratio_weight >= 0);
      partitioner_config->device_resource_capacities[res_id] = res_capacity;
      partitioner_config->device_resource_max_imbalances[res_id] =
          res_imbalance;
      partitioner_config->device_resource_ratio_weights[res_id] =
          res_ratio_weight;
    } else {
      assert_b(false) {
        printf("Unknown general configuration element: --%s--\nDid you run "
              "verification with the DTD?\n", myNodePtr->name);
      }
    }
  }
}

void XmlConfigReader::PopulatePreprocessorConfiguration(
    PartitionerConfig* partitioner_config, xmlNodePtr myNodePtr) {
  assert(partitioner_config != NULL);
  assert(myNodePtr != NULL);
  assert(myNodePtr->type == XML_ELEMENT_NODE);
  assert(!strcmp((char*)(myNodePtr->name), "preprocessor_configuration"));
  myNodePtr = ChildNonComment(myNodePtr);
  assert(myNodePtr->children != NULL);

  partitioner_config->preprocessor_options_set = true;

  for (; myNodePtr != NULL; myNodePtr = NextNonComment(myNodePtr)) {
    assert(myNodePtr->name != NULL);
    if (!strcmp((char*)(myNodePtr->name),"all_universal_resource_strategy")) {
      partitioner_config->preprocessor_policy =
          PartitionerConfig::kAllUniversalResource;
      xmlNodePtr childPtr = ChildNonComment(myNodePtr);
      assert(childPtr != NULL);
      assert(!strcmp((char*)childPtr->name, "universal_resource_id"));
      childPtr = ChildNonComment(childPtr);
      assert(childPtr != NULL);
      partitioner_config->universal_resource_id =
          atoi((char*)(childPtr->content));
    } else if (!strcmp((char*)(myNodePtr->name),
                       "proportional_to_capacity_strategy")) {
      partitioner_config->preprocessor_policy =
          PartitionerConfig::kProportionalToCapacity;
      xmlNodePtr childPtr = ChildNonComment(myNodePtr);
      assert(childPtr != NULL);
      if (!strcmp((char*)childPtr->name, "use_total_capacity")) {
        partitioner_config->preprocessor_capacity_type =
            PartitionerConfig::kTotalCapacity;
      } else if (!strcmp((char*)childPtr->name, "use_flexible_capacity")) {
        partitioner_config->preprocessor_capacity_type =
            PartitionerConfig::kFlexibleCapacity;
      } else {
        assert_b(false) {
          printf("Unknown proportional-to-capacity element: --%s--\nDid you "
                 "run verification with the DTD?\n", childPtr->name);
        }
      }
    } else if (!strcmp((char*)(myNodePtr->name),
                       "lowest_capacity_cost_strategy")) {
      partitioner_config->preprocessor_policy =
          PartitionerConfig::kLowestCapacityCost;
      xmlNodePtr childPtr = ChildNonComment(myNodePtr);
      assert(childPtr != NULL);
      if (!strcmp((char*)childPtr->name, "use_total_capacity")) {
        partitioner_config->preprocessor_capacity_type =
            PartitionerConfig::kTotalCapacity;
      } else if (!strcmp((char*)childPtr->name, "use_flexible_capacity")) {
        partitioner_config->preprocessor_capacity_type =
            PartitionerConfig::kFlexibleCapacity;
      } else {
        assert_b(false) {
          printf("Unknown lowest-capacity-cost element: --%s--\nDid you run "
                "verification with the DTD?\n", childPtr->name);
        }
      }
    } else if (!strcmp((char*)(myNodePtr->name), "random_strategy")) {
      partitioner_config->preprocessor_policy =
          PartitionerConfig::kRandomPreprocessor;
    } else if (!strcmp((char*)(myNodePtr->name), "fixed_random_seed")) {
      partitioner_config->use_fixed_random_seed = true;
      partitioner_config->fixed_random_seed =
          atoi((char*)(myNodePtr->children->content));
    } else {
      assert_b(false) {
        printf("Unknown partition config element: --%s--\nDid you run "
              "verification with the DTD?\n", myNodePtr->name);
      }
    }
  }
}

void XmlConfigReader::PopulateKlfmConfiguration(
    PartitionerConfig* partitioner_config, xmlNodePtr myNodePtr) {
  assert(partitioner_config != NULL);
  assert(myNodePtr != NULL);
  assert(myNodePtr->type == XML_ELEMENT_NODE);
  assert(!strcmp((char*)(myNodePtr->name), "klfm_configuration"));
  myNodePtr = ChildNonComment(myNodePtr);
  assert(myNodePtr != NULL);

  partitioner_config->klfm_options_set = true;

  for (; myNodePtr != NULL; myNodePtr = NextNonComment(myNodePtr)) {
    assert(myNodePtr->name != NULL);
    if (!strcmp((char*)(myNodePtr->name),
        "use_multilevel_constraint_relaxation")) {
      partitioner_config->use_multilevel_constraint_relaxation = true;
    }
    else if (!strcmp((char*)(myNodePtr->name),"gain_bucket_type")) {
      xmlNodePtr childPtr = ChildNonComment(myNodePtr);
      assert(childPtr != NULL);
      if (!strcmp((char*)childPtr->name, "single_resource_bucket")) {
        partitioner_config->gain_bucket_type =
            PartitionerConfig::kGainBucketSingleResource;
      } else if (!strcmp((char*)childPtr->name,
                         "multi_resource_exclusive_bucket")) {
        partitioner_config->gain_bucket_type =
            PartitionerConfig::kGainBucketMultiResourceExclusive;
        childPtr = ChildNonComment(childPtr);
        assert(childPtr != NULL);
        for (; childPtr != NULL; childPtr = NextNonComment(childPtr)) {
          if (!strcmp((char*)(childPtr->name),
              "use_adaptive_implementations")) {
          partitioner_config->gain_bucket_type =
              PartitionerConfig::kGainBucketMultiResourceExclusiveAdaptive;
          partitioner_config->use_adaptive_node_implementations = true;
          } else if (!strcmp((char*)(childPtr->name),
              "use_random_resource_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
                PartitionerConfig::kGbmreSelectionPolicyRandomResource;
          } else if (!strcmp((char*)(childPtr->name),
              "use_largest_resource_imbalance_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
               PartitionerConfig::kGbmreSelectionPolicyLargestResourceImbalance;
          } else if (!strcmp((char*)(childPtr->name),
              "use_largest_unconstrained_gain_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
               PartitionerConfig::kGbmreSelectionPolicyLargestUnconstrainedGain;
          } else if (!strcmp((char*)(childPtr->name),
              "use_largest_gain_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
                PartitionerConfig::kGbmreSelectionPolicyLargestGain;
          } else {
            assert_b(false) {
              printf("Unknown partition config element: --%s--\nDid you run "
                    "verification with the DTD?\n", childPtr->name);
            }
          }
        }
      } else if (!strcmp((char*)childPtr->name,
                         "multi_resource_mixed_bucket")) {
        partitioner_config->gain_bucket_type =
            PartitionerConfig::kGainBucketMultiResourceMixed;
        childPtr = ChildNonComment(childPtr);
        assert(childPtr != NULL);
        for (; childPtr != NULL; childPtr = NextNonComment(childPtr)) {
          if (!strcmp((char*)(childPtr->name),
              "use_adaptive_implementations")) {
            partitioner_config->gain_bucket_type =
                PartitionerConfig::kGainBucketMultiResourceMixedAdaptive;
            partitioner_config->use_adaptive_node_implementations = true;
          } else if (!strcmp((char*)(childPtr->name),
              "use_random_resource_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
                PartitionerConfig::kGbmrmSelectionPolicyRandomResource;
          } else if (!strcmp((char*)(childPtr->name), "use_ratio_in_score")) {
            partitioner_config->use_ratio_in_imbalance_score = true;
          } else if (!strcmp((char*)(childPtr->name),
              "use_most_unbalanced_resource_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
                PartitionerConfig::kGbmrmSelectionPolicyMostUnbalancedResource;
          } else if (!strcmp((char*)(childPtr->name),
              "use_best_gain_imbalance_score_classic_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
                PartitionerConfig::kGbmrmSelectionPolicyBestGainImbalanceScoreClassic;
          } else if (!strcmp((char*)(childPtr->name),
            "use_best_gain_imbalance_score_with_affinities_selection_policy")) {
            partitioner_config->gain_bucket_selection_policy =
                PartitionerConfig::kGbmrmSelectionPolicyBestGainImbalanceScoreWithAffinities;
          } else {
            assert_b(false) {
              printf("Unknown partition config element: --%s--\nDid you run "
                     "verification with the DTD?\n", childPtr->name);
            }
          }
        }
      } else {
        assert_b(false) {
          printf("Unknown bucket_type element: --%s--\nDid you run "
                 "verification with the DTD?\n", childPtr->name);
        }
      }
    } else if (!strcmp((char*)myNodePtr->name,
                       "node_implementation_options")) {
      for (xmlNodePtr childPtr = ChildNonComment(myNodePtr);
           childPtr != NULL; childPtr = NextNonComment(childPtr)) {
        if (!strcmp((char*)childPtr->name,
            "restrict_supernodes_to_default_implementation")) {
          partitioner_config->restrict_supernodes_to_default_implementation = true;
        } else if (!strcmp((char*)childPtr->name,
                           "supernode_implementations_cap")) {
          partitioner_config->supernode_implementations_cap =
              atoi((char*)(childPtr->children->content));
        } else if (!strcmp((char*)childPtr->name,
                           "reuse_previous_run_implementations")) {
          partitioner_config->reuse_previous_run_implementations = true;
        } else if (!strcmp((char*)childPtr->name, "mutation_options")) {
          for (xmlNodePtr moChildPtr = ChildNonComment(childPtr);
               moChildPtr != NULL; moChildPtr = NextNonComment(moChildPtr)) {
            if (!strcmp((char*)moChildPtr->name, "mutation_rate")) {
              partitioner_config->mutation_rate =
                  atoi((char*)(moChildPtr->children->content));
            } else {
              assert_b(false) {
                printf("Unknown mutation_options element: "
                      "--%s--\nDid you run verification with the DTD?\n",
                      childPtr->name);
              }
            }
          }
        } else if (!strcmp((char*)childPtr->name, "rebalance_options")) {
          for (xmlNodePtr rebChildPtr = ChildNonComment(childPtr);
               rebChildPtr != NULL; rebChildPtr = NextNonComment(rebChildPtr)) {
            if (!strcmp((char*)rebChildPtr->name,
                        "rebalance_on_start_of_pass")) {
              partitioner_config->rebalance_on_start_of_pass = true;
            } else if (!strcmp((char*)rebChildPtr->name,
                        "rebalance_on_end_of_run")) {
              partitioner_config->rebalance_on_end_of_run = true;
            } else if (!strcmp((char*)rebChildPtr->name,
                               "rebalance_on_demand")) {
              partitioner_config->rebalance_on_demand = true;
              for (xmlNodePtr odChildPtr = ChildNonComment(rebChildPtr);
                  odChildPtr != NULL; odChildPtr = NextNonComment(odChildPtr)) {
                if (!strcmp((char*)odChildPtr->name,
                            "rebalance_on_demand_cap_per_run")) {
                  partitioner_config->rebalance_on_demand_cap_per_run =
                      atoi((char*)(odChildPtr->children->content));
                } else if (!strcmp((char*)odChildPtr->name,
                                  "rebalance_on_demand_cap_per_pass")) {
                  partitioner_config->rebalance_on_demand_cap_per_pass =
                      atoi((char*)(odChildPtr->children->content));
                } else {
                  assert_b(false) {
                    printf("Unknown rebalance_on_demand element: "
                          "--%s--\nDid you run verification with the DTD?\n",
                          odChildPtr->name);
                  }
                }
              }
            } else {
              assert_b(false) {
                printf("Unknown rebalance_options element: "
                      "--%s--\nDid you run verification with the DTD?\n",
                      rebChildPtr->name);
              }
            }
          }
        } else {
          assert_b(false) {
            printf("Unknown node_implementation_options element: "
                  "--%s--\nDid you run verification with the DTD?\n",
                  childPtr->name);
          }
        }
      }
    } else {
      assert_b(false) {
        printf("Unknown partition config element: --%s--\nDid you run "
              "verification with the DTD?\n", myNodePtr->name);
      }
    }
  }
}

void XmlConfigReader::PopulatePostprocessorConfiguration(
    PartitionerConfig* partitioner_config, xmlNodePtr myNodePtr) {

}

xmlNodePtr XmlConfigReader::NextNonComment(xmlNodePtr old_ptr) {
  if (old_ptr == NULL) return old_ptr;
  xmlNodePtr ptr = old_ptr->next;
  while (ptr && !strcmp((char*)(ptr->name), "comment")) {
    ptr = ptr->next;
  }
  return ptr;
}

xmlNodePtr XmlConfigReader::ChildNonComment(xmlNodePtr old_ptr) {
  xmlNodePtr ptr = old_ptr;
  if (ptr != NULL) {
    ptr = ptr->children;
  }
  while (ptr &&  !strcmp((char*)(ptr->name), "comment")) {
    ptr = ptr->next;
  }
  return ptr;
}
