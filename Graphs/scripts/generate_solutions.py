import sys
import subprocess
import os.path

bin_path = "/home/gregerso/git/graphs/Graphs/binaries/"
graph_path = "/research/gregerso/dev/projects/fpga-hep-dif/trunk/tools/partitioning/klfm/graphs/"
config_path = "/research/gregerso/dev/projects/fpga-hep-dif/trunk/tools/partitioning/klfm/configs/"
sol_path = "/research/gregerso/dev/projects/fpga-hep-dif/trunk/tools/partitioning/klfm/sol/"

chaco_graphs = [
    "144",
    "598a",
    "add20",
    "auto",
    "brack2",
    "cti",
    "fe_ocean",
    "fe_rotor",
    "fe_tooth",
    "finan512_experimental",
    "m14b",
    "memplus_experimental",
    "vibrobox",
    "wave",
    "wing_experimental"
]

ntl_graphs = [
    "blob",
    "boundtop_mod",
    "diffeq1",
    "fft128",
    "isolation",
    "jet_reconstruction",
    "mcml_mod",
    "raygen",
    "rct",
    "sha"
]

for graph in chaco_graphs:
  binary = bin_path + "partition_main"
  input_fullname = graph_path + graph + ".graph"
  config_fullname = config_path + "proportional_adaptive_affinity_1.xml"

  initial_output_basename = sol_path + graph + "-solution-initial"
  final_output_basename = sol_path + graph + "-solution-final"

  initial_output_mst = final_output_basename + ".mst"
  initial_output_sol = final_output_basename + ".sol"
  final_output_mst = final_output_basename + ".mst"
  final_output_sol = final_output_basename + ".sol"

  if not os.path.isfile(input_fullname):
    print "Input file: " + input_fullname + " not found. Skipping."
    continue
  if not os.path.isfile(config_fullname):
    print "Config file: " + config_fullname + " not found. Skipping."
    continue
  if (os.path.isfile(initial_output_mst) and os.path.isfile(initial_output_sol) and
      os.path.isfile(final_output_mst) and os.path.isfile(final_output_sol)):
    print "Skipping " + graph + "; output solutions already exist."
    continue
  else:
    command = [
        binary,
        "--chaco " + input_fullname,
        "--config " + config_fullname,
        "--export-initial-sol " + initial_output_basename,
        "--export-final-sol " + final_output_basename,
        "--sol-scip-format",
        "--sol-gurobi-format"
    ]
    print "Processing " + input_fullname
    for command_str in command:
      print command_str,
    p = subprocess.call(command)

for graph in ntl_graphs:
  binary = bin_path + "partition_main"
  input_fullname = graph_path + graph + ".ntl"
  config_fullname = config_path + "proportional_adaptive_affinity_1.xml"

  initial_output_basename = sol_path + graph + "-solution-initial"
  final_output_basename = sol_path + graph + "-solution-final"

  initial_output_mst = final_output_basename + ".mst"
  initial_output_sol = final_output_basename + ".sol"
  final_output_mst = final_output_basename + ".mst"
  final_output_sol = final_output_basename + ".sol"

  if not os.path.isfile(input_fullname):
    print "Input file: " + input_fullname + " not found. Skipping."
    continue
  if not os.path.isfile(config_fullname):
    print "Config file: " + config_fullname + " not found. Skipping."
    continue
  if (os.path.isfile(initial_output_mst) and os.path.isfile(initial_output_sol) and
      os.path.isfile(final_output_mst) and os.path.isfile(final_output_sol)):
    print "Skipping " + graph + "; output solutions already exist."
    continue
  else:
    command = [
        binary,
        "--ntl " + input_fullname,
        "--config " + config_fullname,
        "--export-initial-sol " + initial_output_basename,
        "--export-final-sol " + final_output_basename,
        "--sol-scip-format",
        "--sol-gurobi-format"
    ]
    print "Processing " + input_fullname
    for command_str in command:
      print command_str,
    p = subprocess.call(command)
