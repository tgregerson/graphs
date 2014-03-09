#ifndef GAIN_BUCKET_STANDARD_H_
#define GAIN_BUCKET_STANDARD_H_

#include "gain_bucket_interface.h"

#include <cassert>
#include <list>
#include <set>
#include <unordered_map>
#include <vector>

#include "gain_bucket_entry.h"

class GainBucketStandard : public GainBucketInterface {
 public:
  // Be VERY careful about changing this from std::list, since we store
  // iterators and need to reason about what operations will invalidate them.
  typedef std::list<GainBucketEntry> BucketContents;

  GainBucketStandard() : num_entries_(0) {
    buckets_.resize(2 * MAX_GAIN + 1);
  }

  virtual ~GainBucketStandard() {}

  virtual void Add(GainBucketEntry entry);

  // Returns the entry of the highest gain element.
  virtual GainBucketEntry Top() const;

  // Removes the entry corresponding to Top() from the gain bucket.
  virtual void Pop();

  // Returns false if there are any entries in the bucket.
  virtual bool Empty() const;

  // Add the gain modifier to each of the nodes in the vector.
  virtual void UpdateGains(int gain_modifier,
                           const EdgeKlfm::NodeIdVector& nodes_to_update);

  // Returns true if the node with 'node_id' is in the bucket.
  virtual bool HasNode(int node_id);

  // Removes the node with 'node_id' and returns its entry.
  virtual GainBucketEntry RemoveByNodeId(int node_id);

  // Returns a reference to the gain bucket entry with 'node_id'.
  virtual GainBucketEntry& GbeRefByNodeId(int node_id);

  // Print debug information.
  virtual void Print(bool condensed) const;

  // Return the number of entries in the bucket.
  virtual int num_entries() const { return num_entries_; }

 private:
  // Use greater to make the set sort in descending value.
  std::set<int, std::greater<int>> occupied_buckets_by_index_;
  std::vector<BucketContents> buckets_;
  // Note: gain, NOT gain offset!
  std::unordered_map<int, int> node_id_to_current_gain_;
  // This data structure is used to accelerate finding items in a bucket.
  // Care must be taken to update it any time any operation may change or
  // invalidate an iterator to BucketContents.
  std::unordered_map<int, BucketContents::iterator>
      node_id_to_bucket_iterator_;
  int num_entries_;

};

#endif // GAIN_BUCKET_STANDARD_H
