/*
 * compare_vcd_main.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: gregerso
 */

#include <cassert>
#include <cstdio>

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "file_helpers.h"
#include "signal_entropy_info.h"
#include "tclap/CmdLine.h"
#include "vcd_parser.h"

using namespace std;

struct SigEntropyComparisonData {
  int width{-1};
  vector<vector<double>> slice_bit_differences;
  vector<double> bit_difference_average{0.0};
  vector<double> bit_difference_stdev{0.0};
};

int main(int argc, char *argv[]) {

  TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

  TCLAP::ValueArg<string> gold_vcd_input_file_flag(
      "g", "gold", "Gold input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> cmp_vcd_input_file_flag(
      "c", "compare", "Compared input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> result_output_file_flag(
      "o", "output", "Compared input file name", false, "", "string",
      cmd);

  TCLAP::ValueArg<int> time_interval_micro(
      "t", "time_interval_u", "Time interval in microseconds", false, 0, "int",
      cmd);

  cmd.parse(argc, argv);

  FILE* gold_in_file = fopen(gold_vcd_input_file_flag.getValue().c_str(), "r");
  assert(!fhelp::IsError(gold_in_file));
  FILE* cmp_in_file = fopen(cmp_vcd_input_file_flag.getValue().c_str(), "r");
  assert(!fhelp::IsError(gold_in_file));

  std::ofstream outfile;
  if (result_output_file_flag.isSet()) {
    outfile.open(result_output_file_flag.getValue());
    assert(outfile.is_open());
  }

  std::ostream& os = (outfile.is_open()) ? outfile : std::cout;

  vcd_parser::vcd_token::VcdDefinitions vd;
  long long int interval_micro =
      (time_interval_micro.isSet()) ?
          1000000 * time_interval_micro.getValue() : 0;

  std::unordered_map<std::string, SignalEntropyInfo> gold_entropy_data;
  vcd_parser::EntropyFromVcdDefinitions(
      gold_in_file, &gold_entropy_data, false, os, 0, interval_micro);
  cout << "Done processing " << gold_vcd_input_file_flag.getValue() << endl;
  fclose(gold_in_file);

  std::unordered_map<std::string, SignalEntropyInfo> cmp_entropy_data;
  vcd_parser::EntropyFromVcdDefinitions(
      cmp_in_file, &cmp_entropy_data, false, os, 0, interval_micro);
  cout << "Done processing " << cmp_vcd_input_file_flag.getValue() << endl;
  fclose(cmp_in_file);

  std::unordered_map<std::string, SigEntropyComparisonData> comparison_data;
  for (const auto& entry : cmp_entropy_data) {
    const string& signal_name = entry.first;
    if (gold_entropy_data.find(signal_name) != gold_entropy_data.end()) {
      SigEntropyComparisonData cdata_entry;
      const SignalEntropyInfo& gold_entry = gold_entropy_data.at(signal_name);
      const SignalEntropyInfo& cmp_entry = entry.second;
      assert(gold_entry.width == cmp_entry.width);
      cdata_entry.width = gold_entry.width;
      cdata_entry.slice_bit_differences.resize(gold_entry.time_slices.size());
      for (size_t i = 0; i < gold_entry.time_slices.size(); ++i) {
        const SignalEntropyTimeSlice& gold_slice = gold_entry.time_slices.at(i);
        const SignalEntropyTimeSlice& cmp_slice = cmp_entry.time_slices.at(i);
        for (size_t j = 0; j < gold_slice.bit_info.size(); ++j) {
          const BitEntropyInfo& gold_bit = gold_slice.bit_info.at(j);
          const BitEntropyInfo& cmp_bit = cmp_slice.bit_info.at(j);
          cdata_entry.slice_bit_differences.at(i).push_back(
              cmp_bit.min_entropy() - gold_bit.min_entropy());
        }
      }
      comparison_data.insert(make_pair(signal_name, cdata_entry));
    }
  }

  cout << gold_entropy_data.size() - comparison_data.size()
       << " unpaired gold signals\n";
  cout << cmp_entropy_data.size() - comparison_data.size()
       << " unpaired gold signals\n";

  return 0;
}



