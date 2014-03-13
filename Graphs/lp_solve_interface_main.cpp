#include "lp_solve_interface.h"

#include <cstdlib>

using namespace std;

void print_usage_and_exit() {
  cout << "Usage: lp_solve_interface timeout_s "
       << "(-chaco chaco_graph_input_file] | "
       << "-ntl ntl_input_file | "
       << "-mps mps_input_file) "
       << "[-write_mps mps_output_file] "
       << endl;
  exit(1);
}

int main(int argc, char *argv[]) {

  if (argc < 4) {
    print_usage_and_exit();
  }

  bool use_chaco = false;
  bool use_ntl = false;
  bool use_mps = false;
  bool write_mps = false;

  long timeout_s = atol(argv[1]);

  string input_type_str(argv[2]);
  if (input_type_str == "-chaco") {
    use_chaco = true;
  } else if (input_type_str == "-ntl") {
    use_ntl = true;
  } else if (input_type_str == "-mps") {
    use_mps = true;
  } else {
    print_usage_and_exit();
  }
  string input_filename(argv[3]);

  string output_mps_filename;
  if (argc > 4) {
    if (argc != 6) {
      print_usage_and_exit();
    }
    string write_output_str(argv[4]);
    if (write_output_str != "-write_mps") {
      print_usage_and_exit();
    }
    write_mps = true;
    output_mps_filename = argv[5];
  }

  LpSolveInterface interface;
  if (use_chaco) {
    interface.LoadFromChaco(input_filename);
  } else if (use_ntl) {
    interface.LoadFromNtl(input_filename);
  } else if (use_mps) {
    interface.LoadFromMps(input_filename);
  }

  interface.RunSolver(timeout_s);

  if (write_mps) {
    interface.WriteToMps(output_mps_filename);
  }

  return 0;
}



