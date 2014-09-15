/*
 * xilinx_functional_nodes.cpp
 *
 *  Created on: Sep 4, 2014
 *      Author: gregerso
 */

#include "xilinx_functional_nodes.h"

#include <cmath>

#include <exception>

#include "functional_edge.h"
#include "structural_netlist_lexer.h"

using namespace std;

namespace vivado {

void XilinxBufgNode::AddConnection(const NamedConnection& connection) {
  if (connection.first == "O") {
    named_output_connections.insert(connection);
  } else if (connection.first == "I") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

double XilinxBufgNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes) const {
  return ComputeEntropy(output_name, edges, nodes, 0, 0);
}

double XilinxBufgNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (bit_high != 0 && bit_low != 0) {
    cout << "Invalid bit range for LUT ComputeEntropy" << endl;
    throw std::exception();
  }
  return 1.0;
}

void XilinxCarry4Node::AddConnection(const NamedConnection& connection) {
  if (connection.first == "CO" ||
      connection.first == "O") {
    named_output_connections.insert(connection);
  } else if (connection.first == "CI" ||
             connection.first == "CYINIT" ||
             connection.first == "DI" ||
             connection.first == "S") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxDsp48e1Node::AddConnection(const NamedConnection& connection) {
  if (connection.first == "CARRYCASCOUT" ||
      connection.first == "MULTSIGNOUT" ||
      connection.first == "OVERFLOW" ||
      connection.first == "PATTERNBDETECT" ||
      connection.first == "PATTERNDETECT" ||
      connection.first == "UNDERFLOW" ||
      connection.first == "BCOUT" ||
      connection.first == "ACOUT" ||
      connection.first == "CARRYOUT" ||
      connection.first == "P" ||
      connection.first == "PCOUT") {
    named_output_connections.insert(connection);
  } else if (connection.first == "CARRYCASCIN" ||
             connection.first == "CARRYIN" ||
             connection.first == "CEA1" ||
             connection.first == "CEA2" ||
             connection.first == "CEAD" ||
             connection.first == "CEALUMODE" ||
             connection.first == "CEB1" ||
             connection.first == "CEB2" ||
             connection.first == "CEC" ||
             connection.first == "CECARRYIN" ||
             connection.first == "CECTRL" ||
             connection.first == "CED" ||
             connection.first == "CEINMODE" ||
             connection.first == "CEM" ||
             connection.first == "CEP" ||
             connection.first == "CLK" ||
             connection.first == "MULTSIGNIN" ||
             connection.first == "RSTA" ||
             connection.first == "RSTALLCARRYIN" ||
             connection.first == "RSTALUMODE" ||
             connection.first == "RSTB" ||
             connection.first == "RSTC" ||
             connection.first == "RSTCTRL" ||
             connection.first == "RSTD" ||
             connection.first == "RSTINMODE" ||
             connection.first == "RSTM" ||
             connection.first == "RSTP" ||
             connection.first == "B" ||
             connection.first == "BCIN" ||
             connection.first == "D" ||
             connection.first == "A" ||
             connection.first == "ACIN" ||
             connection.first == "CARRYINSEL" ||
             connection.first == "ALUMODE" ||
             connection.first == "C" ||
             connection.first == "PCIN" ||
             connection.first == "INMODE" ||
             connection.first == "OPMODE") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxFdceNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "Q") {
    named_output_connections.insert(connection);
  } else if (port_name == "C" || port_name == "CE" || port_name == "CLR" ||
             port_name == "D") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxFdpeNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "Q") {
    named_output_connections.insert(connection);
  } else if (port_name == "C" || port_name == "CE" || port_name == "PRE" ||
             port_name == "D") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxFdreNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "Q") {
    named_output_connections.insert(connection);
  } else if (port_name == "C" || port_name == "CE" || port_name == "R" ||
             port_name == "D") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxFdseNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "Q") {
    named_output_connections.insert(connection);
  } else if (port_name == "C" || port_name == "CE" || port_name == "S" ||
             port_name == "D") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxGndNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "G") {
    named_output_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxIbufNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "O") {
    named_output_connections.insert(connection);
  } else if (port_name == "I") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxLutNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name.empty()) {
    throw std::exception();
  }
  if (port_name[0] == 'O') {
    named_output_connections.insert(connection);
  } else if (port_name[0] == 'I') {
    named_input_connections.insert(connection);
  } else {
    named_unknown_connections.insert(connection);
  }
}

double XilinxLutNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes) const {
  return ComputeEntropy(output_name, edges, nodes, 0, 0);
}

double XilinxLutNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (bit_high != 0 && bit_low != 0) {
    cout << "Invalid bit range for LUT ComputeEntropy" << endl;
    throw std::exception();
  }
  if (output_name != "O" && output_name != "") {
    std::cout << "Unexpected output name: " << output_name << "\n";
    throw std::exception();
  }
  auto param_it = named_parameters.find("INIT");
  if (param_it == named_parameters.end()) {
    std::cout << "Couldn't find INIT parameter.\n";
    throw std::exception();
  }
  std::string init_str = param_it->second;
  StructuralNetlistLexer::ConsumeHexImmediate(init_str, nullptr);
  std::pair<int, uint64_t> init_val =
      StructuralNetlistLexer::VlogImmediateToUint64(init_str);
  int zeroes = 0;
  int ones = 0;
  for (int i = 0; i < init_val.first; ++i) {
    uint64_t bit_mask = 1 << i;
    if (bit_mask & init_val.second) {
      ++ones;
    } else {
      ++zeroes;
    }
  }
  double p_zero = zeroes / (double)(zeroes + ones);
  double p_one = 1.0 - p_zero;
  return -p_zero*log2(p_zero) + -p_one*log2(p_one);
}

double XilinxLutNode::ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      NodeMap* nodes) const {
  if (bit_pos != 0) {
    cout << "Invalid bit range for LUT ComputeProbabilityOne" << endl;
    throw std::exception();
  }
  if (output_name != "O" && output_name != "") {
    std::cout << "Unexpected output name: " << output_name << "\n";
    throw std::exception();
  }
  auto param_it = named_parameters.find("INIT");
  if (param_it == named_parameters.end()) {
    std::cout << "Couldn't find INIT parameter.\n";
    throw std::exception();
  }
  std::string init_str = param_it->second;
  StructuralNetlistLexer::ConsumeHexImmediate(init_str, nullptr);
  std::pair<int, uint64_t> init_val =
      StructuralNetlistLexer::VlogImmediateToUint64(init_str);
  vector<double> input_probability_ones;
  for (pair<string,string> input_connection : named_input_connections) {
    auto edge_it = edges->find(input_connection.second);
    if (edge_it == edges->end()) {
      throw exception();
    }
    // TODO: This is fragile. It relies on the map being sorted in ascending
    // order of inputs.
    FunctionalEdge* edge = edge_it->second;
    double d = edge->ProbabilityOne(bit_pos,edges,nodes);
    input_probability_ones.push_back(d);
  }
  vector<double> address_probabilities;
  for (int i = 0; i < init_val.first; ++i) {
    double probability_selected = 1.0;
    int mask = 1;
    for (double input_p_one : input_probability_ones) {
      if (i & mask) {
        probability_selected *= input_p_one;
      } else {
        probability_selected *= (1.0 - input_p_one);
      }
      mask = mask << 1;
    }
    address_probabilities.push_back(probability_selected);
  }
  double p_one = 0.0;
  for (int i = 0; i < init_val.first; ++i) {
    uint64_t bit_mask = 1 << i;
    if (bit_mask & init_val.second) {
      p_one += address_probabilities[i];
    }
  }
  return p_one;
}

void XilinxObufNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "O") {
    named_output_connections.insert(connection);
  } else if (port_name == "I") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxMuxf7Node::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "O") {
    named_output_connections.insert(connection);
  } else if (port_name == "I0" || port_name == "I1" || port_name == "S") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxMuxf8Node::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "O") {
    named_output_connections.insert(connection);
  } else if (port_name == "I0" || port_name == "I1" || port_name == "S") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxRamb18e1Node::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "DOADO" ||
      port_name == "DOBDO" ||
      port_name == "DOPADOP" ||
      port_name == "DOPBDOP") {
    named_output_connections.insert(connection);
  } else if (port_name == "CLKARDCLK" ||
             port_name == "CLKBWRCLK" ||
             port_name == "ENARDEN" ||
             port_name == "ENBWREN" ||
             port_name == "REGCEAREGCE" ||
             port_name == "REGCEB" ||
             port_name == "RSTRAMARSTRAM" ||
             port_name == "RSTRAMB" ||
             port_name == "RSTREGARSTREG" ||
             port_name == "RSTREGB" ||
             port_name == "ADDRARDADDR" ||
             port_name == "ADDRBWRADDR" ||
             port_name == "DIADI" ||
             port_name == "DIBDI" ||
             port_name == "DIPADIP" ||
             port_name == "DIPBDIP" ||
             port_name == "WEA" ||
             port_name == "WEBWE") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxRamb36e1Node::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "CASCADEOUTA" ||
      port_name == "CASCADEOUTB" ||
      port_name == "SBITERR" ||
      port_name == "DBITERR" ||
      port_name == "DOADO" ||
      port_name == "DOBDO" ||
      port_name == "DOPADOP" ||
      port_name == "DOPBDOP" ||
      port_name == "ECCPARITY" ||
      port_name == "RDADDRECC") {
    named_output_connections.insert(connection);
  } else if (port_name == "CLKARDCLK" ||
             port_name == "CLKBWRCLK" ||
             port_name == "ENARDEN" ||
             port_name == "ENBWREN" ||
             port_name == "REGCEAREGCE" ||
             port_name == "REGCEB" ||
             port_name == "RSTRAMARSTRAM" ||
             port_name == "RSTRAMB" ||
             port_name == "RSTREGARSTREG" ||
             port_name == "RSTREGB" ||
             port_name == "CASCADEINA" ||
             port_name == "CASCADEINB" ||
             port_name == "INJECTDBITERR" ||
             port_name == "INJECTSBITERR" ||
             port_name == "ADDRARDADDR" ||
             port_name == "ADDRBWRADDR" ||
             port_name == "DIADI" ||
             port_name == "DIBDI" ||
             port_name == "DIPADIP" ||
             port_name == "DIPBDIP" ||
             port_name == "WEA" ||
             port_name == "WEBWE") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxRam64mNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "DOA" ||
      port_name == "DOB" ||
      port_name == "DOC" ||
      port_name == "DOD") {
    named_output_connections.insert(connection);
  } else if (port_name == "DIA" ||
             port_name == "DIB" ||
             port_name == "DIC" ||
             port_name == "DID" ||
             port_name == "WCLK" ||
             port_name == "WE" ||
             port_name == "ADDRA" ||
             port_name == "ADDRB" ||
             port_name == "ADDRC" ||
             port_name == "ADDRD") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxSrl16eNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "Q") {
    named_output_connections.insert(connection);
  } else if (port_name == "A0" || port_name == "A1" || port_name == "A2" ||
             port_name == "A3" || port_name == "CE" || port_name == "CLK" ||
             port_name == "D") {
    named_input_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

void XilinxVccNode::AddConnection(const NamedConnection& connection) {
  const string& port_name = connection.first;
  if (port_name == "P") {
    named_output_connections.insert(connection);
  } else {
    cout << "Unexpected port name: " << connection.first << endl;
    throw std::exception();
  }
}

}  // namespace vivado



