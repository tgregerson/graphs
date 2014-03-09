#ifndef GAIN_BUCKET_ENTRY_H_
#define GAIN_BUCKET_ENTRY_H_

#include "node.h"

#include <cassert>
#include <vector>

class GainBucketEntry {
 public:
  GainBucketEntry() {}
  GainBucketEntry(int gain, int id, const std::vector<int>& weight_vector,
                  int current_weight_vector_index = 0)
    : gain(gain), id(id),
      current_weight_vector_index(current_weight_vector_index) {
    all_weight_vectors.push_back(weight_vector);     
  }
  GainBucketEntry(int gain, Node* node) : gain(gain), id(node->id),
      current_weight_vector_index(node->selected_weight_vector_index()) { 
    assert(!node->WeightVectors().empty());
    for (auto& wv : node->WeightVectors()) {
      all_weight_vectors.push_back(wv);
    }
    assert(node->SelectedWeightVector() == current_weight_vector());
  }

  const std::vector<int>& current_weight_vector() const {
    assert(current_weight_vector_index < all_weight_vectors.size());
    return all_weight_vectors[current_weight_vector_index];
  };

  int gain;
  int id;
  int current_weight_vector_index;
  std::vector<std::vector<int>> all_weight_vectors;
};

#endif // GAIN_BUCKET_ENTRY_H
