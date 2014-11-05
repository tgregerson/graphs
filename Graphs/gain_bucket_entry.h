#ifndef GAIN_BUCKET_ENTRY_H_
#define GAIN_BUCKET_ENTRY_H_

#include "node.h"

#include <cassert>
#include <vector>

class GainBucketEntry {
 public:
  GainBucketEntry() {}

  GainBucketEntry(double cost, int id, const std::vector<int>& weight_vector,
                  int current_weight_vector_index = 0)
    : cost_(cost), id_(id),
      current_weight_vector_index_(current_weight_vector_index) {
    gain_ = FastDoublePower2Mul(cost, kExponent);
    all_weight_vectors_.push_back(weight_vector);
  }

  GainBucketEntry(double cost, Node* node) : cost_(cost), id_(node->id),
      current_weight_vector_index_(node->selected_weight_vector_index()) {
    gain_ = FastDoublePower2Mul(cost, kExponent);
    assert(!node->WeightVectors().empty());
    for (auto& wv : node->WeightVectors()) {
      all_weight_vectors_.push_back(wv);
    }
    assert(node->SelectedWeightVector() == current_weight_vector());
  }

  int Id() const { return id_; }
  double CostGain() const { return cost_; }
  void SetCostGain(double cost) { cost_ = cost; }
  int GainIndex() const { return gain_; }
  void SetGainIndex(int gain) { gain_ = gain; }

  int CurrentWeightVectorIndex() const {
    return current_weight_vector_index_;
  }
  void SetCurrentWeightVectorIndex(int cwvi) {
    current_weight_vector_index_ = cwvi;
  }

  const std::vector<std::vector<int>>& AllWeightVectors() const {
    return all_weight_vectors_;
  }

  const std::vector<int>& current_weight_vector() const {
    assert(current_weight_vector_index_ < all_weight_vectors_.size());
    return all_weight_vectors_[current_weight_vector_index_];
  };

 private:
  static const int kExponent{8};
  // Performs fast multiplication of a double by a power of 2,
  // similar to shifting an int. Returns input * 2^exponent.
  // Does not handle special double conditions like NaN, infinity,
  // or subnormal.
  double FastDoublePower2Mul(double input, int exponent) {
    long long exponent_masked = static_cast<long long>(input) & 0x7FF00000ULL;
    long long exponent_val = exponent_masked >> 52;
    exponent_val += static_cast<long long>(exponent);
    exponent_masked = exponent_val << 52;
    return static_cast<double>(
        (static_cast<long long>(input) & 0x800FFFFFULL) & exponent_masked);
  }

  int gain_{0};
  double cost_{0.0};
  int id_{0};
  size_t current_weight_vector_index_{0};
  std::vector<std::vector<int>> all_weight_vectors_;
};

#endif // GAIN_BUCKET_ENTRY_H
