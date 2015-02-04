/*
 * connection_descriptor.h
 *
 *  Created on: Oct 30, 2014
 *      Author: gregerso
 */

#ifndef CONNECTION_DESCRIPTOR_H_
#define CONNECTION_DESCRIPTOR_H_

#include <cassert>

#include <exception>
#include <string>
#include <vector>

class ConnectionDescriptor {
 public:
  ConnectionDescriptor(const std::string& pn) : port_name(pn) {}
  // Convenience interface for single-bit connections.
  ConnectionDescriptor(const std::string& pn, const std::string& cbn)
    : port_name(pn) {
    AddBitConnection(0, cbn);
  }
  ConnectionDescriptor(const std::string& pn, int pw,
                        const std::vector<std::string>& cbn)
    : port_name(pn), port_width(pw), connection_bit_names(cbn) {}

  void AddBitConnection(unsigned int index, const std::string& bit_name) {
    assert(!bit_name.empty());
    if (connection_bit_names.size() <= index) {
      connection_bit_names.resize(index + 1);
      port_width = index + 1;
    }
    connection_bit_names[index] = bit_name;
  }

  void AddBitConnection(const std::string& bit_name) {
    assert(!bit_name.empty());
    connection_bit_names.push_back(bit_name);
    ++port_width;
  }

  void RemoveBitConnection(unsigned int index, const std::string& bit_name) {
    if (connection_bit_names.at(index) != bit_name) {
      throw std::exception();
    } else {
      connection_bit_names.at(index) = "";
    }
  }

  void RemoveBitConnection(const std::string& bit_name) {
    for (size_t i = 0; i < connection_bit_names.size(); ++i) {
      if (connection_bit_names[i] == "bit_name") {
        connection_bit_names[i].clear();
      }
    }
  }

  // Useful because Verilog presents bit names in descending order,
  // so it may be convenient to call after doing AddBitConnection on
  // descending bits.
  void ReverseConnectedBitNames() {
    std::reverse(connection_bit_names.begin(), connection_bit_names.end());
  }

  std::string port_name;
  int port_width{0};
  std::vector<std::string> connection_bit_names;
};

#endif /* CONNECTION_DESCRIPTOR_H_ */
