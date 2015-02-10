/*
 * signal_entropy_info.h
 *
 *  Created on: Feb 6, 2015
 *      Author: gregerso
 */

#ifndef SIGNAL_ENTROPY_INFO_H_
#define SIGNAL_ENTROPY_INFO_H_

#include <cassert>
#include <cmath>

#include <string>
#include <vector>

enum class FourValueLogic {
  ZERO,
  ONE,
  X,
  Z,
};

struct BitEntropyInfo {
  void Update(char value_char, long long time_since_last_update) {
    switch (cur_val) {
      case FourValueLogic::ZERO:
        num_0 += time_since_last_update;
        break;
      case FourValueLogic::ONE:
        num_1 += time_since_last_update;
        break;
      case FourValueLogic::X:
        num_x += time_since_last_update;
        break;
      case FourValueLogic::Z:
        num_z += time_since_last_update;
        break;
      default: assert(false);
    }
    switch (value_char) {
      case '0':
        cur_val = FourValueLogic::ZERO;
        break;
      case '1':
        cur_val = FourValueLogic::ONE;
        break;
      case 'x':
      case 'X':
        cur_val = FourValueLogic::X;
        break;
      case 'z':
      case 'Z':
        cur_val = FourValueLogic::Z;
        break;
      default: assert(false);
    }
    cur_val_char = value_char;
  }

  double p_0() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_0) / double(total);
    }
  }
  double p_1() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_1) / double(total);
    }
  }
  double p_x() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_x) / double(total);
    }
  }
  double p_z() const {
    long long total = total_time();
    if (total == 0) {
      return 0.0;
    } else {
      return double(num_z) / double(total);
    }
  }
  double min_entropy() const {
    double p0_adj = p_0();
    double p1_adj = p_1();
    if (p0_adj >= p1_adj) {
      p0_adj += p_x() + p_z();
    } else {
      p1_adj += p_x() + p_z();
    }
    if (p0_adj <= 0.0 || p0_adj >= 1.0 ||
        p1_adj <= 0.0 || p1_adj >= 1.0) {
      return 0.0;
    } else {
      return -(p0_adj)*log2(p0_adj) + -(p1_adj)*log2(p1_adj);
    }
  }

  long long int total_time() const {
    return num_0 + num_1 + num_x + num_z;
  }

  FourValueLogic cur_val{FourValueLogic::X};
  char cur_val_char{'x'};
  long long int num_0{0};
  long long int num_1{0};
  long long int num_x{0};
  long long int num_z{0};
};

struct SignalEntropyTimeSlice {
  void InitFrom(const SignalEntropyTimeSlice& previous_slice) {
    start_time = previous_slice.end_time;
    bit_info.resize(previous_slice.bit_info.size());
    for (size_t i = 0; i < bit_info.size(); ++i) {
      bit_info.at(i).cur_val = previous_slice.bit_info.at(i).cur_val;
      bit_info.at(i).cur_val_char = previous_slice.bit_info.at(i).cur_val_char;
    }
  }
  unsigned long long start_time{0};
  unsigned long long end_time{0};
  std::vector<BitEntropyInfo> bit_info;
};

struct SignalEntropyInfo {
  SignalEntropyInfo() {
    time_slices.resize(1);
  }

  SignalEntropyTimeSlice& CurrentTimeSlice() {
    assert(!time_slices.empty());
    return time_slices.back();
  }

  void AdvanceTimeSlice(long long int current_time) {
    SignalEntropyTimeSlice& prev_slice = CurrentTimeSlice();
    prev_slice.end_time = current_time;
    SignalEntropyTimeSlice new_slice;
    new_slice.InitFrom(prev_slice);
    time_slices.push_back(new_slice); // prev_slice reference invalidated!
  }

  int scope_prefix_code{-1};
  unsigned long long last_update_time{0};
  unsigned long long width{1};
  long long bit_low{0};
  std::string unscoped_orig_name;
  std::vector<SignalEntropyTimeSlice> time_slices;
};

#endif /* SIGNAL_ENTROPY_INFO_H_ */
