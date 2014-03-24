#!/bin/bash

#for GRAPH_BASE in raygen
#for GRAPH_BASE in sha diffeq1 raygen blob boundtop_mod isolation fft128 jet_reconstruction rct mcml_mod add20 memplus_experimental cti vibrobox wing_experimental brack2 finan512_experimental fe_tooth fe_rotor 598a fe_ocean 144 wave m14b auto
for GRAPH_BASE in auto m14b wave 144 fe_ocean 598a fe_rotor fe_tooth finan512_experimental brack2 wing_experimental vibrobox cti memplus_experimental add20 mcml_mod rct jet_reconstruction isolation fft128 boundtop_mod blob diffeq1 raygen sha
do
  condor_submit ${KLFM_PATH}/scripts/${GRAPH_BASE}/condor/condor_all.scr
done
