#include "lp_solve_interface.h"

#include <cstdlib>

#include "tclap/CmdLine.h"

using namespace std;

void print_usage_and_exit() {
  cout << "Usage: lp_solve_interface REQUIRED_ARGS [OPTIONS*]" << endl
       << endl
       << "REQUIRED_ARGS: " << endl
       << "(--chaco chaco_graph_input_file] | "
       << "--ntl ntl_input_file | "
       << "--mps mps_input_file) " << endl
       << endl
       << "OPTIONS:" << endl
       << "--imbalance max_imbalance_fraction" << endl
       << "--solve [SOLVE_OPTIONS*]" << endl
       << "--write_lp mps_output_file" << endl
       << "--write_mps mps_output_file" << endl
       << endl
       << "SOLVE_OPTIONS:" << endl
       << "--timeout timeout_s "
       << endl;
  exit(1);
}

int main(int argc, char *argv[]) {
  bool solve = false;
  bool use_chaco = false;
  bool use_ntl = false;
  bool use_mps = false;
  bool write_lp = false;
  bool write_mps = false;
  long timeout_s = 0;
  double max_imbalance_fraction = 0.01;

  string input_filename;
  string output_mps_filename;
  string output_lp_filename;

  // Parse command-line arguments
  try {
    TCLAP::CmdLine cmd("Command description message", ' ', "0.0");

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

    TCLAP::ValueArg<string> lp_output_file_flag(
        "l", "write_lp", "Output LP file name", false, "", "string", cmd,
        nullptr);

    TCLAP::ValueArg<string> mps_output_file_flag(
        "o", "write_mps", "Output MPS file name", false, "", "string", cmd,
        nullptr);

    TCLAP::ValueArg<long> solver_timeout_flag(
        "t", "timeout_s", "Solver timeout in seconds", false, 0, "long", cmd,
        nullptr);

    TCLAP::ValueArg<double> max_imbalance_fraction_flag(
        "i", "max_imbalance", "Maximum resource imbalance fraction", false,
        0.01, "double", cmd, nullptr);

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

    if (lp_output_file_flag.isSet()) {
      write_lp = true;
      output_lp_filename = lp_output_file_flag.getValue();
    }

    if (mps_output_file_flag.isSet()) {
      write_mps = true;
      output_mps_filename = mps_output_file_flag.getValue();
    }

    if (max_imbalance_fraction_flag.isSet()) {
      max_imbalance_fraction = max_imbalance_fraction_flag.getValue();
    }

    solve = solve_switch.getValue();

    if (solve && solver_timeout_flag.isSet()) {
      timeout_s = solver_timeout_flag.getValue();
    }

  } catch (TCLAP::ArgException &e) {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
  }

  LpSolveInterface interface(max_imbalance_fraction);
  if (use_chaco) {
    interface.LoadFromChaco(input_filename);
  } else if (use_ntl) {
    interface.LoadFromNtl(input_filename);
  } else if (use_mps) {
    interface.LoadFromMps(input_filename);
  }

  if (write_mps) {
    interface.WriteToMps(output_mps_filename);
  }

  if (write_lp) {
    interface.WriteToLp(output_lp_filename);
  }

  if (solve) {
    interface.RunSolver(timeout_s);
  }

  return 0;
}



