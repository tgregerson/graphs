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

void XilinxBufgNode::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "I") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxBufgNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes) const {
  return ComputeEntropy(output_name, edges, wires, nodes, 0, 0);
}

double XilinxBufgNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes, int bit_high, int bit_low) const {
  if (bit_high != 0 && bit_low != 0) {
    cout << "Invalid bit range for LUT ComputeEntropy" << endl;
    throw std::exception();
  }
  return 1.0;
}

void XilinxCarry4Node::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "CO" ||
      connection.port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "CI" ||
             connection.port_name == "CYINIT" ||
             connection.port_name == "DI" ||
             connection.port_name == "S") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxDsp48e1Node::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "CARRYCASCOUT" ||
      connection.port_name == "MULTSIGNOUT" ||
      connection.port_name == "OVERFLOW" ||
      connection.port_name == "PATTERNBDETECT" ||
      connection.port_name == "PATTERNDETECT" ||
      connection.port_name == "UNDERFLOW" ||
      connection.port_name == "BCOUT" ||
      connection.port_name == "ACOUT" ||
      connection.port_name == "CARRYOUT" ||
      connection.port_name == "P" ||
      connection.port_name == "PCOUT") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "CARRYCASCIN" ||
             connection.port_name == "CARRYIN" ||
             connection.port_name == "CEA1" ||
             connection.port_name == "CEA2" ||
             connection.port_name == "CEAD" ||
             connection.port_name == "CEALUMODE" ||
             connection.port_name == "CEB1" ||
             connection.port_name == "CEB2" ||
             connection.port_name == "CEC" ||
             connection.port_name == "CECARRYIN" ||
             connection.port_name == "CECTRL" ||
             connection.port_name == "CED" ||
             connection.port_name == "CEINMODE" ||
             connection.port_name == "CEM" ||
             connection.port_name == "CEP" ||
             connection.port_name == "CLK" ||
             connection.port_name == "MULTSIGNIN" ||
             connection.port_name == "RSTA" ||
             connection.port_name == "RSTALLCARRYIN" ||
             connection.port_name == "RSTALUMODE" ||
             connection.port_name == "RSTB" ||
             connection.port_name == "RSTC" ||
             connection.port_name == "RSTCTRL" ||
             connection.port_name == "RSTD" ||
             connection.port_name == "RSTINMODE" ||
             connection.port_name == "RSTM" ||
             connection.port_name == "RSTP" ||
             connection.port_name == "B" ||
             connection.port_name == "BCIN" ||
             connection.port_name == "D" ||
             connection.port_name == "A" ||
             connection.port_name == "ACIN" ||
             connection.port_name == "CARRYINSEL" ||
             connection.port_name == "ALUMODE" ||
             connection.port_name == "C" ||
             connection.port_name == "PCIN" ||
             connection.port_name == "INMODE" ||
             connection.port_name == "OPMODE") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxFdNode::ComputeEntropy(
      const string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes) const {
  return ComputeEntropy(output_name, edges, wires, nodes, 0, 0);
}

double XilinxFdNode::ComputeEntropy(
      const string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes, int bit_high, int bit_low) const {
  if (bit_high != bit_low) {
    throw std::exception();
  }
  return ComputeShannonEntropy(output_name, bit_high, edges, wires, nodes);
}

double XilinxFdNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* edges, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name != "Q" && output_name != "") {
    std::cout << "Unexpected output name: " << output_name << "\n";
    throw std::exception();
  } else if (bit_pos != 0) {
    std::cout << "Unexpected output bit position: " << bit_pos << "\n";
    throw std::exception();
  } else {
     string input_edge_name =
         named_input_connections.at("D").connection_bit_names.front();
     FunctionalEdge* input_edge = wires->at(input_edge_name);
     return input_edge->ProbabilityOne(bit_pos, edges, wires, nodes);
  }
}

void XilinxFdceNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "Q") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "C" || port_name == "CE" || port_name == "CLR" ||
             port_name == "D") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxFdpeNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "Q") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "C" || port_name == "CE" || port_name == "PRE" ||
             port_name == "D") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxFdreNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "Q") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "C" || port_name == "CE" || port_name == "R" ||
             port_name == "D") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxFdseNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "Q") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "C" || port_name == "CE" || port_name == "S" ||
             port_name == "D") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxGndNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "G") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxIbufNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "I") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxLutNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name.empty()) {
    throw std::exception();
  }
  if (port_name[0] == 'O') {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name[0] == 'I') {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    named_unknown_connections.insert(make_pair(connection.port_name, connection));
  }
}

double XilinxLutNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes) const {
  return ComputeEntropy(output_name, edges, wires, nodes, 0, 0);
}

double XilinxLutNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, EdgeMap* wires,
      NodeMap* nodes, int bit_high, int bit_low) const {
  if (bit_high != bit_low) {
    throw std::exception();
  }
  return ComputeShannonEntropy(output_name, bit_high, edges, wires, nodes);
}

double XilinxLutNode::ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
      EdgeMap* wires, NodeMap* nodes) const {
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
  for (const auto& input_connection : named_input_connections) {
    const vector<string>& bit_names =
        input_connection.second.connection_bit_names;
    for (const string& bit_name : bit_names) {
      FunctionalEdge* wire = wires->at(bit_name);
      double d = wire->ProbabilityOne(bit_pos, edges, wires, nodes);
      input_probability_ones.push_back(d);
    }
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

void XilinxObufNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "I") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxMuxf7Node::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "I0" || port_name == "I1" || port_name == "S") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxMuxf8Node::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "I0" || port_name == "I1" || port_name == "S") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxRamb18e1Node::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "DOADO" ||
      port_name == "DOBDO" ||
      port_name == "DOPADOP" ||
      port_name == "DOPBDOP") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
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
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxRamb36e1Node::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
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
    named_output_connections.insert(make_pair(connection.port_name, connection));
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
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxRam64mNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "DOA" ||
      port_name == "DOB" ||
      port_name == "DOC" ||
      port_name == "DOD") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
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
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxSrl16eNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "Q") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "A0" || port_name == "A1" || port_name == "A2" ||
             port_name == "A3" || port_name == "CE" || port_name == "CLK" ||
             port_name == "D") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

void XilinxVccNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "P") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

}  // namespace vivado



