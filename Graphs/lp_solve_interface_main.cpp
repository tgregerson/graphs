#include "lp_solve_interface.h"

#include <cstdlib>

#include "tclap/CmdLine.h"

using namespace std;

void print_usage_and_exit() {
  cout << "Usage: lp_solve_interface INPUT_ARGS [ADDITIONAL_OPTIONS*]" << endl
       << endl
       << "INPUT_ARGS: " << endl
       << "(-chaco chaco_graph_input_file] | "
       << "-ntl ntl_input_file | "
       << "-mps mps_input_file) " << endl
       << endl
       << "ADDITIONAL_OPTIONS:" << endl
       << "-imbalance imbalance_float" << endl
       << "-solve [SOLVE_OPTIONS*]" << endl
       << "-write_mps mps_output_file" << endl
       << endl
       << "SOLVE_OPTIONS:" << endl
       << "-timeout timeout_s "
       << endl;
  exit(1);
}

int main(int argc, char *argv[]) {

  if (argc < 4) {
    print_usage_and_exit();
  }

  bool solve = false;
  bool use_chaco = false;
  bool use_ntl = false;
  bool use_mps = false;
  bool write_mps = false;
  long timeout_s = 0;

  string input_filename;
  string output_filename;

  // Parse command-line arguments
  try {
    TCLAP::CmdLine cmd("Command description message", ' ', '0.0');

    vector<TCLAP::Arg*> input_file_args;
    TCLAP::ValueArg<string> chaco_input_file_flag(
        "c", "chaco", "CHACO-format input file name", false, "string",
        nullptr);
    input_file_args.push_back(&chaco_input_file_flag);

    TCLAP::ValueArg<string> ntl_input_file_flag(
        "n", "ntl", "NTL-format input file name", false, "string",
        nullptr);
    input_file_args.push_back(&ntl_input_file_flag);

    TCLAP::ValueArg<string> mps_input_file_flag(
        "m", "mps", "MPS-format input file name", false, "string",
        nullptr);
    input_file_args.push_back(&mps_input_file_flag);

    cmd.xorAdd(input_file_args);

    TCLAP::ValueArg<string> mps_output_file_flag(
        "o", "output_mps", "Output MPS file name", false, "string", cmd,
        nullptr);

    TCLAP::ValueArg<long> solver_timeout_flag(
        "t", "timeout_s", "Solver timeout in seconds", false, "long", cmd,
        nullptr);

    TCLAP::SwitchArg solve_switch(
        "s", "solve", "Solve LP model", cmd, false);

    cmd.parse(argc, argv);

    if (chaco_input_file_flag.isSet()) {
      use_chaco = true;
      input_filename = chaco_input_file_flag.getValue();
    } else if (ntl_input_file_flag.isSet()) {
      use_ntl = true;
      input_filename = ntl_input_file_flag.getValue();
    } else if (mps_input_file_flag.isSet()) {
      use_mps = true;
      input_filename = mps_input_file_flag.getValue();
    }

    if (mps_output_file_flag.isSet()) {
      write_mps = true;
      output_filename = mps_output_file_flag.getValue();
    }

    solve = solve_switch.getValue();

    if (solve && solver_timeout_flag.isSet()) {
      timeout_s = solver_timeout_flag.getValue();
    }

  } catch (TCLAP::ArgException &e) {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
  }

  LpSolveInterface interface;
  if (use_chaco) {
    interface.LoadFromChaco(input_filename);
  } else if (use_ntl) {
    interface.LoadFromNtl(input_filename);
  } else if (use_mps) {
    interface.LoadFromMps(input_filename);
  }

  if (solve) {
    interface.RunSolver(timeout_s);
  }

  if (write_mps) {
    interface.WriteToMps(output_filename);
  }

  return 0;
}



