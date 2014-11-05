#ifndef GAIN_BUCKET_INTERFACE_H_
#define GAIN_BUCKET_INTERFACE_H_

#ifndef MAX_GAIN
#define MAX_GAIN 4000
#endif

#include <vector>

#include "edge_klfm.h"
#include "gain_bucket_entry.h"

class GainBucketInterface {
 public:
  virtual ~GainBucketInterface() {}

  // Add 'entry' to the bucket.
  virtual void Add(GainBucketEntry entry) = 0;

  // Returns the entry of the highest gain element.
  virtual GainBucketEntry Top() const = 0;

  // Removes the entry corresponding to Top() from the gain bucket.
  virtual void Pop() = 0;

  // Returns false if there are any entries in the bucket.
  virtual bool Empty() const = 0;

  // Add the gain modifier to each of the nodes in the vector.
  virtual void UpdateGains(double gain_modifier,
                           const EdgeKlfm::NodeIdVector& nodes_to_update) = 0;

  // Returns true if the node with 'node_id' is in the bucket.
  virtual bool HasNode(int node_id) = 0;

  // Removes the node with 'node_id' and returns its entry.
  virtual GainBucketEntry RemoveByNodeId(int node_id) = 0;

  // Returns a reference to the gain bucket entry with 'node_id'.
  virtual GainBucketEntry& GbeRefByNodeId(int node_id) = 0;

  // Print debug information.
  virtual void Print(bool condensed) const = 0;

  // Return the number of entries in the bucket.
  virtual int num_entries() const = 0;
};

#endif // GAIN_BUCKET_INTERFACE_H
