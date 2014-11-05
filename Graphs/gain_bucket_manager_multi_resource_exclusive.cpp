#include "gain_bucket_manager_multi_resource_exclusive.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

#include "universal_macros.h"

using namespace std;

GainBucketEntry
    GainBucketManagerMultiResourceExclusive::GetNextGainBucketEntry(
        const vector<int>& current_balance,
        const vector<int>& total_weight) {

  // How do we select which bucket to examine?
  //
  // 1. Random
  // 2. Resource with greatest imbalance pct
  // 3. Highest gain from all unconstrained buckets
  // 4. Highest gain from all buckets (limit search in constrained buckets)
  GainBucketEntry entry;
  switch (selection_policy_) {
    case PartitionerConfig::kGbmreSelectionPolicyRandomResource:
      entry = GetNextGainBucketEntryRandomResource(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using RANDOM RESOURCE policy." <<
          endl;
    break;
    case PartitionerConfig::kGbmreSelectionPolicyLargestResourceImbalance:
      entry = GetNextGainBucketEntryLargestImbalanceResource(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using LARGEST IMBALANCE policy." <<
          endl;
    break;
    case PartitionerConfig::kGbmreSelectionPolicyLargestUnconstrainedGain:
      entry = GetNextGainBucketEntryLargestUnconstrainedGain(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using UNCONSTRAINED GAIN policy." <<
          endl;
    break;
    case PartitionerConfig::kGbmreSelectionPolicyLargestGain:
      entry = GetNextGainBucketEntryLargestGain(
          current_balance, total_weight);
      DLOG(DEBUG_OPT_TRACE, 3) <<
          "Get next gain bucket entry using LARGEST GAIN policy." <<
          endl;
    break;
    default:
      assert_b(false) {
        printf("Unrecognized selection policy for multi-resource-exclusive "
               "gain bucket.\n");
      }
  }

  RUN_DEBUG(DEBUG_OPT_BUCKET_NODE_SELECT, 0) {
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

void GainBucketManagerMultiResourceExclusive::RemoveNode(int node_id) {
  int erased = node_id_to_resource_index_.erase(node_id);
  if (erased > 0) {
    for (auto& bucket : gain_buckets_a_) {
      if (bucket->HasNode(node_id)) {
        // TODO Get rid of debug check when stable.
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
    num_nodes_--;
  }
}

GainBucketEntry GainBucketManagerMultiResourceExclusive::GetNextGainBucketEntryRandomResource(
    const vector<int>& current_balance, const vector<int>& total_weight) {
  vector<GainBucketInterface*> constrained_buckets;
  vector<GainBucketInterface*> unconstrained_buckets;
  vector<int> max_constrained_node_weights;
  vector<int> max_weight_imbalance =
      GetMaxImbalance(max_imbalance_fraction_, total_weight);
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    // For this function, only consider resources that are not exhausted.
    // Note that individual buckets may still be empty, but not both for a
    // given resource.
    if (!(gain_buckets_a_[i]->Empty() && gain_buckets_b_[i]->Empty())) {
      // Want to round down here, so the truncated div 2 is appropriate.
      int mcnw = (max_weight_imbalance[i] - abs(current_balance[i])) / 2;
      if (mcnw < 0) {
        mcnw = 0;
      }
      max_constrained_node_weights.push_back(mcnw);
      if (current_balance[i] < 0) {
        DLOG(DEBUG_OPT_BUCKET_NODE_SELECT_RANDOM_RESOURCE, 0) <<
            "Constrain A because resource " << i << " = " <<
            current_balance[i] << endl;
        constrained_buckets.push_back(gain_buckets_a_[i]);
        unconstrained_buckets.push_back(gain_buckets_b_[i]);
      } else {
        DLOG(DEBUG_OPT_BUCKET_NODE_SELECT_RANDOM_RESOURCE, 0) <<
            "Constrain B because resource " << i << " = " <<
            current_balance[i] << endl;
        constrained_buckets.push_back(gain_buckets_b_[i]);
        unconstrained_buckets.push_back(gain_buckets_a_[i]);
      }
    }
  }
  assert(!(unconstrained_buckets.empty() && constrained_buckets.empty()));
  int bucket_index = random_engine_() % unconstrained_buckets.size();

  GainBucketInterface* constrained_bucket = constrained_buckets[bucket_index];
  GainBucketInterface* unconstrained_bucket =
      unconstrained_buckets[bucket_index];
  int resource_index = -1;
  if (!unconstrained_bucket->Empty()) {
    for (size_t i = 0; i < num_resources_per_node_; i++) {
      if (unconstrained_bucket->Top().current_weight_vector()[i] != 0) {
        resource_index = i;
      }
    }
  } else {
    for (size_t i = 0; i < num_resources_per_node_; i++) {
      if (constrained_bucket->Top().current_weight_vector()[i] != 0) {
        resource_index = i;
      }
    }
  }
  assert(resource_index >= 0);
  RUN_DEBUG(DEBUG_OPT_BUCKET_NODE_SELECT_RANDOM_RESOURCE, 0) {
    cout << "Choosing based on resource " << resource_index << ". "
         << "Current balance: " << current_balance[0] << " "
         << current_balance[1] << " " << current_balance[2] << "." << endl;
    if (gain_buckets_a_[resource_index] == constrained_bucket &&
        gain_buckets_b_[resource_index] == unconstrained_bucket) {
      cout << "Bucket A is constrained." << endl;
      cout << "Bucket B is unconstrained." << endl;
    } else if (gain_buckets_b_[resource_index] == constrained_bucket &&
        gain_buckets_a_[resource_index] == unconstrained_bucket) {
      cout << "Bucket B is constrained." << endl;
      cout << "Bucket A is unconstrained." << endl;
    } else {
      cout << "Can't determine which bucket is constrained." << endl;
    }
  }

  // TODO : Confirm that entry fits! Retry if it doesn't.
  GainBucketEntry entry = SelectBetweenBuckets(
      constrained_bucket, unconstrained_bucket, resource_index,
      max_constrained_node_weights[bucket_index]);
  return entry;
}

GainBucketEntry GainBucketManagerMultiResourceExclusive::GetNextGainBucketEntryLargestImbalanceResource(
    const vector<int>& current_balance, const vector<int>& total_weight) {
  vector<GainBucketInterface*> constrained_buckets;
  vector<GainBucketInterface*> unconstrained_buckets;
  vector<int> max_constrained_node_weights;
  vector<int> max_weight_imbalance =
      GetMaxImbalance(max_imbalance_fraction_, total_weight);
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    int mcnw = (max_weight_imbalance[i] - abs(current_balance[i])) / 2;
    if (mcnw < 0) {
      mcnw = 0;
    }
    max_constrained_node_weights.push_back(mcnw);
    if (current_balance[i] < 0) {
      constrained_buckets.push_back(gain_buckets_a_[i]);
      unconstrained_buckets.push_back(gain_buckets_b_[i]);
    } else {
      constrained_buckets.push_back(gain_buckets_b_[i]);
      unconstrained_buckets.push_back(gain_buckets_a_[i]);
    }
  }
  assert(!(constrained_buckets.empty() && unconstrained_buckets.empty()));

  double largest_frac = 0.0;
  int resource_index = 0;
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    if (!(constrained_buckets[i]->Empty() &&
          unconstrained_buckets[i]->Empty())) {
      double weight_frac =
          abs((double)current_balance[i] / (double)max_weight_imbalance[i]);
      if (weight_frac >= largest_frac) {
        resource_index = i;
        largest_frac = weight_frac;
      }
    }
  }

  GainBucketInterface* constrained_bucket = constrained_buckets[resource_index];
  GainBucketInterface* unconstrained_bucket =
      unconstrained_buckets[resource_index];

  GainBucketEntry entry = SelectBetweenBuckets(
      constrained_bucket, unconstrained_bucket, resource_index,
      max_constrained_node_weights[resource_index]);
  return entry;
}

GainBucketEntry GainBucketManagerMultiResourceExclusive::GetNextGainBucketEntryLargestUnconstrainedGain(
    const vector<int>& current_balance, const vector<int>& total_weight) {
  vector<GainBucketInterface*> unconstrained_buckets;
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    if (current_balance[i] < 0) {
      if (!gain_buckets_b_[i]->Empty()) {
        unconstrained_buckets.push_back(gain_buckets_b_[i]);
      }
    } else {
      if (!gain_buckets_a_[i]->Empty()) {
        unconstrained_buckets.push_back(gain_buckets_a_[i]);
      }
    }
  }
  if (unconstrained_buckets.empty()) {
    // The case where the only unlocked nodes are in constrained buckets
    // should only occur near the end of partitioning, so the policy used
    // is probably not important.
    return GetNextGainBucketEntryRandomResource(current_balance, total_weight);
  }

  double max_unconstrained_gain = unconstrained_buckets[0]->Top().CostGain();
  int index = 0;
  for (size_t i = 1; i < unconstrained_buckets.size(); i++) {
    double my_gain = unconstrained_buckets[i]->Top().CostGain();
    if (my_gain > max_unconstrained_gain) {
      max_unconstrained_gain = my_gain;
      index = i;
    }
  }
  GainBucketEntry entry = unconstrained_buckets[index]->Top();
  unconstrained_buckets[index]->Pop();
  RemoveNode(entry.Id());
  return entry;
}

GainBucketEntry
    GainBucketManagerMultiResourceExclusive::GetNextGainBucketEntryLargestGain(
        const vector<int>& current_balance,
        const vector<int>& total_weight) {

  // For each bucket, search until we find an entry that fits or the max
  // search depth is reached. Sort the found entries by gain and return the
  // highest one. With exclusive resources, it is guaranteed that at least
  // half the buckets will return an entry.
  vector<int> max_weight_imbalance =
      GetMaxImbalance(max_imbalance_fraction_, total_weight);
  vector<int> max_constrained_node_weights;
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    int mcnw = (max_weight_imbalance[i] - abs(current_balance[i])) / 2;
    if (mcnw < 0) {
      mcnw = 0;
    }
    max_constrained_node_weights.push_back(mcnw);
  }

  vector<pair<bool, pair<int, GainBucketInterface*>>> buckets;
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    bool part_a_constrained = current_balance[i] < 0;
    if (!gain_buckets_a_[i]->Empty()) {
      pair<int, GainBucketInterface*> res_bucket_pair =
          make_pair(i, gain_buckets_a_[i]);
      buckets.push_back(make_pair(part_a_constrained, res_bucket_pair));
    }
    if (!gain_buckets_b_[i]->Empty()) {
      pair<int, GainBucketInterface*> res_bucket_pair =
          make_pair(i, gain_buckets_b_[i]);
      buckets.push_back(make_pair(!part_a_constrained, res_bucket_pair));
    }
  }

  vector<pair<int, GainBucketEntry>> top_entries;
  for (size_t bucket_num = 0; bucket_num < buckets.size(); bucket_num++) {
    vector<GainBucketEntry> passed_entries;
    GainBucketEntry entry;
    bool found_good_entry = false;
    GainBucketInterface* bucket = buckets[bucket_num].second.second;
    int res = buckets[bucket_num].second.first;
    bool bucket_is_constrained = buckets[bucket_num].first;
    // This is likely to be a critical loop in the algorithm, so the value
    // of MAX_CONSTRAINED_ENTRY_CHECKS may have a large impact on performance.
    for (size_t entry_num = 0; entry_num < MAX_CONSTRAINED_ENTRY_CHECKS &&
         !bucket->Empty(); entry_num++) {
      entry = bucket->Top();
      bucket->Pop();
      bool entry_fits = true;
      if (bucket_is_constrained &&
          abs(entry.current_weight_vector()[res]) >
              max_constrained_node_weights[res]) {
        entry_fits = false;
      }
      if (entry_fits) {
        found_good_entry = true;
        break;
      } else {
        passed_entries.push_back(entry);
      }
    }
    if (found_good_entry) {
      top_entries.push_back(make_pair(bucket_num, entry));
    }
    // Put all of the entries that didn't fit back in their buckets.
    for (auto& passed_entry : passed_entries) {
      bucket->Add(passed_entry);
    }
  }
  if (top_entries.size() == 0) {
    // This case may occur when adaptive node implementations are allowed.
    // The selection of implementations on previous node moves may create a
    // situation where it is impossible to make a move that does not exceed
    // the weight imbalance. This usually will only occur when the gain buckets
    // are almost empty. It doesn't matter which entry we return in this case
    // so just return the first entry in the first bucket.
    assert(use_adaptive_);
    assert(!buckets.empty());
    GainBucketEntry entry = buckets[0].second.second->Top();
    RemoveNode(entry.Id());
    return entry;
  }

  // Shuffle top_entries. This prevents favoring certain buckets by their
  // resource order.
  shuffle(top_entries.begin(), top_entries.end(), random_engine_);

  // Find max-gain entry. Put any non-max entries back in their buckets.
  double max_gain = top_entries[0].second.CostGain();
  int max_index = 0;
  for (size_t i = 1; i < top_entries.size(); i++) {
    if (top_entries[i].second.CostGain() > max_gain) {
      // Put the previous max back in its bucket.
      buckets[top_entries[max_index].first].second.second->Add(
          top_entries[max_index].second);
      max_gain = top_entries[i].second.CostGain();
      max_index = i;
    } else {
      // Not the max, so put it back in its bucket.
      buckets[top_entries[i].first].second.second->Add(top_entries[i].second);
    }
  }

  // At this point, all non-max entries have been copied back to their buckets,
  // so it is safe to return.
  GainBucketEntry& entry = top_entries[max_index].second;
  RemoveNode(entry.Id());
  return entry;
}

GainBucketEntry GainBucketManagerMultiResourceExclusive::SelectBetweenBuckets(
    GainBucketInterface* constrained_bucket,
    GainBucketInterface* unconstrained_bucket,
    int resource_index,
    int max_constrained_node_weight) {

  GainBucketEntry constrained_entry;
  GainBucketEntry unconstrained_entry;

  // Handle the case where one of the buckets is empty.
  if (constrained_bucket->Empty()) {
    assert(!unconstrained_bucket->Empty()); // DEBUG
    unconstrained_entry = unconstrained_bucket->Top();
    unconstrained_bucket->Pop();
    RemoveNode(unconstrained_entry.Id());
    return unconstrained_entry;
  } else if (unconstrained_bucket->Empty()) {
    assert(!constrained_bucket->Empty()); // DEBUG
    constrained_entry = constrained_bucket->Top();
    constrained_bucket->Pop();
    RemoveNode(constrained_entry.Id());
    return constrained_entry;
  }

  constrained_entry = constrained_bucket->Top();
  unconstrained_entry = unconstrained_bucket->Top();

  /* Actually finding the highest gain node that will fit has unacceptable
     worst-case complexity - O(n). Instead, we will only consider the highest
     gain entry from the unconstrained bucket and a statically capped number
     of entries from the constrained bucket, determined by the macro value
     MAX_CONSTRAINED_ENTRY_CHECKS. */
  int constrained_entries_checked = 1;
  int max_checks =
      (constrained_bucket->num_entries() > MAX_CONSTRAINED_ENTRY_CHECKS) ?
      MAX_CONSTRAINED_ENTRY_CHECKS : constrained_bucket->num_entries() - 1;
  vector<GainBucketEntry> constrained_entries_passed;
  while ((constrained_entry.CostGain() > unconstrained_entry.CostGain()) &&
         (constrained_entry.current_weight_vector()[resource_index] >
          max_constrained_node_weight) &&
         (constrained_entries_checked <= max_checks)) {
    constrained_entries_passed.push_back(constrained_entry);
    constrained_bucket->Pop();
    constrained_entry = constrained_bucket->Top();
    constrained_entries_checked++;
  }

  bool use_constrained =
    (constrained_entry.CostGain() > unconstrained_entry.CostGain()) &&
    (constrained_entry.current_weight_vector()[resource_index] <=
     max_constrained_node_weight);

  if (use_constrained) {
    constrained_bucket->Pop();
  } else {
    unconstrained_bucket->Pop();
  }

  for (auto& it : constrained_entries_passed) {
    constrained_bucket->Add(it);
  }

  if (use_constrained) {
    RemoveNode(constrained_entry.Id());
    DLOG(DEBUG_OPT_BUCKET_NODE_SELECT, 0) <<
        "Used constrained entry with weight " <<
        constrained_entry.current_weight_vector()[0] << " " <<
        constrained_entry.current_weight_vector()[1] << " " <<
        constrained_entry.current_weight_vector()[2] << 
        " when max weight is " << max_constrained_node_weight << endl;
    return constrained_entry;
  } else {
    RemoveNode(unconstrained_entry.Id());
    DLOG(DEBUG_OPT_BUCKET_NODE_SELECT, 0) <<
        "Used unconstrained entry with weight " <<
        unconstrained_entry.current_weight_vector()[0] << " " <<
        unconstrained_entry.current_weight_vector()[1] << " " <<
        unconstrained_entry.current_weight_vector()[2] <<
        " when max weight is " << max_constrained_node_weight << endl;
    return unconstrained_entry;
  }
}

int GainBucketManagerMultiResourceExclusive::NumUnlockedNodes() const {
  return num_nodes_;
}

bool GainBucketManagerMultiResourceExclusive::Empty() const {
  return num_nodes_ == 0;
}

void GainBucketManagerMultiResourceExclusive::AddNode(double gain, Node* node,
    bool in_part_a, const std::vector<int>& total_weight) {
  GainBucketEntry entry(gain, node);
  if (use_adaptive_) {
    // Can only add one entry per resource type.
    vector<pair<int,int>> res_min_index;
    res_min_index.assign(num_resources_per_node_, make_pair(-1,0));
    for (size_t i = 0; i < node->WeightVectors().size(); i++) {
      const vector<int>& wv = node->WeightVector(i);
      for (size_t res = 0; res < wv.size(); res++) {
        if (wv[res] != 0) {
          if (res_min_index[res].first == -1 ||
              res_min_index[res].second < wv[res]) {
            res_min_index[res].first = i;
            res_min_index[res].second = wv[res];
          }
          break;
        }
      }
    }
    for (auto it : res_min_index) {
      if (it.first >= 0) {
        entry.SetCurrentWeightVectorIndex(it.first);
        AddEntry(entry, in_part_a);
      }
    }
  } else {
    AddEntry(entry, in_part_a);
  }
  num_nodes_++;
}

void GainBucketManagerMultiResourceExclusive::AddEntry(
    const GainBucketEntry& entry, bool in_part_a) {
  assert(entry.current_weight_vector().size() == num_resources_per_node_);
  int pos = -1;
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    if (entry.current_weight_vector()[i] != 0) {
      assert_b(pos == -1) {
        printf("Using a Resource-Exclusive gain bucket, but detected a weight "
               "vector with weights in more than one resource in node %d\n",
               entry.Id());
      }
      pos = i;
    }
  }
  assert_b(pos >= 0) {
    printf("Encountered an empty weight vector in node %d\n", entry.Id());
  }
  if (in_part_a) {
    gain_buckets_a_[pos]->Add(entry);
  } else {
    gain_buckets_b_[pos]->Add(entry);
  }
  node_id_to_resource_index_.insert(make_pair(entry.Id(), pos));
}

void GainBucketManagerMultiResourceExclusive::UpdateGains(
    double gain_modifier, const vector<int>& nodes_to_increase_gain,
    const vector<int>& nodes_to_decrease_gain, bool moved_from_part_a) {
  vector<vector<int>> inc;  
  inc.resize(num_resources_per_node_);
  vector<vector<int>> dec;  
  dec.resize(num_resources_per_node_);
  for (auto id : nodes_to_increase_gain) {
    assert(node_id_to_resource_index_.count(id) != 0);
    auto it_pair = node_id_to_resource_index_.equal_range(id);
    for (auto it = it_pair.first; it != it_pair.second; it++) {
      inc[it->second].push_back(id);
    }
  }
  for (auto id : nodes_to_decrease_gain) {
    assert(node_id_to_resource_index_.count(id) != 0);
    auto it_pair = node_id_to_resource_index_.equal_range(id);
    for (auto it = it_pair.first; it != it_pair.second; it++) {
      dec[it->second].push_back(id);
    }
  }
  for (size_t i = 0; i < num_resources_per_node_; i++) {
    if (!inc[i].empty()) {
      if (moved_from_part_a) {
        gain_buckets_a_[i]->UpdateGains(gain_modifier, inc[i]);
      } else {
        gain_buckets_b_[i]->UpdateGains(gain_modifier, inc[i]);
      }
    }
    if (!dec[i].empty()) {
      if (moved_from_part_a) {
        gain_buckets_b_[i]->UpdateGains(-gain_modifier, dec[i]);
      } else {
        gain_buckets_a_[i]->UpdateGains(-gain_modifier, dec[i]);
      }
    }
  }
}

void GainBucketManagerMultiResourceExclusive::UpdateNodeImplementation(
    Node* node) {
  if (use_adaptive_) {
    // The selected implementation should be in one of the buckets already,
    // except in strange cases. 
    return;
  }
  auto res_it = node_id_to_resource_index_.find(node->id);
  if (res_it == node_id_to_resource_index_.end()) {
    return;
  }
  int res_index = res_it->second;
  if (gain_buckets_a_[res_index]->HasNode(node->id)) {
    double gain =
        gain_buckets_a_[res_index]->RemoveByNodeId(node->id).CostGain();
    gain_buckets_a_[res_index]->Add(GainBucketEntry(gain, node));
  } else {
    double gain =
        gain_buckets_b_[res_index]->RemoveByNodeId(node->id).CostGain();
    gain_buckets_b_[res_index]->Add(GainBucketEntry(gain, node));
  }
}

bool GainBucketManagerMultiResourceExclusive::InPartA(int node_id) {
  auto res_it = node_id_to_resource_index_.find(node_id);
  if (res_it == node_id_to_resource_index_.end()) {
    return false;
  }
  if (gain_buckets_a_[res_it->second]->HasNode(node_id)) {
    return true;
  } else {
    return false;
  }
}

void GainBucketManagerMultiResourceExclusive::Print(bool condensed) const {
  for (size_t i = 0; i < gain_buckets_a_.size(); i++) {
    printf("\nGain Bucket A - Resource %lu:\n", i);
    gain_buckets_a_[i]->Print(condensed);
    printf("\n");
  }
  for (size_t i = 0; i < gain_buckets_b_.size(); i++) {
    printf("\nGain Bucket B - Resource %lu:\n", i);
    gain_buckets_b_[i]->Print(condensed);
    printf("\n");
  }
}

void GainBucketManagerMultiResourceExclusive::set_selection_policy(
    PartitionerConfig::GainBucketSelectionPolicy selection_policy) {
  selection_policy_ = selection_policy;
}
