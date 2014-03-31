import getopt
import os.path
import subprocess
import sys

klfm_path = "/research/gregerso/dev/projects/fpga-hep-dif/trunk/tools/partitioning/klfm/"
condor_result_path = klfm_path + "results/"
mps_path = klfm_path + "mps/"
solution_path = klfm_path + "sol/"
script_path = klfm_path + "scripts/"
binary_path = script_path + "condor_bin/"
binary = binary_path + "scip-3.1.0.linux.x86_64.gnu.opt.spx"

graphs = [
    "144",
    "598a",
    "auto",
    "blob",
    "boundtop_mod",
    "brack2",
    "cti",
    "diffeq1",
    "fe_ocean",
    "fe_rotor",
    "fe_tooth",
    "fft128",
    "finan512_experimental",
    "isolation",
    "jet_reconstruction",
    "m14b",
    "mcml_mod",
    "memplus_experimental",
    "vibrobox",
    "raygen",
    "rct",
    "sha",
    "wave",
    "wing_experimental",
]

memory_map = {
  "144"                   : 14000,
  "598a"                  : 14000,
  "auto"                  : 14000,
  "blob"                  : 4096,
  "boundtop_mod"          : 4096,
  "brack2"                : 8096,
  "cti"                   : 4096,
  "diffeq1"               : 3072,
  "fe_ocean"              : 10000,
  "fe_rotor"              : 14000,
  "fe_tooth"              : 12000,
  "fft128"                : 8096,
  "finan512_experimental" : 8096,
  "isolation"             : 8096,
  "jet_reconstruction"    : 10000,
  "m14b"                  : 14000,
  "mcml_mod"              : 14000,
  "memplus_experimental"  : 4096,
  "raygen"                : 8096,
  "rct"                   : 12000,
  "sha"                   : 2048,
  "vibrobox"              : 6000,
  "wave"                  : 14000,
  "wing_experimental"     : 6000,
}

time_map_h = {
  "144"                   : 12,
  "598a"                  : 12,
  "auto"                  : 12,
  "blob"                  : 12,
  "boundtop_mod"          : 12,
  "brack2"                : 12,
  "cti"                   : 12,
  "diffeq1"               : 12,
  "fe_ocean"              : 12,
  "fe_rotor"              : 12,
  "fe_tooth"              : 12,
  "fft128"                : 12,
  "finan512_experimental" : 12,
  "isolation"             : 12,
  "jet_reconstruction"    : 12,
  "m14b"                  : 12,
  "memplus_experimental"  : 12,
  "mcml_mod"              : 12,
  "raygen"                : 12,
  "rct"                   : 12,
  "sha"                   : 12,
  "vibrobox"              : 12,
  "wave"                  : 12,
  "wing_experimental"     : 12,
}

def PrintHelpAndQuit():
  print 'generate_condor_scripts.py <run_time_in_hours>'
  sys.exit(2)

if len(sys.argv) != 2:
  PrintHelpAndQuit()
time_h = sys.argv[1]

for graph in graphs:
  # Generate batch files.
  none_file_batch_fullname    = script_path + graph + "/condor/batch-none-" + time_h
  initial_file_batch_fullname = script_path + graph + "/condor/batch-initial-" + time_h
  final_file_batch_fullname   = script_path + graph + "/condor/batch-final-" + time_h

  none_file_batch    = file(none_file_batch_fullname, "w")
  initial_file_batch = file(initial_file_batch_fullname, "w")
  final_file_batch   = file(final_file_batch_fullname, "w")

  batch_common_prefix  = 'read ' + mps_path + graph + '.mps '
  none_file_batch.write(batch_common_prefix)
  initial_file_batch.write(batch_common_prefix)
  final_file_batch.write(batch_common_prefix)

  initial_file_batch.write('read ' + solution_path + graph + '-solution-initial.sol ')
  final_file_batch.write(  'read ' + solution_path + graph + '-solution-final.sol ')

  batch_common_midfix  = 'set limits time ' + str(3600 * int(time_h)) + ' opt write solution '
  none_file_batch.write(batch_common_midfix)
  initial_file_batch.write(batch_common_midfix)
  final_file_batch.write(batch_common_midfix)

  none_file_batch.write(   solution_path + graph + '-post-ilp-none-'    + time_h + '.sol quit\n')
  initial_file_batch.write(solution_path + graph + '-post-ilp-initial-' + time_h + '.sol quit\n')
  final_file_batch.write(  solution_path + graph + '-post-ilp-final-'   + time_h + '.sol quit\n')

  none_file_batch.close()
  initial_file_batch.close()
  final_file_batch.close()

  # Generate condor submit descriptions
  output_none_fullname    = script_path + graph + "/condor/condor_submit_scip_none_" + time_h + ".scr"
  output_initial_fullname = script_path + graph + "/condor/condor_submit_scip_initial_" + time_h + ".scr"
  output_final_fullname   = script_path + graph + "/condor/condor_submit_scip_final_" + time_h + ".scr"
  
  if (os.path.exists(output_none_fullname)):
    print "File " + output_none_fullname + " already exists. Overwriting."
  else:
    print "File " + output_none_fullname + " doesn't exist. Creating."
  if (os.path.exists(output_initial_fullname)):
    print "File " + output_initial_fullname + " already exists. Overwriting."
  else:
    print "File " + output_initial_fullname + " doesn't exist. Creating."
  if (os.path.exists(output_final_fullname)):
    print "File " + output_final_fullname + " already exists. Overwriting."
  else:
    print "File " + output_final_fullname + " doesn't exist. Creating."

  none_file    = file(output_none_fullname, "w")
  initial_file = file(output_initial_fullname, "w")
  final_file   = file(output_final_fullname, "w")

  preamble = (
      "Executable = " + binary + "\n",
      "initialdir = " + klfm_path + "\n",
      "Universe = vanilla\n",
      "Getenv = true\n",
      "notification = Always\n",
      "notify_user = tony.gregerson@gmail.com\n",
      "Rank = TARGET.Mips\n",
      "request_memory = " + str(memory_map[graph]) + "\n\n"
  )
  none_file.writelines(preamble)
  initial_file.writelines(preamble)
  final_file.writelines(preamble)

  arguments_common_prefix  = 'Arguments = -b '
  none_file.write(arguments_common_prefix)
  initial_file.write(arguments_common_prefix)
  final_file.write(arguments_common_prefix)

  none_file.write(none_file_batch_fullname + "\n")
  initial_file.write(initial_file_batch_fullname + "\n")
  final_file.write(final_file_batch_fullname + "\n")

  none_file.write(   "Output = " + condor_result_path + "out/" + graph + "-none-"    + time_h + ".out\n")
  initial_file.write("Output = " + condor_result_path + "out/" + graph + "-initial-" + time_h + ".out\n")
  final_file.write(  "Output = " + condor_result_path + "out/" + graph + "-final-"   + time_h + ".out\n")

  none_file.write(   "Log = " + condor_result_path + "log/" + graph + "-none-"    + time_h + ".log\n")
  initial_file.write("Log = " + condor_result_path + "log/" + graph + "-initial-" + time_h + ".log\n")
  final_file.write(  "Log = " + condor_result_path + "log/" + graph + "-final-"   + time_h + ".log\n")

  none_file.write(   "Error = " + condor_result_path + "err/" + graph + "-none-"    + time_h + ".err\n")
  initial_file.write("Error = " + condor_result_path + "err/" + graph + "-initial-" + time_h + ".err\n")
  final_file.write(  "Error = " + condor_result_path + "err/" + graph + "-final-"   + time_h + ".err\n")

  none_file.write("Queue\n")
  initial_file.write("Queue\n")
  final_file.write("Queue\n")

  none_file.close()
  initial_file.close()
  final_file.close()
