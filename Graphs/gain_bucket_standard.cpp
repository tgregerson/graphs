#include "gain_bucket_standard.h"

#include "universal_macros.h"

using namespace std;

void GainBucketStandard::Add(GainBucketEntry entry) {
  int gain_index = entry.gain + MAX_GAIN;
  assert_b(gain_index >= 0) {
    printf("Gain Index of %d was computed. May need to increase MAX_GAIN\n",
           gain_index);
  }

  BucketContents::iterator orig_front = buckets_[gain_index].begin();

  // Gains can be positive or negative, so add offset to index into buckets.
  if (buckets_[gain_index].empty()) {
    occupied_buckets_by_index_.insert(gain_index);
  }

  buckets_[gain_index].push_front(entry);
  node_id_to_current_gain_[entry.id] = entry.gain;
  BucketContents::iterator bucket_iterator = buckets_[gain_index].begin();
  node_id_to_bucket_iterator_.insert(make_pair(entry.id, bucket_iterator));

  // Update iterator affected by insertion into bucket.
  bucket_iterator++;
  if (bucket_iterator != buckets_[gain_index].end()) {
    node_id_to_bucket_iterator_[bucket_iterator->id] = bucket_iterator;
  }
  num_entries_++;
}

GainBucketEntry GainBucketStandard::Top() const {
  assert(!occupied_buckets_by_index_.empty());
  auto top_bucket_iter = occupied_buckets_by_index_.begin();
  int gain_index = *top_bucket_iter;
  const BucketContents& bucket = buckets_[gain_index];
  return bucket.front();
}

void GainBucketStandard::Pop() {
  auto top_bucket_iter = occupied_buckets_by_index_.begin();
  int gain_index = *top_bucket_iter;
  BucketContents& bucket = buckets_[gain_index];
  assert(!bucket.empty());
  RemoveByNodeId(bucket.front().id);
}

bool GainBucketStandard::Empty() const {
  return (num_entries_ == 0);
}

void GainBucketStandard::UpdateGains(
    int gain_modifier, const EdgeKlfm::NodeIdVector& nodes_to_update) {
  for (auto node_id : nodes_to_update) {
    GainBucketEntry entry = RemoveByNodeId(node_id);
    entry.gain += gain_modifier;
    Add(entry);
  }
}

bool GainBucketStandard::HasNode(int node_id) {
  return node_id_to_current_gain_.count(node_id) != 0;
}

GainBucketEntry GainBucketStandard::RemoveByNodeId(int node_id) {

  assert(HasNode(node_id));
  int gain = node_id_to_current_gain_[node_id];
  int gain_index = gain + MAX_GAIN;
  assert(occupied_buckets_by_index_.count(gain_index) != 0);
  BucketContents& bucket = buckets_[gain_index];
  assert(!bucket.empty());

  assert(node_id_to_bucket_iterator_.count(node_id) != 0);
  BucketContents::iterator erase_iter = node_id_to_bucket_iterator_[node_id];
  GainBucketEntry entry = *erase_iter;
  assert(entry.id == node_id);

  // Erase returns the new iterator for the element that follows the erased one.
  BucketContents::iterator next_iter = bucket.erase(erase_iter);
  if (bucket.empty()) {
    occupied_buckets_by_index_.erase(gain_index);
  }
  node_id_to_current_gain_.erase(node_id);
  node_id_to_bucket_iterator_.erase(node_id);
  assert(node_id_to_bucket_iterator_.count(node_id) == 0);
  num_entries_--;

  // Update the iterators before and after the erased element.
  if (next_iter != bucket.end()) {
    assert(next_iter->id != node_id);
    node_id_to_bucket_iterator_[next_iter->id] = next_iter;
  }
  if (next_iter != bucket.begin()) {
    next_iter--;
    assert(next_iter->id != node_id);
    node_id_to_bucket_iterator_[next_iter->id] = next_iter;
  }

  return entry;
}

GainBucketEntry& GainBucketStandard::GbeRefByNodeId(int node_id) {

  assert(HasNode(node_id));
  int gain = node_id_to_current_gain_[node_id];
  int gain_index = gain + MAX_GAIN;
  assert(occupied_buckets_by_index_.count(gain_index) != 0);
  BucketContents& bucket = buckets_[gain_index];
  assert(!bucket.empty());

  assert(node_id_to_bucket_iterator_.count(node_id) != 0);
  BucketContents::iterator iter = node_id_to_bucket_iterator_[node_id];
  return *iter;
}

void GainBucketStandard::Print(bool condensed) const {
  printf("Occupied buckets (by value): ");
  for (auto gain_index : occupied_buckets_by_index_) {
    printf("%d ", gain_index - MAX_GAIN);
  }
  printf("\n");
  if (!condensed) {
    for (auto gain_index : occupied_buckets_by_index_) {
      const BucketContents& bucket = buckets_[gain_index];
      printf("Nodes in bucket %d (id: weight):\n", gain_index - MAX_GAIN);
      for (auto entry : bucket) {
        printf("(%d: ", entry.id);
        for (auto wt_it : entry.current_weight_vector()) {
          printf(" %d", wt_it);
        }
        printf(")\n");
      }
      printf("\n");
    }
  }
  printf("\n");
}
