#!/bin/bash
source graph_include.sh

CONFIG=proportional_adaptive_affinity_5
GRAPH_PATH=${KLFM_PATH}/graphs/${GRAPH}
CONFIG_PATH=${KLFM_PATH}/configs/${CONFIG}.xml
RESULT_PATH=${KLFM_PATH}/results/${GRAPH}_${CONFIG}.txt

${KLFM_PATH}/partition_main $GRAPH_PATH $CONFIG_PATH $RUNS_PER_GROUP $RESULT_PATH 
