import sys
import subprocess
import os.path

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

time_map_s = {
  "144"                   : (12 * 3600),
  "598a"                  : (12 * 3600),
  "auto"                  : (12 * 3600),
  "blob"                  : (12 * 3600),
  "boundtop_mod"          : (12 * 3600),
  "brack2"                : (12 * 3600),
  "cti"                   : (12 * 3600),
  "diffeq1"               : (12 * 3600),
  "fe_ocean"              : (12 * 3600),
  "fe_rotor"              : (12 * 3600),
  "fe_tooth"              : (12 * 3600),
  "fft128"                : (12 * 3600),
  "finan512_experimental" : (12 * 3600),
  "isolation"             : (12 * 3600),
  "jet_reconstruction"    : (12 * 3600),
  "m14b"                  : (12 * 3600),
  "memplus_experimental"  : (12 * 3600),
  "mcml_mod"              : (12 * 3600),
  "raygen"                : (12 * 3600),
  "rct"                   : (12 * 3600),
  "sha"                   : (12 * 3600),
  "vibrobox"              : (12 * 3600),
  "wave"                  : (12 * 3600),
  "wing_experimental"     : (12 * 3600),
}

for graph in graphs:
  # Generate batch files.
  none_file_batch_fullname = script_path + graph + "/condor/batch-none"
  initial_file_batch_fullname = script_path + graph + "/condor/batch-initial"
  final_file_batch_fullname = script_path + graph + "/condor/batch-final"

  none_file_batch = file(none_file_batch_fullname, "w")
  initial_file_batch = file(initial_file_batch_fullname, "w")
  final_file_batch = file(final_file_batch_fullname, "w")

  batch_common_prefix  = 'read ' + mps_path + graph + '.mps '
  none_file_batch.write(batch_common_prefix)
  initial_file_batch.write(batch_common_prefix)
  final_file_batch.write(batch_common_prefix)

  initial_file_batch.write('read ' + solution_path + graph + '-solution-initial.sol ')
  final_file_batch.write(  'read ' + solution_path + graph + '-solution-final.sol ')

  batch_common_midfix  = 'set limits time ' + str(time_map_s[graph]) + ' opt write solution '
  none_file_batch.write(batch_common_midfix)
  initial_file_batch.write(batch_common_midfix)
  final_file_batch.write(batch_common_midfix)

  none_file_batch.write(   solution_path + graph + '-post-ilp-none.sol quit\n')
  initial_file_batch.write(solution_path + graph + '-post-ilp-initial.sol quit\n')
  final_file_batch.write(  solution_path + graph + '-post-ilp-final.sol quit\n')

  none_file_batch.close()
  initial_file_batch.close()
  final_file_batch.close()

  # Generate condor submit descriptions
  output_none_fullname    = script_path + graph + "/condor/condor_submit_scip_none.scr"
  output_initial_fullname = script_path + graph + "/condor/condor_submit_scip_initial.scr"
  output_final_fullname   = script_path + graph + "/condor/condor_submit_scip_final.scr"
  
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

  none_file = file(output_none_fullname, "w")
  initial_file = file(output_initial_fullname, "w")
  final_file = file(output_final_fullname, "w")

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

  none_file.write(   "Output = " + condor_result_path + "out/" + graph + "-none.out\n")
  initial_file.write("Output = " + condor_result_path + "out/" + graph + "-initial.out\n")
  final_file.write(  "Output = " + condor_result_path + "out/" + graph + "-final.out\n")

  none_file.write(   "Log = " + condor_result_path + "log/" + graph + "-none.log\n")
  initial_file.write("Log = " + condor_result_path + "log/" + graph + "-initial.log\n")
  final_file.write(  "Log = " + condor_result_path + "log/" + graph + "-final.log\n")

  none_file.write(   "Error = " + condor_result_path + "err/" + graph + "-none.err\n")
  initial_file.write("Error = " + condor_result_path + "err/" + graph + "-initial.err\n")
  final_file.write(  "Error = " + condor_result_path + "err/" + graph + "-final.err\n")

  none_file.write("Queue\n")
  initial_file.write("Queue\n")
  final_file.write("Queue\n")

  none_file.close()
  initial_file.close()
  final_file.close()
