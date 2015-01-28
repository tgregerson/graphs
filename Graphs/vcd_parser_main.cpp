#include "vcd_parser.h"

#include <cassert>
#include <cstdio>

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>

#include "file_helpers.h"
#include "tclap/CmdLine.h"
#include "universal_macros.h"
#include "vcd_lexer.h"

using namespace std;

int main(int argc, char *argv[]) {

  TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

  TCLAP::ValueArg<string> vcd_input_file_flag(
      "v", "vcd", "VCD input file name", true, "", "string",
      cmd);

  TCLAP::ValueArg<string> entropy_output_file_flag(
      "e", "entropy", "Entropy output file name", false, "", "string",
      cmd);

  TCLAP::ValueArg<int> time_interval_micro(
      "t", "time_interval_u", "Time interval in microseconds", false, 0, "int",
      cmd);

  TCLAP::ValueArg<int> max_mb(
      "m", "max_mb", "Stop after parsing this many megabytes", false, 0, "int",
      cmd);

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
      (time_interval_micro.isSet()) ?
          1000000 * time_interval_micro.getValue() : 0;

  vcd_parser::EntropyFromVcdDefinitions(
      in_file, os, max_mb.getValue(), interval_micro);

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
