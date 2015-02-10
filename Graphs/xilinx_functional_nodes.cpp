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

void EpimBlackboxNode::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "egamma") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "clk" ||
             connection.port_name == "we" ||
             connection.port_name == "d_in" ||
             connection.port_name == "ecal_sum" ||
             connection.port_name == "hcal_sum" ||
             connection.port_name == "wr_addr") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double EpimBlackboxNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "egamma") {
    return ComputeEntropy(output_name, wires, nodes, 0, 0);
  } else {
    cout << "Unrecognized output name: " << output_name << endl;
    throw std::exception();
  }
}

double EpimBlackboxNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "egamma") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    cout << "Unrecognized output name: " << output_name << endl;
    throw std::exception();
  }
}

double EpimBlackboxNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  return 0.5;
}

void XilinxBufNode::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "I") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxBufNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxBufNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
}

double XilinxBufNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name == "O" && bit_pos == 0) {
    string wire_name =
        named_input_connections.at("I").connection_bit_names.at(0);
    return wires->at(wire_name)->ProbabilityOne(wires, nodes);
  } else {
    throw std::exception();
  }
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

double XilinxCarry4Node::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "CO" || output_name == "O") {
    return ComputeShannonEntropy(output_name, wires, nodes, 3, 0);
  } else {
    cout << "Unexpected output name: " << output_name << endl;
    throw std::exception();
  }
}

double XilinxCarry4Node::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
    return ComputeShannonEntropy(
        output_name, wires, nodes, bit_high, bit_low);
}

double XilinxCarry4Node::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  const vector<string>& d_in =
      named_input_connections.at("DI").connection_bit_names;
  vector<double> d_in_p1;
  for (const string& s : d_in) {
    d_in_p1.push_back(wires->at(s)->ProbabilityOne(wires, nodes));
  }

  const vector<string>& s_in =
      named_input_connections.at("S").connection_bit_names;
  vector<double> s_in_p1;
  for (const string& s : s_in) {
    s_in_p1.push_back(wires->at(s)->ProbabilityOne(wires, nodes));
  }

  const string& ci_in =
      named_input_connections.at("CI").connection_bit_names.at(0);
  const string& cyinit_in =
      named_input_connections.at("CYINIT").connection_bit_names.at(0);

  // If CYINIT is non-zero, it is used as the carry-in, otherwise CI is.
  // It is considered invalid for both of them to be non-zero.
  double cyinit_p1 = wires->at(cyinit_in)->ProbabilityOne(wires, nodes);
  double ci_p1 = wires->at(ci_in)->ProbabilityOne(wires, nodes);
  if (cyinit_p1 > 0.0 && ci_p1 > 0.0) {
    throw std::exception();
  }

  vector<double> carry_p1(5);
  carry_p1[0] = cyinit_p1 > 0.0 ? cyinit_p1 : ci_p1;
  for (size_t i = 0; i < carry_p1.size() - 1; ++i) {
    carry_p1[i + 1] = (1.0 - s_in_p1.at(i)) * d_in_p1.at(i) +
                      s_in_p1.at(i) * carry_p1[i];
  }

  vector<double> o_out_p1(4);
  for (size_t i = 0; i < o_out_p1.size(); ++i) {
    o_out_p1[i] = s_in_p1.at(0) * (1.0 - carry_p1[0]) +
                  (1.0 - s_in_p1.at(0)) * carry_p1[0];
  }

  if (output_name == "CO") {
    return carry_p1.at(bit_pos + 1);
  } else if (output_name == "O") {
    return o_out_p1.at(bit_pos);
  } else {
    cout << "Unexpected output name: " << output_name << endl;
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

double XilinxDsp48e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "CARRYCASCOUT" ||
      output_name == "MULTSIGNOUT" ||
      output_name == "OVERFLOW" ||
      output_name == "PATTERNBDETECT" ||
      output_name == "PATTERNDETECT" ||
      output_name == "UNDERFLOW") {
    return ComputeEntropy(output_name, wires, nodes, 0, 0);
  } else if (output_name == "BCOUT") {
    return ComputeEntropy(output_name, wires, nodes, 17, 0);
  } else if (output_name == "ACOUT") {
    return ComputeEntropy(output_name, wires, nodes, 29, 0);
  } else if (output_name == "CARRYOUT") {
    return ComputeEntropy(output_name, wires, nodes, 3, 0);
  } else if (output_name == "P" ||
             output_name == "PCOUT") {
    return ComputeEntropy(output_name, wires, nodes, 47, 0);
  } else {
    throw std::exception();
  }
}

double XilinxDsp48e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "CARRYCASCOUT" ||
      output_name == "MULTSIGNOUT" ||
      output_name == "OVERFLOW" ||
      output_name == "PATTERNBDETECT" ||
      output_name == "PATTERNDETECT" ||
      output_name == "UNDERFLOW" ||
      output_name == "BCOUT" ||
      output_name == "ACOUT" ||
      output_name == "CARRYOUT" ||
      output_name == "P" ||
      output_name == "PCOUT") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxDsp48e1Node::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name == "CARRYCASCOUT" ||
      output_name == "MULTSIGNOUT" ||
      output_name == "OVERFLOW" ||
      output_name == "PATTERNBDETECT" ||
      output_name == "PATTERNDETECT" ||
      output_name == "UNDERFLOW" ||
      output_name == "BCOUT" ||
      output_name == "ACOUT" ||
      output_name == "CARRYOUT" ||
      output_name == "P" ||
      output_name == "PCOUT") {
    // TODO: DSP48E has very complex functions. For now, punt on computing its
    // bit probabilities and assume worst-case (0.5).
    return 0.5;
  } else {
    throw std::exception();
  }
}

double XilinxFdNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxFdNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (bit_high != bit_low) {
    throw std::exception();
  }
  return ComputeShannonEntropy(output_name, bit_high, wires, nodes);
}

double XilinxFdNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name != "Q" && output_name != "") {
    std::cout << "Unexpected output name: " << output_name << "\n";
    throw std::exception();
  } else {
     string input_edge_name =
         named_input_connections.at("D").connection_bit_names.front();
     FunctionalEdge* input_edge = wires->at(input_edge_name);
     return input_edge->ProbabilityOne(wires, nodes);
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

void XilinxFifo18e1Node::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "ALMOSTEMPTY" ||
      connection.port_name == "ALMOSTFULL" ||
      connection.port_name == "DO" ||
      connection.port_name == "DOP" ||
      connection.port_name == "EMPTY" ||
      connection.port_name == "FULL" ||
      connection.port_name == "WRCOUNT" ||
      connection.port_name == "RDCOUNT" ||
      connection.port_name == "WRERR" ||
      connection.port_name == "RDERR") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "DI" ||
             connection.port_name == "DIP" ||
             connection.port_name == "RDEN" ||
             connection.port_name == "REGCE" ||
             connection.port_name == "RST" ||
             connection.port_name == "RSTREG" ||
             connection.port_name == "WRCLK" ||
             connection.port_name == "RDCLK" ||
             connection.port_name == "WREN") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxFifo18e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "ALMOSTEMPTY" ||
      output_name == "ALMOSTFULL" ||
      output_name == "EMPTY" ||
      output_name == "FULL" ||
      output_name == "WRERR" ||
      output_name == "RDERR") {
    return ComputeEntropy(output_name, wires, nodes, 0, 0);
  } else if (output_name == "DO") {
    unsigned long bit_high;
    if (named_parameters.find("DATA_WIDTH") != named_parameters.end()) {
      const string& val_str = named_parameters.at("DATA_WIDTH");
      if (!StructuralNetlistLexer::ConsumeUnbasedImmediate(
          val_str, nullptr).empty()) {
        throw std::exception();
      }
      bit_high = strtoul(val_str.c_str(), nullptr, 10);
      if (bit_high == 0) {
        throw std::exception();
      }
    } else {
      bit_high = 31;
    }
    return ComputeEntropy(output_name, wires, nodes, bit_high, 0);
  } else if (output_name == "WRCOUNT" ||
             output_name == "RDCOUNT") {
    return ComputeEntropy(output_name, wires, nodes, 11, 0);
  } else if (output_name == "DOP") {
    return ComputeEntropy(output_name, wires, nodes, 3, 0);
  } else {
    throw std::exception();
  }
}

double XilinxFifo18e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "ALMOSTEMPTY" ||
      output_name == "ALMOSTFULL" ||
      output_name == "EMPTY" ||
      output_name == "FULL" ||
      output_name == "WRERR" ||
      output_name == "RDERR" ||
      output_name == "DO" ||
      output_name == "WRCOUNT" ||
      output_name == "RDCOUNT" ||
      output_name == "DOP") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxFifo18e1Node::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name == "ALMOSTEMPTY" ||
      output_name == "ALMOSTFULL" ||
      output_name == "EMPTY" ||
      output_name == "FULL" ||
      output_name == "WRERR" ||
      output_name == "RDERR") {
    return 0.5;
  } else if (output_name == "DO") {
    const ConnectionDescriptor& desc = named_input_connections.at("DI");
    return wires->at(
        desc.connection_bit_names.at(bit_pos))->ProbabilityOne(wires, nodes);
  } else if (output_name == "WRCOUNT" ||
             output_name == "RDCOUNT") {
    return 0.5;
  } else if (output_name == "DOP") {
    const ConnectionDescriptor& desc = named_input_connections.at("DIP");
    return wires->at(
        desc.connection_bit_names.at(bit_pos))->ProbabilityOne(wires, nodes);
  } else {
    throw std::exception();
  }
}

void XilinxFifo36e1Node::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "ALMOSTEMPTY" ||
      connection.port_name == "ALMOSTFULL" ||
      connection.port_name == "DO" ||
      connection.port_name == "DOP" ||
      connection.port_name == "DBITERR" ||
      connection.port_name == "SBITERR" ||
      connection.port_name == "ECCPARITY" ||
      connection.port_name == "EMPTY" ||
      connection.port_name == "FULL" ||
      connection.port_name == "WRCOUNT" ||
      connection.port_name == "RDCOUNT" ||
      connection.port_name == "WRERR" ||
      connection.port_name == "RDERR") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "DI" ||
             connection.port_name == "DIP" ||
             connection.port_name == "RDEN" ||
             connection.port_name == "REGCE" ||
             connection.port_name == "RST" ||
             connection.port_name == "RSTREG" ||
             connection.port_name == "WRCLK" ||
             connection.port_name == "RDCLK" ||
             connection.port_name == "WREN" ||
             connection.port_name == "INJECTDBITERR" ||
             connection.port_name == "INJECTSBITERR") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxFifo36e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "ALMOSTEMPTY" ||
      output_name == "ALMOSTFULL" ||
      output_name == "EMPTY" ||
      output_name == "DBITERR" ||
      output_name == "SBITERR" ||
      output_name == "FULL" ||
      output_name == "WRERR" ||
      output_name == "RDERR") {
    return ComputeEntropy(output_name, wires, nodes, 0, 0);
  } else if (output_name == "DO") {
    unsigned long bit_high;
    if (named_parameters.find("DATA_WIDTH") != named_parameters.end()) {
      const string& val_str = named_parameters.at("DATA_WIDTH");
      if (!StructuralNetlistLexer::ConsumeUnbasedImmediate(
          val_str, nullptr).empty()) {
        throw std::exception();
      }
      bit_high = strtoul(val_str.c_str(), nullptr, 10);
      if (bit_high == 0) {
        throw std::exception();
      }
    } else {
      bit_high = 63;
    }
    return ComputeEntropy(output_name, wires, nodes, bit_high, 0);
  } else if (output_name == "WRCOUNT" ||
             output_name == "RDCOUNT") {
    return ComputeEntropy(output_name, wires, nodes, 12, 0);
  } else if (output_name == "DOP" ||
             output_name == "ECCPARITY") {
    return ComputeEntropy(output_name, wires, nodes, 7, 0);
  } else {
    throw std::exception();
  }
}

double XilinxFifo36e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "ALMOSTEMPTY" ||
      output_name == "ALMOSTFULL" ||
      output_name == "EMPTY" ||
      output_name == "ECCPARITY" ||
      output_name == "DBITERR" ||
      output_name == "SBITERR" ||
      output_name == "FULL" ||
      output_name == "WRERR" ||
      output_name == "RDERR" ||
      output_name == "DO" ||
      output_name == "WRCOUNT" ||
      output_name == "RDCOUNT" ||
      output_name == "DOP") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxFifo36e1Node::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name == "ALMOSTEMPTY" ||
      output_name == "ALMOSTFULL" ||
      output_name == "EMPTY" ||
      output_name == "ECCPARITY" ||
      output_name == "DBITERR" ||
      output_name == "SBITERR" ||
      output_name == "FULL" ||
      output_name == "WRERR" ||
      output_name == "RDERR") {
    return 0.5;
  } else if (output_name == "DO") {
    const ConnectionDescriptor& desc = named_input_connections.at("DI");
    return wires->at(
        desc.connection_bit_names.at(bit_pos))->ProbabilityOne(wires, nodes);
  } else if (output_name == "WRCOUNT" ||
             output_name == "RDCOUNT") {
    return 0.5;
  } else if (output_name == "DOP") {
    const ConnectionDescriptor& desc = named_input_connections.at("DIP");
    return wires->at(
        desc.connection_bit_names.at(bit_pos))->ProbabilityOne(wires, nodes);
  } else {
    throw std::exception();
  }
}

double XilinxGndNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return 0.0;
}

double XilinxGndNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  return 0.0;
}

double XilinxGndNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  return 0.0;
}

void XilinxLdceNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "Q") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "G" || port_name == "GE" || port_name == "CLR" ||
             port_name == "D") {
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
    throw std::exception();
  }
}
double XilinxLutNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxLutNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (bit_high != bit_low) {
    throw std::exception();
  }
  return ComputeShannonEntropy(output_name, bit_high, wires, nodes);
}

double XilinxLutNode::ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
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

  vector<double> addr_wire_p1s;
  if (named_input_connections.size() < 1 ||
      named_input_connections.begin()->first != "I0") {
    // Confirm that input connections are in ascending order; should be
    // guaranteed by use of map data structure.
    throw std::exception();
  }
  for (const auto& input_connection : named_input_connections) {
    const vector<string>& bit_names =
        input_connection.second.connection_bit_names;
    if (bit_names.size() != 1) {
      throw std::exception();
    } else {
      addr_wire_p1s.push_back(
          wires->at(bit_names.at(0))->ProbabilityOne(wires, nodes));
    }
  }

  size_t num_addr_bits = addr_wire_p1s.size();
  size_t num_addresses = 1 << num_addr_bits;
  double p1 = 0.0;

  for (size_t addr = 0; addr < num_addresses; ++addr) {
    uint64_t lut_mask = 1ULL << addr;
    if (lut_mask & init_val.second) {
      // We have a one entry in the LUT. Compute the probability that this
      // address is selected.
      double addr_p1 = 1.0;
      for (size_t addr_bit = 0; addr_bit < num_addr_bits; ++addr_bit) {
        int addr_mask = 1 << addr_bit;
        double addr_bit_p1 = addr_wire_p1s.at(addr_bit);
        if (addr & addr_mask) {
          addr_p1 *= addr_bit_p1;
        } else {
          addr_p1 *= (1.0 - addr_bit_p1);
        }
      }
      p1 += addr_p1;
    }
  }
  return p1;
}

void XilinxMmcme2_AdvNode::AddConnection(const ConnectionDescriptor& connection) {
  if (connection.port_name == "CLKOUT0" ||
      connection.port_name == "CLKOUT0B" ||
      connection.port_name == "CLKOUT1" ||
      connection.port_name == "CLKOUT1B" ||
      connection.port_name == "CLKOUT2" ||
      connection.port_name == "CLKOUT2B" ||
      connection.port_name == "CLKOUT3" ||
      connection.port_name == "CLKOUT3B" ||
      connection.port_name == "CLKOUT4" ||
      connection.port_name == "CLKOUT5" ||
      connection.port_name == "CLKOUT6" ||
      connection.port_name == "CLKFBOUT" ||
      connection.port_name == "CLKFBOUTB" ||
      connection.port_name == "LOCKED" ||
      connection.port_name == "DO" ||
      connection.port_name == "DRDY" ||
      connection.port_name == "PSDONE" ||
      connection.port_name == "CLKINSTOPPED" ||
      connection.port_name == "CLKFBSTOPPED") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (connection.port_name == "CLKIN1" ||
             connection.port_name == "CLKIN2" ||
             connection.port_name == "CLKFBIN" ||
             connection.port_name == "RST" ||
             connection.port_name == "PWRDWN" ||
             connection.port_name == "CLKINSEL" ||
             connection.port_name == "DADDR" ||
             connection.port_name == "DI" ||
             connection.port_name == "DWE" ||
             connection.port_name == "DEN" ||
             connection.port_name == "DCLK" ||
             connection.port_name == "PSINCDEC" ||
             connection.port_name == "PSEN" ||
             connection.port_name == "PSCLK") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxMmcme2_AdvNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "CLKOUT0" ||
      output_name == "CLKOUT0B" ||
      output_name == "CLKOUT1" ||
      output_name == "CLKOUT1B" ||
      output_name == "CLKOUT2" ||
      output_name == "CLKOUT2B" ||
      output_name == "CLKOUT3" ||
      output_name == "CLKOUT3B" ||
      output_name == "CLKOUT4" ||
      output_name == "CLKOUT5" ||
      output_name == "CLKOUT6" ||
      output_name == "CLKFBOUT" ||
      output_name == "CLKFBOUTB" ||
      output_name == "LOCKED" ||
      output_name == "DRDY" ||
      output_name == "PSDONE" ||
      output_name == "CLKINSTOPPED" ||
      output_name == "CLKFBSTOPPED") {
    return ComputeEntropy(output_name, wires, nodes, 0, 0);
  } else if (output_name == "DO") {
    return ComputeEntropy(output_name, wires, nodes, 15, 0);
  } else {
    throw std::exception();
  }
}

double XilinxMmcme2_AdvNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "CLKOUT0" ||
      output_name == "CLKOUT0B" ||
      output_name == "CLKOUT1" ||
      output_name == "CLKOUT1B" ||
      output_name == "CLKOUT2" ||
      output_name == "CLKOUT2B" ||
      output_name == "CLKOUT3" ||
      output_name == "CLKOUT3B" ||
      output_name == "CLKOUT4" ||
      output_name == "CLKOUT5" ||
      output_name == "CLKOUT6" ||
      output_name == "CLKFBOUT" ||
      output_name == "CLKFBOUTB" ||
      output_name == "DO" ||
      output_name == "LOCKED" ||
      output_name == "DRDY" ||
      output_name == "PSDONE" ||
      output_name == "CLKINSTOPPED" ||
      output_name == "CLKFBSTOPPED") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxMmcme2_AdvNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  return 0.5;
}

double XilinxMuxNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxMuxNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  return ComputeShannonEntropy(output_name, bit_high, wires, nodes);
}

double XilinxMuxNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  const string& sel_signal =
      named_input_connections.at("S").connection_bit_names.at(0);
  FunctionalEdge* sel_wire = wires->at(sel_signal);
  double prob_sel_1 = sel_wire->ProbabilityOne(wires, nodes);
  double prob_sel_0 = 1.0 - prob_sel_1;
  string i1_signal =
      named_input_connections.at("I1").connection_bit_names.at(0);
  string i0_signal =
      named_input_connections.at("I0").connection_bit_names.at(0);
  double i1_p1 = wires->at(i1_signal)->ProbabilityOne(wires, nodes);
  double i0_p1 = wires->at(i0_signal)->ProbabilityOne(wires, nodes);
  return prob_sel_1 * i1_p1 + prob_sel_0 * i0_p1;
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

double XilinxRamb18e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "DOADO" || output_name == "DOBDO") {
    return ComputeEntropy(output_name, wires, nodes, 15, 0);
  } else if (output_name == "DOPADOP" || output_name == "DOPBDOP") {
    return ComputeEntropy(output_name, wires, nodes, 1, 0);
  } else {
    throw std::exception();
  }
}

double XilinxRamb18e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "DOADO" ||
      output_name == "DOBDO" ||
      output_name == "DOPADOP" ||
      output_name == "DOPBDOP") {
    double total_entropy = 0.0;
    for (int i = bit_low; i <= bit_high; ++i) {
      total_entropy += ComputeShannonEntropy(output_name, i, wires, nodes);
    }
    return total_entropy;
  } else {
    throw std::exception();
  }
}

double XilinxRamb18e1Node::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  // Currently, we don't try to predict output values for RAM.
  return 0.5;
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

double XilinxRamb36e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  if (output_name == "CASCADEOUTA" ||
      output_name == "CASCADEOUTB" ||
      output_name == "SBITERR" ||
      output_name == "DBITERR") {
    return ComputeEntropy(output_name, wires, nodes, 0, 0);
  } else if (output_name == "ECCPARITY") {
    return ComputeEntropy(output_name, wires, nodes, 7, 0);
  } else if (output_name == "RDADDRECC") {
    return ComputeEntropy(output_name, wires, nodes, 8, 0);
  } else if (output_name == "DOADO" ||
             output_name == "DOBDO" ||
             output_name == "DOPADOP" ||
             output_name == "DOPBDOP") {
    return ComputeEntropy(output_name, wires, nodes, 31, 0);
  } else {
    throw std::exception();
  }
}

double XilinxRamb36e1Node::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "CASCADEOUTA" ||
      output_name == "CASCADEOUTB" ||
      output_name == "SBITERR" ||
      output_name == "DBITERR" ||
      output_name == "DOADO" ||
      output_name == "DOBDO" ||
      output_name == "DOPADOP" ||
      output_name == "DOPBDOP" ||
      output_name == "ECCPARITY" ||
      output_name == "RDADDRECC") {
    double total_entropy = 0.0;
    for (int i = bit_low; i <= bit_high; ++i) {
      total_entropy += ComputeShannonEntropy(output_name, i, wires, nodes);
    }
    return total_entropy;
  } else {
    throw std::exception();
  }
}

double XilinxRamb36e1Node::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  // Currently, we don't try to predict output values for RAM.
  return 0.5;
}

void XilinxRam32mNode::AddConnection(const ConnectionDescriptor& connection) {
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

double XilinxRam32mNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 1, 0);
}

double XilinxRam32mNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "DOA" ||
      output_name == "DOB" ||
      output_name == "DOC" ||
      output_name == "DOD") {
    if (bit_high != 1 || bit_low != 0) {
      throw std::exception();
    }
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxRam32mNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  // Currently, we don't try to predict output values for RAM.
  return 0.5;
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

double XilinxRam64mNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxRam64mNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "DOA" ||
      output_name == "DOB" ||
      output_name == "DOC" ||
      output_name == "DOD") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxRam64mNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  // Currently, we don't try to predict output values for RAM.
  return 0.5;
}

void XilinxRam64x1dNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "SPO" ||
      port_name == "DPO") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "WE" ||
             port_name == "D" ||
             port_name == "WCLK" ||
             port_name == "A0" ||
             port_name == "A1" ||
             port_name == "A2" ||
             port_name == "A3" ||
             port_name == "A4" ||
             port_name == "A5" ||
             port_name == "DPRA0" ||
             port_name == "DPRA1" ||
             port_name == "DPRA2" ||
             port_name == "DPRA3" ||
             port_name == "DPRA4" ||
             port_name == "DPRA5") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxRam64x1dNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxRam64x1dNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "SPO" ||
      output_name == "DPO") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxRam64x1dNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  // Currently, we don't try to predict output values for RAM.
  return 0.5;
}

void XilinxRam128x1sNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "WE" ||
             port_name == "D" ||
             port_name == "WCLK" ||
             port_name == "A0" ||
             port_name == "A1" ||
             port_name == "A2" ||
             port_name == "A3" ||
             port_name == "A4" ||
             port_name == "A5" ||
             port_name == "A6") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxRam128x1sNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxRam128x1sNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "O") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxRam128x1sNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  // Currently, we don't try to predict output values for RAM.
  return 0.5;
}

void XilinxRam256x1sNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "O") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "WE" ||
             port_name == "D" ||
             port_name == "WCLK" ||
             port_name == "A") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxRam256x1sNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxRam256x1sNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (output_name == "O") {
    return ComputeShannonEntropy(output_name, wires, nodes, bit_high, bit_low);
  } else {
    throw std::exception();
  }
}

double XilinxRam256x1sNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  // Currently, we don't try to predict output values for RAM.
  return 0.5;
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

double XilinxSrl16eNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxSrl16eNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (bit_high != bit_low) {
    throw std::exception();
  }
  return ComputeShannonEntropy(output_name, bit_high, wires, nodes);
}

double XilinxSrl16eNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name != "Q" && output_name != "") {
    std::cout << "Unexpected output name: " << output_name << "\n";
    throw std::exception();
  } else {
     string input_edge_name =
         named_input_connections.at("D").connection_bit_names.front();
     FunctionalEdge* input_edge = wires->at(input_edge_name);
     return input_edge->ProbabilityOne(wires, nodes);
  }
}

void XilinxSrlc32eNode::AddConnection(const ConnectionDescriptor& connection) {
  const string& port_name = connection.port_name;
  if (port_name == "Q" ||
      port_name == "Q31") {
    named_output_connections.insert(make_pair(connection.port_name, connection));
  } else if (port_name == "A" || port_name == "CE" ||
             port_name == "CLK" || port_name == "D") {
    named_input_connections.insert(make_pair(connection.port_name, connection));
  } else {
    cout << "Unexpected port name: " << connection.port_name << endl;
    throw std::exception();
  }
}

double XilinxSrlc32eNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return ComputeEntropy(output_name, wires, nodes, 0, 0);
}

double XilinxSrlc32eNode::ComputeEntropy(
      const string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  if (bit_high != bit_low) {
    throw std::exception();
  }
  return ComputeShannonEntropy(output_name, bit_high, wires, nodes);
}

double XilinxSrlc32eNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  if (output_name != "Q" && output_name != "Q31" && output_name != "") {
    std::cout << "Unexpected output name: " << output_name << "\n";
    throw std::exception();
  } else {
     string input_edge_name =
         named_input_connections.at("D").connection_bit_names.front();
     FunctionalEdge* input_edge = wires->at(input_edge_name);
     return input_edge->ProbabilityOne(wires, nodes);
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

double XilinxVccNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const {
  return 0.0;
}

double XilinxVccNode::ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const {
  return 0.0;
}

double XilinxVccNode::ComputeProbabilityOne(
    const string& output_name, int bit_pos, EdgeMap* wires,
    NodeMap* nodes) const {
  return 1.0;
}

}  // namespace vivado

