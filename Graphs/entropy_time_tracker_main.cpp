/*
 * entropy_time_tracker.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: gregerso
 */


#include <cassert>
#include <cstdio>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

#include "structural_netlist_lexer.h"
#include "tclap/CmdLine.h"

using namespace std;

struct SliceValues {
  void Accumulate(const SliceValues& sv) {
    num_zero += sv.num_zero;
    num_one += sv.num_one;
    num_x += sv.num_x;
    num_z += sv.num_z;
  }
  long long unsigned int num_zero{0};
  long long unsigned int num_one{0};
  long long unsigned int num_x{0};
  long long unsigned int num_z{0};
};

double MinSliceEntropy(const SliceValues& sv) {
  long long unsigned int effective_zeroes = sv.num_zero;
  long long unsigned int effective_ones = sv.num_one;
  if (effective_zeroes >= effective_ones) {
    effective_zeroes += (sv.num_x + sv.num_z);
  } else {
    effective_ones += (sv.num_x + sv.num_z);
  }
  long long int total = effective_zeroes + effective_ones;
  double p_zero = double(effective_zeroes) / double(total);
  double p_one = 1.0 - p_zero;
  if (p_zero <= 0.0 || p_one <= 0.0) {
    return 0.0;
  } else {
    return -(p_zero)*log2(p_zero) + -(p_one)*log2(p_one);
  }
}

int main(int argc, char *argv[]) {

  TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

  TCLAP::ValueArg<string> entropy_input_file_flag(
      "e", "entropy", "Entropy input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> whitelist_input_file_flag(
      "w", "whitelist_signals", "Whitelist signals input file name", false,
      "", "string", cmd);

  TCLAP::ValueArg<string> output_file_flag(
      "o", "output", "Output file", false, "", "string",
      cmd);

  cmd.parse(argc, argv);

  ifstream entropy_in_file;
  entropy_in_file.open(entropy_input_file_flag.getValue());
  assert(entropy_in_file.is_open());

  unordered_set<string> whitelist_signals;
  if (whitelist_input_file_flag.isSet()) {
    ifstream whitelist_in_file;
    whitelist_in_file.open(whitelist_input_file_flag.getValue());
    assert(whitelist_in_file.is_open());
    while (!whitelist_in_file.eof()) {
      string signal_name;
      if (StructuralNetlistLexer::ConsumeIdentifierStream(
          whitelist_in_file, &signal_name)) {
        string bit_range;
        if (StructuralNetlistLexer::ConsumeBitRangeStream(
            whitelist_in_file, &bit_range)) {
              signal_name.append(bit_range);
        }
        whitelist_signals.insert(signal_name);
      }
    }
    cout << "Whitelisted " << whitelist_signals.size() << " signals.\n";
  }

  map<string, vector<SliceValues>> signal_slices;
  vector<double> total_slice_entropy;
  string num_slice_str;
  unsigned long long num_time_slices;
  if (StructuralNetlistLexer::ConsumeUnbasedImmediateStream(
      entropy_in_file, &num_slice_str)) {
    num_time_slices = strtoull(num_slice_str.c_str(), nullptr, 10);
  } else {
    num_time_slices = 1;
  }
  cout << "Tracking data for " << num_time_slices << " time slices.\n";
  total_slice_entropy.resize(num_time_slices);
  while (!entropy_in_file.eof()) {
    string signal_name;
    StructuralNetlistLexer::ConsumeIdentifierStream(
        entropy_in_file, &signal_name);
    string bit_range;
    if (StructuralNetlistLexer::ConsumeBitRangeStream(
        entropy_in_file, &bit_range)) {
      signal_name.append(bit_range);
    }
    cout << "Signal: " << signal_name << endl;
    if (!signal_name.empty()) {
      if (whitelist_signals.empty() ||
          whitelist_signals.find(signal_name) != whitelist_signals.end()) {
        vector<SliceValues> slice_values;
        for (unsigned long long int i = 0; i < num_time_slices; ++i) {
          SliceValues sv;
          entropy_in_file >> sv.num_zero;
          entropy_in_file >> sv.num_one;
          entropy_in_file >> sv.num_x;
          entropy_in_file >> sv.num_z;
          slice_values.push_back(sv);
          total_slice_entropy.at(i) += MinSliceEntropy(sv);
        }
        signal_slices.insert(make_pair(signal_name, slice_values));
        cout << "Added " << signal_name << endl;
      }
    } else {
      break;
    }
    // Skip to next line.
    entropy_in_file.ignore(numeric_limits<streamsize>::max(), '\n');
  }
  cout << "Found " << signal_slices.size() << " signals.\n";

  for (const string& wl_signal : whitelist_signals) {
    if (signal_slices.find(wl_signal) == signal_slices.end()) {
      cout << "Didn't find entropy for whitelist signal: " << wl_signal << endl;
    }
  }

  ofstream outfile;
  if (output_file_flag.isSet()) {
    outfile.open(output_file_flag.getValue());
    assert(outfile.is_open());
  }

  ostream& os = (outfile.is_open()) ? outfile : std::cout;

  for (const auto& sig : signal_slices) {
    os << sig.first << " ";
    for (const SliceValues& slice : sig.second) {
      os << MinSliceEntropy(slice) << " ";
    }
    os << endl;
  }

  os << "Total (" << signal_slices.size() << ") ";
  for (double slice_entropy : total_slice_entropy) {
    os << slice_entropy << " ";
  }
  os << endl;
  os << "TotalPercent (" << signal_slices.size() << ") ";
  for (double slice_entropy : total_slice_entropy) {
    os << (slice_entropy / signal_slices.size()) << " ";
  }
  os << endl;

  return 0;
}
