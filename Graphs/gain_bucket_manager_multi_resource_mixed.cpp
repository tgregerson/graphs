#include "gain_bucket_manager_multi_resource_mixed.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

#include "universal_macros.h"
#include "weight_score.h"

using namespace std;

GainBucketEntry
    GainBucketManagerMultiResourceMixed::GetNextGainBucketEntry(
        const vector<int>& current_balance, const vector<int>& total_weight) {

  // How do we select which bucket to examine?
  //
  // 1. Random
  // 2. Resource with greatest imbalance pct
  // 3. Highest gain from all unconstrained buckets
  // 4. Highest gain from all buckets (limit search in constrained buckets)
  GainBucketEntry entry;
  switch (selection_policy_) {
    case PartitionerConfig::kGbmrmSelectionPolicyRandomResource:
      entry = GetNextGainBucketEntryRandomResource(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using RANDOM RESOURCE policy." << endl;
    break;
    case PartitionerConfig::kGbmrmSelectionPolicyMostUnbalancedResource:
      entry = GetNextGainBucketEntryMostUnbalancedResource(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using MOST UNBALANCED policy." << endl;
    break;
    case PartitionerConfig::kGbmrmSelectionPolicyBestGainImbalanceScoreClassic:
      entry = GetNextGainBucketEntryBestGainImbalanceScoreClassic(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using GAIN IMBALANCE CLASSIC policy." <<
          endl;
    break;
    case PartitionerConfig::kGbmrmSelectionPolicyBestGainImbalanceScoreWithAffinities:
      entry = GetNextGainBucketEntryBestGainImbalanceScoreWithAffinities(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using GAIN IMBALANCE AFFINITY policy." <<
          endl;
    break;
    default:
      assert_b(false) {
        printf("Unrecognized selection policy for multi-resource-mixed "
               "gain bucket.\n");
      }
  }

  RUN_DEBUG(DEBUG_OPT_BUCKET_NODE_SELECT, 0) {
    cout << "Returning entry with node ID: " << entry.Id() << endl;
    cout << "Returning entry with gain: " << entry.CostGain() << endl;
    cout << "Returning entry with weight: ";
    for (auto wt : entry.current_weight_vector()) {
      cout << wt << " ";
    }
    cout << endl;
  }

  // Remove any duplicate entries from other gain buckets.
  // TODO Is this call still necessary?
  RemoveNode(entry.Id());
  return entry;
}

void GainBucketManagerMultiResourceMixed::RemoveNode(int node_id) {
  int erased = node_id_to_resource_index_.erase(node_id);
  if (erased > 0) {
    for (auto& bucket : gain_buckets_a_) {
      if (bucket->HasNode(node_id)) {
        GainBucketEntry debug = bucket->RemoveByNodeId(node_id);
        assert(debug.Id() == node_id);
      }
    }
    for (auto& bucket : gain_buckets_b_) {
      if (bucket->HasNode(node_id)) {
        GainBucketEntry debug = bucket->RemoveByNodeId(node_id);
        assert(debug.Id() == node_id);
      }
    }
    if (gain_bucket_a_master_->HasNode(node_id)) {
      GainBucketEntry debug = gain_bucket_a_master_->RemoveByNodeId(node_id);
      assert(debug.Id() == node_id);
    } else {
      GainBucketEntry debug = gain_bucket_b_master_->RemoveByNodeId(node_id);
      assert(debug.Id() == node_id);
    }
  }
}

GainBucketEntry GainBucketManagerMultiResourceMixed::GetNextGainBucketEntryRandomResource(
    const vector<int>& current_balance, const vector<int>& total_weight) {
  vector<int> non_empty_indices;
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    if (!(gain_buckets_a_[i]->Empty() && gain_buckets_b_[i]->Empty())) {
      non_empty_indices.push_back(i);
    }
  }
  assert(!non_empty_indices.empty());
  int rand_val = random_engine_() % non_empty_indices.size();
  int bucket_index = non_empty_indices[rand_val];

  GainBucketInterface* bucket_a = gain_buckets_a_[bucket_index];
  GainBucketInterface* bucket_b = gain_buckets_b_[bucket_index];

  GainBucketEntry entry = SelectBetweenBucketsByImbalancePower(
      bucket_a, bucket_b, current_balance, total_weight,
      max_bucket_search_depth_);
  return entry;
}

GainBucketEntry GainBucketManagerMultiResourceMixed::GetNextGainBucketEntryMostUnbalancedResource(
    const vector<int>& current_balance, const vector<int>& total_weight) {
  double largest_frac = 0.0;
  int resource_index = 0;
  const vector<int>& max_weight_imbalance =
      GetMaxImbalanceRef(max_imbalance_fraction_, total_weight);
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    if (!(gain_buckets_a_[i]->Empty() &&
          gain_buckets_b_[i]->Empty())) {
      double weight_frac =
          abs((double)current_balance[i] / (double)max_weight_imbalance[i]);
      if (weight_frac >= largest_frac) {
        resource_index = i;
        largest_frac = weight_frac;
      }
    }
  }

  GainBucketInterface* bucket_a = gain_buckets_a_[resource_index];
  GainBucketInterface* bucket_b = gain_buckets_b_[resource_index];

  GainBucketEntry entry = SelectBetweenBucketsByImbalancePower(
      bucket_a, bucket_b, current_balance, total_weight,
      max_bucket_search_depth_);
  return entry;
}

GainBucketEntry GainBucketManagerMultiResourceMixed::GetNextGainBucketEntryBestGainImbalanceScoreClassic(
    const vector<int>& current_balance, const vector<int>& total_weight) {
  assert(!(gain_bucket_a_master_->Empty() && gain_bucket_b_master_->Empty()));
  GainBucketEntry entry_a, entry_b;
  double entry_a_imbalance_power = numeric_limits<double>::max();
  double entry_b_imbalance_power = numeric_limits<double>::max();
  if (!gain_bucket_a_master_->Empty()) {
    entry_a = gain_bucket_a_master_->Top();
    // TODO Currently, don't use violater.
    // Might want to experiment with this.
    if (use_adaptive_) {
      entry_a_imbalance_power = SetBestWeightVectorByImbalancePower(
          &entry_a, current_balance, total_weight, true, false);
    } else {
      // Use ViolatorImbalancePower for non-adaptive case as it may be
      // easier to get stuck in violation.
      entry_a_imbalance_power = ImbalancePowerIfMoved(
          entry_a.current_weight_vector(), current_balance, total_weight,
          true, true);
    }
  }
  if (!gain_bucket_b_master_->Empty()) {
    entry_b = gain_bucket_b_master_->Top();
    if (use_adaptive_) {
      entry_b_imbalance_power = SetBestWeightVectorByImbalancePower(
          &entry_b, current_balance, total_weight, false, false);
    } else {
      entry_b_imbalance_power = ImbalancePowerIfMoved(
          entry_b.current_weight_vector(), current_balance, total_weight,
          false, true);
    }
  }
  bool use_entry_a;
  if (gain_bucket_a_master_->Empty()) {
    use_entry_a = false;
  } else if (gain_bucket_b_master_->Empty()) {
    use_entry_a = true;
  } else {
    double gain_imbalance_a =
      ComputeGainImbalanceFn(entry_a.CostGain(), entry_a_imbalance_power);
    double gain_imbalance_b =
      ComputeGainImbalanceFn(entry_b.CostGain(), entry_b_imbalance_power);
    // Use the bucket with the lower GainImblance. If they are tied, use
    // the one with more nodes waiting to be moved.
    if (gain_imbalance_a == gain_imbalance_b) {
      if (gain_bucket_a_master_->num_entries() >
          gain_bucket_b_master_->num_entries()) {
        use_entry_a = true;
      } else {
        use_entry_a = false;
      }
    } else if (gain_imbalance_a < gain_imbalance_b) {
      use_entry_a = true;
    } else {
      use_entry_a = false;
    }
  }
  if (use_entry_a) {
    RemoveNode(entry_a.Id());
    return entry_a;
  } else {
    RemoveNode(entry_b.Id());
    return entry_b;
  }
}

GainBucketEntry GainBucketManagerMultiResourceMixed::GetNextGainBucketEntryBestGainImbalanceScoreWithAffinities(
    const vector<int>& current_balance, const vector<int>& total_weight) {
  struct CandidateEntry {
    bool from_part_a;
    int resource_index;
    // Lower score is better.
    double score;
    GainBucketEntry* entry;
  };
  vector<CandidateEntry> candidate_entries;
  for (size_t res_index = 0; res_index < gain_buckets_a_.size(); res_index++) {
    // Part A buckets.
    size_t depth;
    for (depth = 0; depth < max_bucket_search_depth_; depth++) {
      GainBucketEntry* gbe = gain_buckets_a_[res_index]->PeekPtr(depth);
      if (nullptr == gbe) {
        break;
      }
      CandidateEntry candidate;
      candidate.from_part_a = true;
      candidate.resource_index = res_index;
      candidate.entry = gbe;
      // Don't Use ViolaterImbalancePower
      double imbalance_power =  ImbalancePowerIfMoved(
          candidate.entry->current_weight_vector(), current_balance,
          total_weight, true, false);
      if (use_ratio_ && use_adaptive_) {
        imbalance_power += RatioPowerIfChangedByEntry(
            *candidate.entry, total_weight);
      }
      candidate.score =
          ComputeGainImbalanceFn(candidate.entry->CostGain(), imbalance_power);
      candidate_entries.push_back(candidate);
    }
    // Part B buckets.
    for (depth = 0; depth < max_bucket_search_depth_; depth++) {
      GainBucketEntry* gbe = gain_buckets_b_[res_index]->PeekPtr(depth);
      if (nullptr == gbe) {
        break;
      }
      CandidateEntry candidate;
      candidate.from_part_a = false;
      candidate.resource_index = res_index;
      candidate.entry = gbe;
      double imbalance_power =  ImbalancePowerIfMoved(
          candidate.entry->current_weight_vector(), current_balance,
          total_weight, false, false);
      if (use_ratio_ && use_adaptive_) {
        imbalance_power +=
            RatioPowerIfChangedByEntry(*candidate.entry, total_weight);
      }
      candidate.score =
          ComputeGainImbalanceFn(candidate.entry->CostGain(), imbalance_power);
      candidate_entries.push_back(candidate);
    }
  }

  // Get best candidate by score (lower is better).
  size_t best_score_index = 0;
  double best_score = candidate_entries[0].score;
  for (size_t i = 1; i < candidate_entries.size(); i++) {
    double cur_score = candidate_entries[i].score;
    if ((cur_score < best_score) || ((cur_score == best_score) &&
       // Although the gain is already incorporated in the score, we also want
       // to handle the special case where both scores are zero because the
       // partition is perfectly balanced.
       (candidate_entries[best_score_index].entry->CostGain() <
        candidate_entries[i].entry->CostGain()))) {
      best_score = cur_score;
      best_score_index = i;
    }
  }
  GainBucketEntry selected_entry = std::move(*(candidate_entries[best_score_index].entry));
  DLOG(DEBUG_OPT_BUCKET_NODE_SELECT_GAIN_IMBALANCE_WITH_AFFINITIES, 0) <<
      "Selected entry from affinity " <<
      candidate_entries[best_score_index].resource_index << endl;

  // Return unchosen candidates to their buckets.
  for (size_t i = 0; i < candidate_entries.size(); i++) {
    auto& candidate = candidate_entries[i];
    if (i == best_score_index) {
      if (candidate.from_part_a) {
        gain_buckets_a_[candidate.resource_index]->RemoveByNodeId(candidate.entry->Id());
      } else {
        gain_buckets_b_[candidate.resource_index]->RemoveByNodeId(candidate.entry->Id());
      }
    } else {
      if (candidate.from_part_a) {
        gain_buckets_a_[candidate.resource_index]->Touch(candidate.entry->Id());
      } else {
        gain_buckets_b_[candidate.resource_index]->Touch(candidate.entry->Id());
      }
    }
  }
  RemoveNode(selected_entry.Id());
  return selected_entry;
}

GainBucketEntry GainBucketManagerMultiResourceMixed::SelectBetweenBucketsByImbalancePower(
    GainBucketInterface* bucket_a, GainBucketInterface* bucket_b,
    const vector<int>& current_balance, const vector<int>& total_weight,
    int search_depth) {
  assert(search_depth > 0);

  // Handle the case where one of the buckets is empty.
  if (bucket_a->Empty()) {
    assert(!bucket_b->Empty()); // DEBUG
    GainBucketEntry bucket_b_entry = bucket_b->Top();
    bucket_b->Pop();
    RemoveNode(bucket_b_entry.Id());
    return bucket_b_entry;
  } else if (bucket_b->Empty()) {
    assert(!bucket_a->Empty()); // DEBUG
    GainBucketEntry bucket_a_entry = bucket_a->Top();
    bucket_a->Pop();
    RemoveNode(bucket_a_entry.Id());
    return bucket_a_entry;
  }

  vector<pair<double,GainBucketEntry>> bucket_a_entries;
  vector<pair<double,GainBucketEntry>> bucket_b_entries;

  for (int i = 0; i < search_depth; i++) {
    GainBucketEntry bucket_a_entry = bucket_a->Top();
    bucket_a->Pop();
    double imbalance_power = ImbalancePowerIfMoved(
        bucket_a_entry.current_weight_vector(), current_balance, total_weight,
        true, true);
    // TODO: Does it make sense to include ratio here? Might be better off
    // leaving it out and letting rebalance operations take care of it, since
    // we are not selecting between different implementations of the same node
    // here.
    if (use_ratio_ && use_adaptive_) {
      imbalance_power +=
          RatioPowerIfChangedByEntry(bucket_a_entry, total_weight);
    }
    bucket_a_entries.push_back(make_pair(imbalance_power, bucket_a_entry));
    // Equality check on double is OK based on definition of ImbalancePower().
    // If calculation is changed, will want to do a range-based check.
    if (imbalance_power == 0 || bucket_a->Empty()) {
      break;
    }
  }
  for (int i = 0; i < search_depth; i++) {
    GainBucketEntry bucket_b_entry = bucket_b->Top();
    bucket_b->Pop();
    double imbalance_power = ImbalancePowerIfMoved(
        bucket_b_entry.current_weight_vector(), current_balance, total_weight,
        false, true);
    if (use_ratio_ && use_adaptive_) {
      imbalance_power +=
          RatioPowerIfChangedByEntry(bucket_b_entry, total_weight);
    }
    bucket_b_entries.push_back(make_pair(imbalance_power, bucket_b_entry));
    if (imbalance_power == 0 || bucket_b->Empty()) {
      break;
    }
  }

  size_t bucket_a_best_index = 0;
  for (size_t i = 1; i < bucket_a_entries.size(); i++) {
    // Important to use strictly greater-than here, as it will make sure that
    // we keep the highest gain entry when ties occur, since all of the entries
    // are sorted in descending gain order.
    if (bucket_a_entries[i].first >
        bucket_a_entries[bucket_a_best_index].first) {
      bucket_a_best_index = i;
    }
  }
  size_t bucket_b_best_index = 0;
  for (size_t i = 1; i < bucket_b_entries.size(); i++) {
    if (bucket_b_entries[i].first >
        bucket_b_entries[bucket_b_best_index].first) {
      bucket_b_best_index = i;
    }
  }

  // If both have the same violator imbalance power, choose the one with higher
  // gain. Otherwise, choose the one with lower imbalance power.
  // This means we will always pick an entry that fits over one that doesn't.
  bool use_bucket_a_candidate;
  pair<double, GainBucketEntry>& bucket_a_candidate =
      bucket_a_entries[bucket_a_best_index];
  pair<double, GainBucketEntry>& bucket_b_candidate =
      bucket_b_entries[bucket_b_best_index];
  if (bucket_a_candidate.first == bucket_b_candidate.first) {
    if (bucket_a_candidate.second.CostGain() >
        bucket_b_candidate.second.CostGain()) {
      use_bucket_a_candidate = true;
    } else {
      use_bucket_a_candidate = false;
    }
  } else {
    if (bucket_a_candidate.first < bucket_b_candidate.first) {
      use_bucket_a_candidate = true;
    } else {
      use_bucket_a_candidate = false;
    }
  }

  GainBucketEntry selected_entry;
  for (size_t i = 0; i < bucket_a_entries.size(); i++) {
    if (use_bucket_a_candidate && (i == bucket_a_best_index)) {
      selected_entry = bucket_a_entries[i].second;
    } else {
      // Return unselected entry to bucket.
      bucket_a->Add(bucket_a_entries[i].second);
    }
  }
  for (size_t i = 0; i < bucket_b_entries.size(); i++) {
    if (!use_bucket_a_candidate && (i == bucket_b_best_index)) {
      selected_entry = bucket_b_entries[i].second;
    } else {
      // Return unselected entry to bucket.
      bucket_b->Add(bucket_b_entries[i].second);
    }
  }
  return selected_entry;
}

int GainBucketManagerMultiResourceMixed::NumUnlockedNodes() const {
  return gain_bucket_a_master_->num_entries() +
         gain_bucket_b_master_->num_entries();
}

bool GainBucketManagerMultiResourceMixed::Empty() const {
  return NumUnlockedNodes() == 0;
}

void GainBucketManagerMultiResourceMixed::AddNode(double gain, Node* node,
    bool in_part_a, const std::vector<int>& total_weight) {
  GainBucketEntry entry(gain, node);
  if (in_part_a) {
    gain_bucket_a_master_->Add(entry);
  } else {
    gain_bucket_b_master_->Add(entry);
  }
  if (use_adaptive_) {
    // For each resource, add a maximum of one entry, representing the weight
    // vector that has affinity with that resource AND the highest weight in
    // that resource.
    vector<int> res_to_wv_index;
    res_to_wv_index.assign(num_resources_per_node_, -1);
    for (size_t i = 0; i < node->WeightVectors().size(); i++) {
      auto& weight_vector = node->WeightVectors()[i];
      int resource_affinity =
          DetermineResourceAffinity(weight_vector, total_weight);
      int res_wt = weight_vector[resource_affinity];
      int prev_idx = res_to_wv_index[resource_affinity];
      if (prev_idx < 0) {
        //if (res_wt > 0) {
          res_to_wv_index[resource_affinity] = i;
        //}
      } else if (res_wt > node->WeightVectors()[prev_idx][resource_affinity]) {
        res_to_wv_index[resource_affinity] = i;
      }
    }
    for (size_t i = 0; i < num_resources_per_node_; i++) {
      if (res_to_wv_index[i] >= 0) {
        entry.SetCurrentWeightVectorIndex(res_to_wv_index[i]);
        AddEntry(entry, i, in_part_a);
      }
    }
  } else {
    int resource_affinity = DetermineResourceAffinity(
        entry.current_weight_vector(), total_weight);
    AddEntry(entry, resource_affinity, in_part_a);
  }
}

void GainBucketManagerMultiResourceMixed::AddEntry(
    const GainBucketEntry& entry, int associated_resource, bool in_part_a) {
  assert(entry.current_weight_vector().size() == num_resources_per_node_);
  if (in_part_a) {
    gain_buckets_a_[associated_resource]->Add(entry);
  } else {
    gain_buckets_b_[associated_resource]->Add(entry);
  }
  node_id_to_resource_index_.insert(make_pair(entry.Id(), associated_resource));
}

int GainBucketManagerMultiResourceMixed::DetermineResourceAffinity(
    const vector<int>& weight_vector, const std::vector<int>& total_weight) {
  int max_res_frac_idx = 0;
  double max_res_frac = 0.0;
  const vector<int>& max_weight_imbalance =
      GetMaxImbalanceRef(max_imbalance_fraction_, total_weight);
  for (size_t res = 0; res < weight_vector.size(); res++) {
    double res_frac =
      (double)weight_vector[res] / (double)max_weight_imbalance[res];
    if (res_frac > max_res_frac) {
      max_res_frac_idx = res;
      max_res_frac = res_frac;
    }
  }
  return max_res_frac_idx;
}

void GainBucketManagerMultiResourceMixed::UpdateGains(
    double gain_modifier, const vector<int>& nodes_to_increase_gain,
    const vector<int>& nodes_to_decrease_gain, bool moved_from_part_a) {
  for (auto& v : temp_nodes_to_decrease_gain_by_resource_) {
    v.resize(0);
  }
  for (auto& v : temp_nodes_to_increase_gain_by_resource_) {
    v.resize(0);
  }
  for (auto id : nodes_to_increase_gain) {
    //assert(node_id_to_resource_index_.count(id) != 0);
    auto it_pair = node_id_to_resource_index_.equal_range(id);
    for (auto it = it_pair.first; it != it_pair.second; it++) {
      temp_nodes_to_increase_gain_by_resource_[it->second].push_back(id);
    }
  }
  for (auto id : nodes_to_decrease_gain) {
    //assert(node_id_to_resource_index_.count(id) != 0);
    auto it_pair = node_id_to_resource_index_.equal_range(id);
    for (auto it = it_pair.first; it != it_pair.second; it++) {
      temp_nodes_to_decrease_gain_by_resource_[it->second].push_back(id);
    }
  }
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    if (!temp_nodes_to_increase_gain_by_resource_[i].empty()) {
      if (moved_from_part_a) {
        gain_buckets_a_[i]->UpdateGains(gain_modifier, temp_nodes_to_increase_gain_by_resource_[i]);
      } else {
        gain_buckets_b_[i]->UpdateGains(gain_modifier, temp_nodes_to_increase_gain_by_resource_[i]);
      }
    }
    if (!temp_nodes_to_decrease_gain_by_resource_[i].empty()) {
      if (moved_from_part_a) {
        gain_buckets_b_[i]->UpdateGains(-gain_modifier, temp_nodes_to_decrease_gain_by_resource_[i]);
      } else {
        gain_buckets_a_[i]->UpdateGains(-gain_modifier, temp_nodes_to_decrease_gain_by_resource_[i]);
      }
    }
  }
  if (moved_from_part_a) {
    gain_bucket_a_master_->UpdateGains(gain_modifier, nodes_to_increase_gain);
    gain_bucket_b_master_->UpdateGains(-gain_modifier, nodes_to_decrease_gain);
  } else {
    gain_bucket_b_master_->UpdateGains(gain_modifier, nodes_to_increase_gain);
    gain_bucket_a_master_->UpdateGains(-gain_modifier, nodes_to_decrease_gain);
  }
}

void GainBucketManagerMultiResourceMixed::UpdateNodeImplementation(Node* node) {
  auto res_it = node_id_to_resource_index_.find(node->id);
  if (res_it == node_id_to_resource_index_.end()) {
    return;
  }
  bool in_part_a = gain_bucket_a_master_->HasNode(node->id);
  if (in_part_a) {
    GainBucketEntry& gbe = gain_bucket_a_master_->GbeRefByNodeId(node->id);
    gbe.SetCurrentWeightVectorIndex(node->selected_weight_vector_index());
  } else {
    GainBucketEntry& gbe = gain_bucket_b_master_->GbeRefByNodeId(node->id);
    gbe.SetCurrentWeightVectorIndex(node->selected_weight_vector_index());
  }
  // We don't update the implementation for the resource-affinity buckets.
  // The reason for this is that operations that trigger changes to node
  // implementations (mutation, rebalancing) also change the total weight of
  // the graph, which is needed to determine affinity.
}

void GainBucketManagerMultiResourceMixed::Print(bool condensed) const {
  printf("\nGain Bucket A Master\n");
  gain_bucket_a_master_->Print(condensed);
  for (size_t i = 0; i < gain_buckets_a_.size(); i++) {
    printf("\nGain Bucket A - Resource Affinity %lu:\n", i);
    gain_buckets_a_[i]->Print(condensed);
    printf("\n");
  }
  printf("\nGain Bucket B Master\n");
  gain_bucket_b_master_->Print(condensed);
  for (size_t i = 0; i < gain_buckets_b_.size(); i++) {
    printf("\nGain Bucket B - Resource Affinity %lu:\n", i);
    gain_buckets_b_[i]->Print(condensed);
    printf("\n");
  }
}

double GainBucketManagerMultiResourceMixed::ImbalancePowerIfMoved(
    const vector<int>& node_weight, const vector<int>& balance,
    const vector<int>& total_weight, bool from_part_a, bool use_violator) {
  //assert(node_weight.size() == balance.size());
  temp_adjusted_weight_.resize(node_weight.size());
  temp_adjusted_total_weight_.resize(node_weight.size());
  for (size_t i = 0; i < node_weight.size(); i++) {
    int change = 2 * node_weight[i];
    if (from_part_a) {
      /*
      adjusted_weight.push_back(balance[i] - change);
      adjusted_total_weight.push_back(total_weight[i] - change);
      */
      temp_adjusted_weight_[i] = balance[i] - change;
      temp_adjusted_total_weight_[i] = total_weight[i] - change;
    } else {
      /*
      adjusted_weight.push_back(balance[i] + change);
      adjusted_total_weight.push_back(total_weight[i] + change);
      */
      temp_adjusted_weight_[i] = balance[i] + change;
      temp_adjusted_total_weight_[i] = total_weight[i] + change;
    }
  }
  if (use_violator) {
    return ViolatorImbalancePower(temp_adjusted_weight_, temp_adjusted_total_weight_);
  } else {
    const vector<int>& max_weight_imbalance =
        GetMaxImbalanceRef(max_imbalance_fraction_, total_weight);
    return ImbalancePower(temp_adjusted_weight_, max_weight_imbalance);
  }
}

double GainBucketManagerMultiResourceMixed::RatioPowerIfChangedByEntry(
    const GainBucketEntry& entry, const std::vector<int>& total_weight) {
  bool in_part_a = gain_bucket_a_master_->HasNode(entry.Id());
  if (in_part_a) {
    const GainBucketEntry& gbe =
      gain_bucket_a_master_->GbeRefByNodeId(entry.Id());
    return RatioPowerIfChanged(
        gbe.current_weight_vector(), entry.current_weight_vector(),
        resource_ratio_weights_, total_weight);
  } else {
    const GainBucketEntry& gbe =
      gain_bucket_b_master_->GbeRefByNodeId(entry.Id());
    return RatioPowerIfChanged(
        gbe.current_weight_vector(), entry.current_weight_vector(),
        resource_ratio_weights_, total_weight);
  }
}

double GainBucketManagerMultiResourceMixed::ViolatorImbalancePower(
    const vector<int>& balance, const vector<int>& total_weight) {
  const vector<int>& max_weight_imbalance =
      GetMaxImbalanceRef(max_imbalance_fraction_, total_weight);
  bool violated = false;
  for (size_t i = 0; i < balance.size(); i++) {
    if (balance[i] > max_weight_imbalance[i]) {
      violated = true;
    }
  }
  return violated ? ImbalancePower(balance, max_weight_imbalance) : 0.0;
}

double GainBucketManagerMultiResourceMixed::SetBestWeightVectorByImbalancePower(GainBucketEntry* entry,
    const std::vector<int>& balance, const std::vector<int>& total_weight,
    bool from_part_a, bool use_violator) {
  int best_wv_index = -1;
  const vector<int>& current_wv = entry->current_weight_vector();
  double best_imbalance_power = numeric_limits<double>::max();
  for (size_t i = 0; i < entry->AllWeightVectors().size(); i++) {
    auto& wv = entry->AllWeightVectors()[i];
    double imbalance_power = ImbalancePowerIfMoved(
        wv, balance, total_weight, from_part_a, use_violator);
    if (use_ratio_) {
      imbalance_power += RatioPowerIfChanged(
          current_wv, wv, resource_ratio_weights_, total_weight);
    }
    if (imbalance_power < best_imbalance_power) {
      best_imbalance_power = imbalance_power;
      best_wv_index = i;
    }
  }
  assert(best_wv_index >= 0);
  entry->SetCurrentWeightVectorIndex(best_wv_index);
  return best_imbalance_power;
}

double GainBucketManagerMultiResourceMixed::ComputeGainImbalanceFn(
    double gain, double imbalance_power) {
  // TODO Experiment with this.
  double kScalingFactor = 1.0;
  return imbalance_power - kScalingFactor*gain;
}

void GainBucketManagerMultiResourceMixed::set_selection_policy(
    PartitionerConfig::GainBucketSelectionPolicy selection_policy) {
  selection_policy_ = selection_policy;
}

GainBucketEntry& GainBucketManagerMultiResourceMixed::GbeRefByNodeId(
    int node_id) {
  if (gain_bucket_a_master_->HasNode(node_id)) {
    return gain_bucket_a_master_->GbeRefByNodeId(node_id);
  } else {
    return gain_bucket_b_master_->GbeRefByNodeId(node_id);
  }
}

GainBucketEntry* GainBucketManagerMultiResourceMixed::GbePtrByNodeId(
    int node_id) {
  if (gain_bucket_a_master_->HasNode(node_id)) {
    return gain_bucket_a_master_->GbePtrByNodeId(node_id);
  } else {
    return gain_bucket_b_master_->GbePtrByNodeId(node_id);
  }
}

bool GainBucketManagerMultiResourceMixed::HasNode(
    int node_id) {
  return gain_bucket_a_master_->HasNode(node_id) ||
         gain_bucket_b_master_->HasNode(node_id);
}

void GainBucketManagerMultiResourceMixed::TouchNodes(const vector<int>& node_ids) {
  for (int node_id : node_ids) {
    for (auto& gb : gain_buckets_a_) {
      if (gb->HasNode(node_id)) {
        gb->Touch(node_id);
      }
    }
    for (auto& gb : gain_buckets_b_) {
      if (gb->HasNode(node_id)) {
        gb->Touch(node_id);
      }
    }
    if (gain_bucket_a_master_->HasNode(node_id)) {
      gain_bucket_a_master_->Touch(node_id);
    } else {
      gain_bucket_b_master_->Touch(node_id);
    }
  }
}
