#include "lp_solve_interface.h"

using namespace std;

void print_usage_and_exit() {
  cout << "Usage: lp_solve_interface "
       << "(-chaco chaco_graph_input_file] | "
       << "-ntl ntl_input_file | "
       << "-mps mps_input_file) "
       << "[-write_mps mps_output_file] "
       << endl;
  exit(1);
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    print_usage_and_exit();
  }

  bool use_chaco = false;
  bool use_ntl = false;
  bool use_mps = false;
  bool write_output = false;

  string input_type_str(argv[1]);
  if (input_type_str == "-chaco") {
    use_chaco = true;
  } else if (input_type_str == "-ntl") {
    use_ntl = true;
  } else if (input_type_str == "-mps") {
    use_mps = true;
  } else {
    print_usage_and_exit();
  }
  string input_filename(argv[2]);

  string output_filename;
  if (argc > 3) {
    if (argc != 5) {
      print_usage_and_exit();
    }
    string write_output_str(argv[3]);
    if (write_output_str != "-write_mps") {
      print_usage_and_exit();
    }
    output_filename = argv[4];
  }

  LpSolveInterface interface;
  if (use_chaco) {
    interface.LoadFromChaco(input_filename);
  } else if (use_ntl) {
    interface.LoadFromNtl(input_filename);
  } else if (use_mps) {
    interface.LoadFromMps(input_filename);
  }

  if (write_output) {
    interface.WriteToMps(output_filename);
  }

  return 0;
}



