#include "vcd_parser.h"

#include <cassert>
#include <cstdio>

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <unordered_map>

#include "file_helpers.h"
#include "signal_entropy_info.h"
#include "tclap/CmdLine.h"
#include "universal_macros.h"
#include "vcd_lexer.h"

#include "/localhome/gregerso/git/signalcontent/src/codec/huffman.h"

using namespace std;
using namespace signal_content::base;

int main(int argc, char *argv[]) {

  TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

  TCLAP::ValueArg<string> vcd_input_file_flag(
      "v", "vcd", "VCD input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> entropy_output_file_flag(
      "e", "entropy", "Entropy output file name", false, "", "string",
      cmd);

  TCLAP::ValueArg<int> time_interval_nano(
      "t", "time_interval_u", "Time interval in nanoseconds", false, 0, "int",
      cmd);

  TCLAP::ValueArg<int> max_mb(
      "m", "max_mb", "Stop after parsing this many megabytes", false, 0, "int",
      cmd);

  TCLAP::ValueArg<string> huffman_file_flag(
      "c", "huffman", "Collect Huffman compression data", false, "", "string", cmd);

  TCLAP::ValueArg<int> huffman_symbol_size_flag(
      "s", "symbol_size", "Set Huffman symbol size", false, 8, "int", cmd);

  cmd.parse(argc, argv);

  std::chrono::high_resolution_clock::time_point t_start =
      std::chrono::high_resolution_clock::now();

  FILE* in_file = fopen(vcd_input_file_flag.getValue().c_str(), "r");
  assert(!fhelp::IsError(in_file));

  std::ofstream outfile;
  if (entropy_output_file_flag.isSet()) {
    outfile.open(entropy_output_file_flag.getValue());
    assert(outfile.is_open());
  }

  std::ostream& os = (outfile.is_open()) ? outfile : std::cout;

  const auto initial_pos = fhelp::GetPosition(in_file);
  fhelp::SeekToEnd(in_file);
  const auto end_pos = fhelp::GetPosition(in_file);
  fhelp::SeekToPosition(in_file, initial_pos);

  vcd_parser::vcd_token::VcdDefinitions vd;
  long long int interval_micro =
      (time_interval_nano.isSet()) ?
          1000 * time_interval_nano.getValue() : 0;

  bool huffman_enabled = huffman_file_flag.isSet() ||
       huffman_symbol_size_flag.isSet();
  EntropyData entropy_data;
  vcd_parser::EntropyFromVcdDefinitions(
      in_file, &entropy_data, true, os, max_mb.getValue(), interval_micro,
      huffman_enabled);
  if (huffman_enabled) {
    cout << "Populating Huffman Codec\n";
    signal_content::codec::HuffmanCodec codec(
        entropy_data.vfd, huffman_symbol_size_flag.getValue());
    codec.PrintCompressionData();
    if (huffman_file_flag.isSet()) {
      std::ofstream huffman_outfile;
      huffman_outfile.open(huffman_file_flag.getValue());
      assert(huffman_outfile.is_open());
      huffman_outfile << "Uncompressed Frame Size\n";
      huffman_outfile << entropy_data.vfd.front().size() *
                         huffman_symbol_size_flag.getValue()
                      << "\n";
      huffman_outfile << "Compressed Frame Sizes\n";
      for (size_t i = 0; i < entropy_data.vfd.size(); ++i) {
        if (i % 1000 == 0) {
          cout << "Encoded frame " << i << " of " << entropy_data.vfd.size()
              << endl;
        }
        const VFrameFv& frame = entropy_data.vfd.at(i);
        vector<bool> encoded = codec.EncodeFrame(frame);
        huffman_outfile << encoded.size() << "\n";
      }
    }
  }

  cout << "Done processing " << vcd_input_file_flag.getValue()
       << ". Bye!" << endl;

  std::chrono::high_resolution_clock::time_point t_end =
      std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> duration_s =
      std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start);

  double Bps = double(end_pos) / duration_s.count();
  double MBps = Bps / (1024 * 1024);
  cout << "Data rate: " << MBps << " MB/s" << endl;
  cout << "Size: " << end_pos << " bytes" << endl;
  cout << "Time: " << duration_s.count() << "s" << endl;

  fclose(in_file);
  return 0;
}
