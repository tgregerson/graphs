#!/bin/bash

POSTFIX=0
DATA_DIVISOR=8
ADDR_DIVISOR=12
for i in {0..4095}
do
  POSTFIX=$(($i / 32))
  DATA_MUL=$(($i % $DATA_DIVISOR))
  DATA_0=$(($DATA_MUL * 8))
  DATA_1=$(($DATA_0 + 1))
  DATA_2=$(($DATA_0 + 2))
  DATA_3=$(($DATA_0 + 3))
  DATA_4=$(($DATA_0 + 4))
  DATA_5=$(($DATA_0 + 5))
  DATA_6=$(($DATA_0 + 6))
  DATA_7=$(($DATA_0 + 7))
  ADDR_0=$(($i % $ADDR_DIVISOR))
  ADDR_1=$(($ADDR_0 + 1))
  ADDR_2=$(($ADDR_0 + 2))
  ADDR_3=$(($ADDR_0 + 3))
  ADDR_4=$(($ADDR_0 + 4))
  ADDR_5=$(($ADDR_0 + 5))
  ADDR_6=$(($ADDR_0 + 6))
  echo "module_begin"
  echo "single_port_ram4kb gen_mem_1kb_${i} \\trilist/tm3_sram_addr_reg[$ADDR_0]$POSTFIX \\trilist/tm3_sram_addr_reg[$ADDR_1]$POSTFIX \\trilist/tm3_sram_addr_reg[$ADDR_2]$POSTFIX \\trilist/tm3_sram_addr_reg[$ADDR_3]$POSTFIX \\trilist/tm3_sram_addr_reg[$ADDR_4]$POSTFIX \\trilist/tm3_sram_addr_reg[$ADDR_5]$POSTFIX \\trilist/tm3_sram_addr_reg[$ADDR_6]$POSTFIX tm3_sram_data_in[$DATA_0]$POSTFIX tm3_sram_data_in[$DATA_1]$POSTFIX tm3_sram_data_in[$DATA_2]$POSTFIX tm3_sram_data_in[$DATA_3]$POSTFIX tm3_sram_data_in[$DATA_4]$POSTFIX tm3_sram_data_in[$DATA_5]$POSTFIX tm3_sram_data_in[$DATA_6]$POSTFIX tm3_sram_data_in[$DATA_7]$POSTFIX tm3_sram_data_out[$DATA_0]$POSTFIX tm3_sram_data_out[$DATA_1]$POSTFIX tm3_sram_data_out[$DATA_2]$POSTFIX tm3_sram_data_out[$DATA_3]$POSTFIX tm3_sram_data_out[$DATA_4]$POSTFIX tm3_sram_data_out[$DATA_5]$POSTFIX tm3_sram_data_out[$DATA_6]$POSTFIX tm3_sram_data_out[$DATA_7]$POSTFIX"
  echo "module_end"
  echo ""
done

for i in {0..127}
do
  for j in {0..63}
  do
    echo "module_begin"
    echo "synthetic_buffer synthbufdi${i}_${j} tms3_sram_data_in[$j] tms3_sram_data_in[$j]$i"
    echo "module_end"
    echo ""
    echo "module_begin"
    echo "synthetic_buffer synthbufdo${i}_${j} tms3_sram_data_out[$j] tms3_sram_data_out[$j]$i"
    echo "module_end"
    echo ""
  done
  for j in {0..17}
  do
    echo "module_begin"
    echo "synthetic_buffer synthbufa${i}_${j} \\trilist/tms3_sram_addr_reg[$j] \\trilist/tms3_sram_addr_reg[$j]$i"
    echo "module_end"
    echo ""
  done
done
