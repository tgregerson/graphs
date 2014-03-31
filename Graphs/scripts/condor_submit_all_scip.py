import sys
import subprocess
import os.path

klfm_path = "/research/gregerso/dev/projects/fpga-hep-dif/trunk/tools/partitioning/klfm/"
script_path = klfm_path + "scripts/"

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

def PrintHelpAndQuit():
  print 'condor_submit_all_scip.py <run_time_in_hours>'
  sys.exit(2)

def submit_script(script):
  if not os.path.isfile(script):
    print "Error: could not find condor script " + script + "\n"
  else:
    command = [
        "/mnt/condor/bin/condor_submit",
        script
    ]
    for command_str in command:
      print command_str,
    print "\n"
    p = subprocess.call(command)

if len(sys.argv) != 2:
  PrintHelpAndQuit()
time_h = sys.argv[1]

for graph in graphs:
  # Generate condor submit descriptions
  script_none_fullname    = script_path + graph + "/condor/condor_submit_scip_none_"    + time_h + ".scr"
  script_initial_fullname = script_path + graph + "/condor/condor_submit_scip_initial_" + time_h + ".scr"
  script_final_fullname   = script_path + graph + "/condor/condor_submit_scip_final_"   + time_h + ".scr"

  submit_script(script_none_fullname)
  submit_script(script_initial_fullname)
  submit_script(script_final_fullname)
