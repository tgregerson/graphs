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
#include "vcd_lexer.h"
#include "universal_macros.h"

using namespace std;

int main(int argc, char *argv[]) {

  if (argc != 2 && argc != 3) {
    cout << "Usage: vcd_parser vcd_file [output_file]" << endl;
    exit(1);
  }

  std::chrono::high_resolution_clock::time_point t_start =
      std::chrono::high_resolution_clock::now();

  //ifstream in_file(argv[1], ios::in | ios::binary);
  FILE* in_file = fopen(argv[1], "r");
  assert(!fhelp::IsError(in_file));

  std::ofstream outfile;
  if (argc == 3) {
    outfile.open(argv[2]);
    assert(outfile.is_open());
  }

  std::ostream& os = (outfile.is_open()) ? outfile : std::cout;

  const auto initial_pos = fhelp::GetPosition(in_file);
  fhelp::SeekToEnd(in_file);
  const auto end_pos = fhelp::GetPosition(in_file);
  fhelp::SeekToPosition(in_file, initial_pos);
  /*
  if (end_pos > 1e9) {
    cout << "File size too big: " << end_pos << endl;
    throw std::exception();
  }
  */

  vcd_parser::vcd_token::VcdDefinitions vd;
  /*
  const long long pre_reserve = end_pos / 5;
  cout << "Pre-reserving " << pre_reserve << " sim commands" << endl;
  vd.simulation_commands.reserve(pre_reserve);
  */

  //VcdParser::ParseVcdDefinitions(file_contents, &vd, false);
  //vcd_lexer::ConsumeVcdDefinitions(in_file, nullptr);
  //vcd_parser::ParseVcdDefinitions(in_file, &vd);
  vcd_parser::EntropyFromVcdDefinitions(in_file, os);

  cout << "Done processing " << argv[1] << ". Bye!" << endl;

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
  //in_file.close();
  return 0;
}
