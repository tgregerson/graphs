#include "gain_bucket_manager_single_resource.h"

#include <cassert>

using namespace std;

GainBucketEntry
    GainBucketManagerSingleResource::GetNextGainBucketEntry(
        const vector<int>& current_balance,
        const vector<int>& total_weight) {

  int weight_balance = current_balance[resource_index_];
  GainBucketInterface* constrained_bucket = (weight_balance > 0) ?
      gain_bucket_b_ : gain_bucket_a_;
  GainBucketInterface* unconstrained_bucket = (weight_balance > 0) ?
      gain_bucket_a_ : gain_bucket_b_;

  // Divide by two here because the weight will be both subtracted from
  // one partition and added to the other, so the shift will be double the
  // node weight.
  int max_constrained_node_weight =
      (int(max_imbalance_fraction_ * total_weight[resource_index_]) -
       abs(weight_balance)) / 2;

  GainBucketEntry constrained_entry;
  GainBucketEntry unconstrained_entry;

  // Handle the case where one of the buckets is empty.
  if (constrained_bucket->Empty()) {
    assert(!unconstrained_bucket->Empty()); // DEBUG
    unconstrained_entry = unconstrained_bucket->Top();
    unconstrained_bucket->Pop();
    return unconstrained_entry;
  } else if (unconstrained_bucket->Empty()) {
    assert(!constrained_bucket->Empty()); // DEBUG
    constrained_entry = constrained_bucket->Top();
    assert(constrained_entry.current_weight_vector()[resource_index_] <=
           max_constrained_node_weight);
    constrained_bucket->Pop();
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
  while ((constrained_entry.gain > unconstrained_entry.gain) &&
         (constrained_entry.current_weight_vector()[resource_index_] >
          max_constrained_node_weight) &&
         (constrained_entries_checked <= max_checks)) {
    constrained_entries_passed.push_back(constrained_entry);
    constrained_bucket->Pop();
    constrained_entry = constrained_bucket->Top();
    constrained_entries_checked++;
  }

  bool use_constrained =
    (constrained_entry.gain > unconstrained_entry.gain) &&
    (constrained_entry.current_weight_vector()[resource_index_] <=
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
    return constrained_entry;
  } else {
    return unconstrained_entry;
  }
}

int GainBucketManagerSingleResource::NumUnlockedNodes() const {
  return gain_bucket_a_->num_entries() + gain_bucket_b_->num_entries();
}

bool GainBucketManagerSingleResource::Empty() const {
  return NumUnlockedNodes() == 0;
}

void GainBucketManagerSingleResource::AddNode(
    int gain, Node* node, bool in_part_a,
    const std::vector<int>& /* total_weight */) {
  GainBucketEntry entry(gain, node);
  AddEntry(entry, in_part_a);
}

void GainBucketManagerSingleResource::AddEntry(
    const GainBucketEntry& entry, bool in_part_a) {
  if (in_part_a) {
    gain_bucket_a_->Add(entry);
  } else {
    gain_bucket_b_->Add(entry);
  }
}

void GainBucketManagerSingleResource::UpdateNodeImplementation(Node* node) {
  if (gain_bucket_a_->HasNode(node->id)) {
    GainBucketEntry& gbe = gain_bucket_a_->GbeRefByNodeId(node->id);
    gbe.current_weight_vector_index = node->selected_weight_vector_index();
  } else if (gain_bucket_b_->HasNode(node->id)) {
    GainBucketEntry& gbe = gain_bucket_b_->GbeRefByNodeId(node->id);
    gbe.current_weight_vector_index = node->selected_weight_vector_index();
  }
}

void GainBucketManagerSingleResource::UpdateGains(
    int gain_modifier, const std::vector<int>& nodes_to_increase_gain, 
    const std::vector<int>& nodes_to_decrease_gain, bool moved_from_part_a) {
  if (moved_from_part_a) {
    gain_bucket_a_->UpdateGains(gain_modifier, nodes_to_increase_gain);
    gain_bucket_b_->UpdateGains(-gain_modifier, nodes_to_decrease_gain);
  } else {
    gain_bucket_b_->UpdateGains(gain_modifier, nodes_to_increase_gain);
    gain_bucket_a_->UpdateGains(-gain_modifier, nodes_to_decrease_gain);
  }
}

void GainBucketManagerSingleResource::Print(bool condensed) const {
  printf("\nGain Bucket A:\n");
  gain_bucket_a_->Print(condensed);
  printf("\nGain Bucket B:\n");
  gain_bucket_b_->Print(condensed);
}
