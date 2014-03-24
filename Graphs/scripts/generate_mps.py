import sys
import subprocess
import os.path

bin_path = "/home/gregerso/git/graphs/Graphs/binaries/"
graph_path = "/research/gregerso/dev/projects/fpga-hep-dif/trunk/tools/partitioning/klfm/graphs/"

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
    "blob_converted",
    "boundtop_mod_converted",
    "diffeq1_converted",
    "fft128_converted",
    "isolation_converted",
    "jet_reconstruction_converted",
    "mcml_mod_converted",
    "raygen_converted",
    "rct_converted",
    "sha_converted"
]

for graph in chaco_graphs:
  binary = bin_path + "lp_solve_interface"
  input_fullname = graph_path + graph + ".graph"
  output_fullname = graph_path + graph + ".mps"
  if not os.path.isfile(input_fullname):
    print "Input file: " + input_fullname + " not found. Skipping."
    continue
  if os.path.isfile(output_fullname):
    print "Skipping " + graph + "; " + output_fullname + " already exists."
  else:
    command = [
        binary,
        "--chaco " + input_fullname,
        "--max_imbalance 0.01",
        "--write_mps " + output_fullname
    ]
    print "Processing " + input_fullname
    for command_str in command:
      print command_str,
    p = subprocess.Popen(command, stdout=subprocess.PIPE)
    while True:
      out = p.stdout.read(1)
      if out == '' and p.poll() != None:
        break
      if out != '':
        sys.stdout.write(out)
        sys.stdout.flush()

for graph in ntl_graphs:
  binary = bin_path + "lp_solve_interface"
  input_fullname = graph_path + graph + ".ntl"
  output_fullname = graph_path + graph + ".mps"
  if not os.path.isfile(input_fullname):
    print "Input file: " + input_fullname + " not found. Skipping.\n"
    continue
  if os.path.isfile(output_fullname):
    print "Skipping " + graph + "; " + output_fullname + " already exists.\n"
  else:
    command = [
        binary,
        "--ntl " + input_fullname,
        "--max_imbalance 0.01",
        "--write_mps " + output_fullname
    ]
    print "Processing " + input_fullname + "\n"
    for command_str in command:
      print command_str,
    p = subprocess.Popen(command, stdout=subprocess.PIPE)
    while True:
      out = p.stdout.read(1)
      if out == '' and p.poll() != None:
        break
      if out != '':
        sys.stdout.write(out)
        sys.stdout.flush()
