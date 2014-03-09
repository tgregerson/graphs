#include "vcd_parser.h"

#include <fstream>
#include <iostream>
#include <set>

#include "universal_macros.h"

using namespace std;

int main(int argc, char *argv[]) {

  if (argc != 3) {
    cout << "Usage: vcd_parser vcd_file signal_file" << endl;
    exit(1);
  }

  ifstream input_file(argv[2]);
  assert_b(input_file.is_open()) {
    cout << "Failed to open signal file: " << input_file
         << ". Terminating." << endl;
  }

  string signal;
  set<string> signals;
  input_file >> signal;
  while (!input_file.eof()) {
    signals.insert(signal);
    input_file >> signal;
  }

  VcdParser vcd_parser(argv[1], signals);
  vcd_parser.Parse(true);
  return 0;
}
