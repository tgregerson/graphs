/*
 * compare_entropy.cpp
 *
 *  Created on: Feb 6, 2015
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

string TransformName(const string& sig_name) {
  if ('\\' != sig_name.at(0)) {
    string escaped_name = "\\" + sig_name;
    size_t range_start_pos = escaped_name.rfind('[');
    if (range_start_pos == string::npos ||
        escaped_name.back() != ']') {
      // No range.
      escaped_name.push_back(' ');
    } else {
      escaped_name = escaped_name.substr(0, range_start_pos) + " " +
                  escaped_name.substr(range_start_pos, string::npos);
    }
    return escaped_name;
  } else {
    return sig_name;
  }
}

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

  TCLAP::ValueArg<string> gold_entropy_input_file_flag(
      "g", "gold", "Gold input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> cmp_entropy_input_file_flag(
      "c", "compare", "Compared input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> whitelist_signals_input_file_flag(
      "w", "whitelist", "Whitelist signals input file name", false, "", "string",
      cmd);

  TCLAP::ValueArg<string> result_output_file_flag(
      "o", "output", "Compared output file name", false, "", "string",
      cmd);

  cmd.parse(argc, argv);

  ifstream gold_in_file;
  gold_in_file.open(gold_entropy_input_file_flag.getValue());
  assert(gold_in_file.is_open());

  int num_time_slices;
  gold_in_file >> num_time_slices;
  if (gold_in_file.fail()) {
    num_time_slices = 1;
    gold_in_file.clear();
  }

  ifstream cmp_in_file;
  cmp_in_file.open(cmp_entropy_input_file_flag.getValue());
  assert(cmp_in_file.is_open());

  int num_time_slices_check;
  cmp_in_file >> num_time_slices_check;
  if (cmp_in_file.fail()) {
    assert(1 == num_time_slices);
    cmp_in_file.clear();
  } else {
    assert(num_time_slices == num_time_slices_check);
  }

  unordered_set<string> whitelist_signals;
  if (whitelist_signals_input_file_flag.isSet()) {
    ifstream whitelist_in_file;
    whitelist_in_file.open(whitelist_signals_input_file_flag.getValue());
    assert(cmp_in_file.is_open());
    string signal_name;
    string bit_range;
    StructuralNetlistLexer::ConsumeWhitespaceStream(
        whitelist_in_file, nullptr);
    while (!whitelist_in_file.eof()) {
      assert(StructuralNetlistLexer::ConsumeIdentifierStream(
          whitelist_in_file, &signal_name));
      bool has_range = StructuralNetlistLexer::ConsumeBitRangeStream(
          whitelist_in_file, &bit_range);
      if (has_range) {
        signal_name.append(bit_range);
      }
      whitelist_signals.insert(signal_name);
      StructuralNetlistLexer::ConsumeWhitespaceStream(
          whitelist_in_file, nullptr);
    }
  }

  std::ofstream outfile;
  if (result_output_file_flag.isSet()) {
    outfile.open(result_output_file_flag.getValue());
    assert(outfile.is_open());
  }

  std::ostream& os = (outfile.is_open()) ? outfile : std::cout;

  unordered_map<string, vector<double>> gold_entropy_data;
  ReadEntropyData(gold_in_file, num_time_slices, &gold_entropy_data);

  unordered_map<string, vector<double>> cmp_entropy_data;
  ReadEntropyData(cmp_in_file, num_time_slices, &cmp_entropy_data);

  std::unordered_map<std::string, BitEntropyComparisonData> comparison_data;
  for (const auto& entry : cmp_entropy_data) {
    const string& signal_name = entry.first;
    if (!whitelist_signals_input_file_flag.isSet() ||
        whitelist_signals.find(signal_name) != whitelist_signals.end()) {
      auto gold_it = gold_entropy_data.find(signal_name);
      if (gold_it == gold_entropy_data.end()) {
        gold_it = gold_entropy_data.find(TransformName(signal_name));
      }
      if (gold_it != gold_entropy_data.end()) {
        BitEntropyComparisonData cdata_entry;
        const vector<double>& gold_data = gold_it->second;
        const vector<double>& cmp_data = entry.second;
        for (size_t i = 0; i < gold_data.size(); ++i) {
          assert(cmp_data.at(i) <= 1.0 && cmp_data.at(i) >= 0.0);
          assert(gold_data.at(i) <= 1.0 && gold_data.at(i) >= 0.0);
          cdata_entry.slice_differences.push_back(cmp_data.at(i) - gold_data.at(i));
        }
        comparison_data.insert(make_pair(gold_it->first, cdata_entry));
      }
    }
  }

  os << "SigName,EntropyDiff,AbsEntropyDiff\n";
  for (const auto& cdata_entry_pair : comparison_data) {
    const string& signal_name = cdata_entry_pair.first;
    const BitEntropyComparisonData& entry = cdata_entry_pair.second;
    os << signal_name;
    for (double d : entry.slice_differences) {
      os << "," << d << "," << abs(d);
    }
    os << endl;
  }

  if (whitelist_signals_input_file_flag.isSet()) {
    cout << "Whitelisted " << whitelist_signals.size() << " signals\n";
  }
  cout << "Matched " << comparison_data.size() << " signals\n";
  cout << gold_entropy_data.size() - comparison_data.size()
       << " unpaired gold signals\n";
  cout << cmp_entropy_data.size() - comparison_data.size()
       << " unpaired compare signals\n";

  return 0;
}






