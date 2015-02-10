/*
 * xilinx_functional_nodes.h
 *
 *  Created on: Sep 4, 2014
 *      Author: gregerso
 */

#ifndef XILINX_FUNCTIONAL_NODES_H_
#define XILINX_FUNCTIONAL_NODES_H_

#include "functional_node.h"

#include <string>

class FunctionalEdge;

// May need to replace this with versioned namespaces if we need to handle
// modules differently with different versions of ISE/Vivado.
namespace vivado {

class EpimBlackboxNode : public FunctionalNode {
 public:
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxBufNode : public FunctionalNode {
 public:
  XilinxBufNode() {}
  virtual ~XilinxBufNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxBufgNode : public XilinxBufNode {
 public:

  XilinxBufgNode() {}
  virtual ~XilinxBufgNode() {}
};

class XilinxCarry4Node : public FunctionalNode {
 public:
  XilinxCarry4Node() {}
  virtual ~XilinxCarry4Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxDsp48e1Node : public FunctionalNode {
 public:
  XilinxDsp48e1Node() {}
  virtual ~XilinxDsp48e1Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxFdNode : public FunctionalNode {
 public:
  virtual void AddConnection(const ConnectionDescriptor& connection) = 0;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxFdceNode : public XilinxFdNode {
 public:
  XilinxFdceNode() {}
  virtual ~XilinxFdceNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
};

class XilinxFdpeNode : public XilinxFdNode {
 public:
  XilinxFdpeNode() {}
  virtual ~XilinxFdpeNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
};

// TODO Could add more detailed entropy calculation for FDRE and FDSE based
// on synchronous set/reset.
class XilinxFdreNode : public XilinxFdNode {
 public:
  XilinxFdreNode() {}
  virtual ~XilinxFdreNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
};

class XilinxFdseNode : public XilinxFdNode {
 public:
  XilinxFdseNode() {}
  virtual ~XilinxFdseNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
};

class XilinxFifo18e1Node : public FunctionalNode {
 public:
  XilinxFifo18e1Node() {}
  virtual ~XilinxFifo18e1Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxFifo36e1Node : public FunctionalNode {
 public:
  XilinxFifo36e1Node() {}
  virtual ~XilinxFifo36e1Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxGndNode : public FunctionalNode {
 public:
  XilinxGndNode() {}
  virtual ~XilinxGndNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxIbufNode : public XilinxBufNode {
 public:
  XilinxIbufNode() {}
  virtual ~XilinxIbufNode() {}
};

class XilinxLdceNode : public XilinxFdNode {
 public:
  XilinxLdceNode() {}
  virtual ~XilinxLdceNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
};

class XilinxLutNode : public FunctionalNode {
 public:
  virtual ~XilinxLutNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxLut1Node : public XilinxLutNode {
 public:
  XilinxLut1Node() {}
  virtual ~XilinxLut1Node() {}
};

class XilinxLut2Node : public XilinxLutNode {
 public:
  XilinxLut2Node() {}
  virtual ~XilinxLut2Node() {}
};

class XilinxLut3Node : public XilinxLutNode {
 public:
  XilinxLut3Node() {}
  virtual ~XilinxLut3Node() {}
};

class XilinxLut4Node : public XilinxLutNode {
 public:
  XilinxLut4Node() {}
  virtual ~XilinxLut4Node() {}
};

class XilinxLut5Node : public XilinxLutNode {
 public:
  XilinxLut5Node() {}
  virtual ~XilinxLut5Node() {}
};

class XilinxLut6Node : public XilinxLutNode {
 public:
  XilinxLut6Node() {}
  virtual ~XilinxLut6Node() {}
};

class XilinxObufNode : public XilinxBufNode {
 public:
  XilinxObufNode() {}
  virtual ~XilinxObufNode() {}
};

class XilinxMmcme2_AdvNode : public FunctionalNode {
 public:
  XilinxMmcme2_AdvNode() {}
  virtual ~XilinxMmcme2_AdvNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxMuxNode : public FunctionalNode {
 public:
  virtual ~XilinxMuxNode() {}
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxMuxf7Node : public XilinxMuxNode {
 public:
  XilinxMuxf7Node() {}
  virtual ~XilinxMuxf7Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
};

class XilinxMuxf8Node : public XilinxMuxNode {
 public:
  XilinxMuxf8Node() {}
  virtual ~XilinxMuxf8Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
};

class XilinxRamb18e1Node : public FunctionalNode {
 public:
  XilinxRamb18e1Node() {}
  virtual ~XilinxRamb18e1Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxRamb36e1Node : public FunctionalNode {
 public:
  XilinxRamb36e1Node() {}
  virtual ~XilinxRamb36e1Node() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxRam32mNode : public FunctionalNode {
 public:
  XilinxRam32mNode() {}
  virtual ~XilinxRam32mNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxRam64mNode : public FunctionalNode {
 public:
  XilinxRam64mNode() {}
  virtual ~XilinxRam64mNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxRam64x1dNode : public FunctionalNode {
 public:
  XilinxRam64x1dNode() {}
  virtual ~XilinxRam64x1dNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxRam128x1sNode : public FunctionalNode {
 public:
  XilinxRam128x1sNode() {}
  virtual ~XilinxRam128x1sNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxRam256x1sNode : public FunctionalNode {
 public:
  XilinxRam256x1sNode() {}
  virtual ~XilinxRam256x1sNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxSrl16eNode : public FunctionalNode {
 public:
  XilinxSrl16eNode() {}
  virtual ~XilinxSrl16eNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxSrlc32eNode : public FunctionalNode {
 public:
  XilinxSrlc32eNode() {}
  virtual ~XilinxSrlc32eNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

class XilinxVccNode : public FunctionalNode {
 public:
  XilinxVccNode() {}
  virtual ~XilinxVccNode() {}
  virtual void AddConnection(const ConnectionDescriptor& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* wires, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* wires,
      NodeMap* nodes) const;
};

}  // namespace vivado

#endif /* XILINX_FUNCTIONAL_NODES_H_ */
