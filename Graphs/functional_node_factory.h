/*
 * functional_node_factory.h
 *
 *  Created on: Sep 4, 2014
 *      Author: gregerso
 */

#ifndef FUNCTIONAL_NODE_FACTORY_H_
#define FUNCTIONAL_NODE_FACTORY_H_

#include "functional_node.h"
#include "xilinx_functional_nodes.h"

class FunctionalNodeFactory {
 public:
  static FunctionalNode* CreateFunctionalNode(const std::string& str, double ver = 1.0) {
    // Replace with calls to constructors.
    FunctionalNode* node;
    if (str == "BUFG") {
      node = new vivado::XilinxBufgNode();
    } else if (str == "CARRY4") {
      node = new vivado::XilinxCarry4Node();
    } else if (str == "DSP48E1") {
      node = new vivado::XilinxDsp48e1Node();
    } else if (str == "FDCE") {
      node = new vivado::XilinxFdceNode();
    } else if (str == "FDPE") {
      node = new vivado::XilinxFdpeNode();
    } else if (str == "FDRE") {
      node = new vivado::XilinxFdreNode();
    } else if (str == "FDSE") {
      node = new vivado::XilinxFdseNode();
    } else if (str == "GND") {
      node = new vivado::XilinxGndNode();
    } else if (str == "IBUF") {
      node = new vivado::XilinxIbufNode();
    } else if (str == "LUT1") {
      node = new vivado::XilinxLut1Node();
    } else if (str == "LUT2") {
      node = new vivado::XilinxLut2Node();
    } else if (str == "LUT3") {
      node = new vivado::XilinxLut3Node();
    } else if (str == "LUT4") {
      node = new vivado::XilinxLut4Node();
    } else if (str == "LUT5") {
      node = new vivado::XilinxLut5Node();
    } else if (str == "LUT6") {
      node = new vivado::XilinxLut6Node();
    } else if (str == "OBUF") {
      node = new vivado::XilinxObufNode();
    } else if (str == "MUXF7") {
      node = new vivado::XilinxMuxf7Node();
    } else if (str == "MUXF8") {
      node = new vivado::XilinxMuxf8Node();
    } else if (str == "RAMB18E1") {
      node = new vivado::XilinxRamb18e1Node();
    } else if (str == "RAMB36E1") {
      node = new vivado::XilinxRamb36e1Node();
    } else if (str == "RAM64M") {
      node = new vivado::XilinxRam64mNode();
    } else if (str == "SRL16E") {
      node = new vivado::XilinxSrl16eNode();
    } else if (str == "VCC") {
      node = new vivado::XilinxVccNode();
    } else {
      throw std::exception();
    }
    node->type_name = str;
    return node;
  }
};

#endif /* FUNCTIONAL_NODE_FACTORY_H_ */
