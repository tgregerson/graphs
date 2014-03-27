#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cout << "Usage: " << argv[0] << " input_file output_file" << endl;
  }
  string input_filename = argv[1];
  string output_filename = argv[2];

  ifstream input(input_filename.c_str());
  ofstream output(output_filename.c_str());

  assert(input.is_open());
  assert(output.is_open());

  string token;
  input >> token;
  while (!input.eof()) {
    output << token << "\n";
    input >> token;
  }
  return 0;
}
