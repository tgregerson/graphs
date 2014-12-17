#include "vcd_parser.h"

#include <cassert>
#include <cstdio>

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>

#include "vcd_lexer.h"
#include "universal_macros.h"

using namespace std;

int main(int argc, char *argv[]) {

  if (argc != 2) {
    cout << "Usage: vcd_parser vcd_file" << endl;
    exit(1);
  }

  std::chrono::high_resolution_clock::time_point t_start =
      std::chrono::high_resolution_clock::now();

  //string file_contents;
  //ifstream in_file(argv[1], ios::in | ios::binary);
  //in_file.seekg(0, ios::end);
  FILE* in_file = fopen(argv[1], "r");
  assert(in_file != nullptr);
  //std::streampos end_pos = in_file.tellg();
  fseeko(in_file, 0, SEEK_END);
  off_t end_pos = ftello(in_file);
  fseeko(in_file, 0, SEEK_SET);
  /*
  if (end_pos > 1e9) {
    cout << "File size too big: " << end_pos << endl;
    throw std::exception();
  }
  */
  //file_contents.resize(in_file.tellg());
  //in_file.seekg(0, ios::beg);

  /*
  cout << "Start reading" << endl;

  in_file.read(&file_contents[0], file_contents.size());

  cout << "Start Lexing" << endl;
  */

  vcd_parser::vcd_token::VcdDefinitions vd;
  //VcdParser::ParseVcdDefinitions(file_contents, &vd, false);
  //vcd_lexer::ConsumeVcdDefinitions(in_file, nullptr);
  vcd_parser::ParseVcdDefinitions(in_file, &vd);

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
