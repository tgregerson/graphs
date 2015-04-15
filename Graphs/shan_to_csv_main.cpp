/*
 * shan_to_csv.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: gregerso
 */

#include <cassert>

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "structural_netlist_lexer.h"
#include "tclap/CmdLine.h"

using namespace std;

struct BitEntropyComparisonData {
  vector<double> slice_differences;
  vector<double> difference_average{0.0};
  vector<double> difference_stdev{0.0};
};

template <typename T>
void ReadEntropyData(istream& in, int num_slices, T* entropy_data) {
  string cur_line;
  string signal_name;
  string bit_select;
  getline(in, cur_line);
  do {
    string remainder =
        StructuralNetlistLexer::ConsumeIdentifier(cur_line, &signal_name);
    try {
      // This may either be the bitrange for an unscoped identifier or the
      // genvar index for a scoped generated identifier.
      remainder =
          StructuralNetlistLexer::ConsumeBitRange(remainder, &bit_select);
      signal_name.append(bit_select);
    } catch (std::exception& e) {}

    string shannon_entropy;
    vector<double> slice_entropies;
    for (int i = 0; i < num_slices; ++i) {
      remainder = StructuralNetlistLexer::ConsumeRealImmediate(remainder, &shannon_entropy);
      // Correct for strtod not accepting exponents with leading zeroes.
      size_t exp_pos = shannon_entropy.find('e');
      if (exp_pos != string::npos) {
        if ('-' == shannon_entropy.at(exp_pos + 1)) {
          ++exp_pos;
        }
        size_t pos;
        for (pos = exp_pos; pos < shannon_entropy.length(); ++pos) {
          char c = shannon_entropy.at(pos);
          if (isdigit(c) && '0' != c) {
            break;
          }
        }
        shannon_entropy = shannon_entropy.substr(0, exp_pos + 1) +
                          shannon_entropy.substr(pos, string::npos);
      }
      double entropy = strtod(shannon_entropy.c_str(), nullptr);
      assert(entropy >= 0.0 && entropy <= 1.0);
      slice_entropies.push_back(entropy);
    }
    entropy_data->insert(make_pair(signal_name, slice_entropies));

    getline(in, cur_line);
  } while (!in.eof());
}

int main(int argc, char *argv[]) {

  TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

  TCLAP::ValueArg<string> shan_entropy_input_file_flag(
      "s", "shan", "Shan input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> result_output_file_flag(
      "o", "output", "Compared output file name", false, "", "string",
      cmd);

  cmd.parse(argc, argv);

  ifstream shan_in_file;
  shan_in_file.open(shan_entropy_input_file_flag.getValue());
  assert(shan_in_file.is_open());

  int num_time_slices;
  shan_in_file >> num_time_slices;
  if (shan_in_file.fail()) {
    num_time_slices = 1;
    shan_in_file.clear();
  }

  std::ofstream outfile;
  if (result_output_file_flag.isSet()) {
    outfile.open(result_output_file_flag.getValue());
    assert(outfile.is_open());
  }

  std::ostream& os = (outfile.is_open()) ? outfile : std::cout;

  unordered_map<string, vector<double>> entropy_data;
  ReadEntropyData(shan_in_file, num_time_slices, &entropy_data);

  std::unordered_map<std::string, BitEntropyComparisonData> comparison_data;
  os << "SigName,Entropy\n";
  for (const auto& entry : entropy_data) {
    const string& signal_name = entry.first;
    os << signal_name;
    const vector<double>& edata = entry.second;
    for (size_t i = 0; i < edata.size(); ++i) {
      os << "," << edata.at(i);
    }
    os << endl;
  }
  return 0;
}
