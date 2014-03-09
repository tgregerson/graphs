#include "vcd_parser.h"

#include <fstream>
#include <iostream>
#include <set>

#include "universal_macros.h"

using namespace std;

int main(int argc, char *argv[]) {

  if (argc < 3) {
    cout << "Usage: vcd_dumpvars_generator signal_file output_file [scope]" << endl;
    exit(1);
  }

  string scope;
  if (argc == 4) {
    scope = argv[3];
  }

  VcdParser::WriteDumpVars(argv[1], argv[2], scope);
  return 0;
}
