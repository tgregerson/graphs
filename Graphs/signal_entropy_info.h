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

// TODO this will break on other computers.
#include "/localhome/gregerso/git/signalcontent/src/base/four_value_logic.h"
#include "/localhome/gregerso/git/signalcontent/src/base/frame_fv.h"


struct BitEntropyInfo {
  void Update(char value_char, long long time_since_last_update) {
    switch (cur_val) {
      case signal_content::base::FourValueLogic::ZERO:
        num_0 += time_since_last_update;
        break;
      case signal_content::base::FourValueLogic::ONE:
        num_1 += time_since_last_update;
        break;
      case signal_content::base::FourValueLogic::X:
        num_x += time_since_last_update;
        break;
      case signal_content::base::FourValueLogic::Z:
        num_z += time_since_last_update;
        break;
      default: assert(false);
    }
    switch (value_char) {
      case '0':
        cur_val = signal_content::base::FourValueLogic::ZERO;
        break;
      case '1':
        cur_val = signal_content::base::FourValueLogic::ONE;
        break;
      case 'x':
      case 'X':
        cur_val = signal_content::base::FourValueLogic::X;
        break;
      case 'z':
      case 'Z':
        cur_val = signal_content::base::FourValueLogic::Z;
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

  signal_content::base::FourValueLogic cur_val{signal_content::base::FourValueLogic::X};
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

  signal_content::base::FourValueLogic cur_value() const {
    const SignalEntropyTimeSlice& cur_slice = time_slices.back();
    return cur_slice.bit_info.front().cur_val;
  }

  std::vector<signal_content::base::FourValueLogic> cur_value_vframe() const {
    std::vector<signal_content::base::FourValueLogic> vals;
    const SignalEntropyTimeSlice& cur_slice = time_slices.back();
    for (const BitEntropyInfo& bit_info : cur_slice.bit_info) {
      vals.push_back(bit_info.cur_val);
    }
    return vals;
  }

  int scope_prefix_code{-1};
  unsigned long long last_update_time{0};
  unsigned long long width{1};
  long long bit_low{0};
  std::string unscoped_orig_name;
  std::vector<SignalEntropyTimeSlice> time_slices;
};

class EntropyData {
 public:
  std::unordered_map<std::string, SignalEntropyInfo> signal_data;

  void AddCurrentVFrameFv() {
    signal_content::base::VFrameFv vframe;
    for (const auto& sig_pair : signal_data) {
      const SignalEntropyInfo& sig_info = sig_pair.second;
      if (sig_info.width > 1) {
        signal_content::base::VFrameFv sig_vals = sig_info.cur_value_vframe();
        for (signal_content::base::FourValueLogic fv : sig_vals) {
          vframe.push_back(fv);
        }
      } else {
        vframe.push_back(sig_info.cur_value());
      }
    }
    vfd.push_back(std::move(vframe));
  }
  signal_content::base::VFrameDeque vfd;
};


#endif /* SIGNAL_ENTROPY_INFO_H_ */
