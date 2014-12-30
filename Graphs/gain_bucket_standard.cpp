#include "gain_bucket_standard.h"

#include "universal_macros.h"

using namespace std;

void GainBucketStandard::Add(const GainBucketEntry& entry) {
  // Quantize floating point gain values into an integer
  int bucket_index = entry.GainIndex() + MAX_GAIN;
  assert_b(bucket_index >= 0) {
    printf("Bucket Index of %d was computed. May need to increase MAX_GAIN\n",
           bucket_index);
  }

  // Gains can be positive or negative, so add offset to index into buckets.
  if (buckets_.at(bucket_index).empty()) {
    occupied_buckets_by_index_.insert(bucket_index);
  }

  buckets_[bucket_index].push_front(entry);
  /*
  assert(node_id_to_current_gain_index_.find(entry.Id()) ==
         node_id_to_current_gain_index_.end());
         */
  BucketContents::iterator bucket_iterator = buckets_.at(bucket_index).begin();
  node_id_to_data_.insert(
      make_pair(entry.Id(), NodeTrackingData(bucket_iterator, entry.GainIndex())));
  /*
  node_id_to_current_gain_index_.insert(
      make_pair(entry.Id(), entry.GainIndex()));
  node_id_to_bucket_iterator_.insert(make_pair(entry.Id(), bucket_iterator));
  */

  // Update iterator affected by insertion into bucket.
  bucket_iterator++;
  if (bucket_iterator != buckets_.at(bucket_index).end()) {
    //node_id_to_bucket_iterator_.at(bucket_iterator->Id()) = bucket_iterator;
    node_id_to_data_.at(bucket_iterator->Id()).bucket_iterator = bucket_iterator;
  }
  num_entries_++;
}

GainBucketEntry& GainBucketStandard::Top() {
  assert(!occupied_buckets_by_index_.empty());
  auto top_bucket_iter = occupied_buckets_by_index_.begin();
  int gain_index = *top_bucket_iter;
  BucketContents& bucket = buckets_.at(gain_index);
  return bucket.front();
}

GainBucketEntry& GainBucketStandard::Peek(int offset) {
  GainBucketEntry* peek_ptr = PeekPtr(offset);
  assert(peek_ptr != nullptr);
  return *peek_ptr;
}

GainBucketEntry* GainBucketStandard::PeekPtr(int offset) {
  int cur_offset = 0;
  if (offset > num_entries_ - 1) {
    return nullptr;
  }
  for (int valid_index : occupied_buckets_by_index_) {
    BucketContents& bucket = buckets_.at(valid_index);
    for (auto& gbe : bucket) {
      if (cur_offset == offset) {
        return &gbe;
      }
      ++cur_offset;
    }
  }
  return nullptr;
}

void GainBucketStandard::Pop() {
  auto top_bucket_iter = occupied_buckets_by_index_.begin();
  int bucket_index = *top_bucket_iter;
  BucketContents& bucket = buckets_.at(bucket_index);
  assert(!bucket.empty());
  RemoveByNodeId(bucket.front().Id());
}


bool GainBucketStandard::Empty() const {
  return (num_entries_ == 0);
}

void GainBucketStandard::UpdateGains(
    double cost_gain_modifier,
    const EdgeKlfm::NodeIdVector& nodes_to_update) {
  for (auto node_id : nodes_to_update) {
    GainBucketEntry entry = RemoveByNodeId(node_id);
    entry.SetCostGain(entry.CostGain() + cost_gain_modifier);
    Add(entry);
  }
}

bool GainBucketStandard::HasNode(int node_id) {
  return node_id_to_data_.find(node_id) != node_id_to_data_.end();
}

void GainBucketStandard::Touch(int node_id) {
  NodeTrackingData& new_front_ntd = node_id_to_data_.at(node_id);
  int gain_index = new_front_ntd.current_gain_index;
  int bucket_index = gain_index + MAX_GAIN;
  BucketContents& bucket = buckets_.at(bucket_index);
  if (new_front_ntd.bucket_iterator == bucket.begin()) {
    return;
  }
  bucket.push_front(std::move(*new_front_ntd.bucket_iterator));
  bucket.erase(new_front_ntd.bucket_iterator);
  new_front_ntd.bucket_iterator = bucket.begin();
}

GainBucketEntry GainBucketStandard::RemoveByNodeId(int node_id) {
  NodeTrackingData& ntd = node_id_to_data_.at(node_id);
  int gain_index = ntd.current_gain_index;
  int bucket_index = gain_index + MAX_GAIN;
  BucketContents& bucket = buckets_.at(bucket_index);
  assert(!bucket.empty());

  BucketContents::iterator erase_iter = ntd.bucket_iterator;
  GainBucketEntry entry = std::move(*erase_iter);
  assert(entry.Id() == node_id);

  // Erase returns the new iterator for the element that follows the erased one.
  BucketContents::iterator next_iter = bucket.erase(erase_iter);
  if (bucket.empty()) {
    occupied_buckets_by_index_.erase(bucket_index);
  }
  node_id_to_data_.erase(node_id);
  //assert(node_id_to_bucket_iterator_.count(node_id) == 0);
  num_entries_--;

  // Update the iterators before and after the erased element.
  if (next_iter != bucket.end()) {
    assert(next_iter->Id() != node_id);
    node_id_to_data_.at(next_iter->Id()).bucket_iterator = next_iter;
  }
  if (next_iter != bucket.begin()) {
    next_iter--;
    assert(next_iter->Id() != node_id);
    node_id_to_data_.at(next_iter->Id()).bucket_iterator = next_iter;
  }

  return entry;
}

GainBucketEntry& GainBucketStandard::GbeRefByNodeId(int node_id) {
  NodeTrackingData& ntd = node_id_to_data_.at(node_id);
  /*
  int gain_index = ntd.current_gain_index;
  int bucket_index = gain_index + MAX_GAIN;
  BucketContents& bucket = buckets_.at(bucket_index);
  assert(!bucket.empty());

  BucketContents::iterator iter = node_id_to_bucket_iterator_.at(node_id);
  */
  return *(ntd.bucket_iterator);
}

GainBucketEntry* GainBucketStandard::GbePtrByNodeId(int node_id) {
  if (node_id > (int)node_id_to_data_.size()) {
    return nullptr;
  }
  NodeTrackingData& ntd = node_id_to_data_.at(node_id);
  return &(*(ntd.bucket_iterator));
}

void GainBucketStandard::Print(bool condensed) const {
  printf("Occupied buckets (by value): ");
  for (auto bucket_index : occupied_buckets_by_index_) {
    printf("%d ", bucket_index - MAX_GAIN);
  }
  printf("\n");
  if (!condensed) {
    for (auto bucket_index : occupied_buckets_by_index_) {
      const BucketContents& bucket = buckets_.at(bucket_index);
      printf("Nodes in bucket %d (id: weight):\n", bucket_index - MAX_GAIN);
      for (auto entry : bucket) {
        printf("(%d: ", entry.Id());
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
