#!/bin/bash
INC_PATH=$(dirname $0)
source ${INC_PATH}/graph_include.sh

CONFIG=universal_resource_single_5
GRAPH_PATH=${KLFM_PATH}/graphs/${GRAPH}
CONFIG_PATH=${KLFM_PATH}/configs/${CONFIG}.xml
RESULT_PATH=${KLFM_PATH}/results/${GRAPH}_${CONFIG}.txt

${KLFM_PATH}/partition_main $GRAPH_PATH $CONFIG_PATH $RUNS_PER_GROUP $RESULT_PATH 
