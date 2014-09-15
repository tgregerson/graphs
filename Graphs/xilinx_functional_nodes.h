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

class XilinxBufgNode : public FunctionalNode {
 public:

  XilinxBufgNode() {}
  virtual ~XilinxBufgNode() {}
  virtual void AddConnection(const NamedConnection& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes,
      int bit_high, int bit_low) const;
};

class XilinxCarry4Node : public FunctionalNode {
 public:
  XilinxCarry4Node() {}
  virtual ~XilinxCarry4Node() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxDsp48e1Node : public FunctionalNode {
 public:
  XilinxDsp48e1Node() {}
  virtual ~XilinxDsp48e1Node() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxFdceNode : public FunctionalNode {
 public:
  XilinxFdceNode() {}
  virtual ~XilinxFdceNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxFdpeNode : public FunctionalNode {
 public:
  XilinxFdpeNode() {}
  virtual ~XilinxFdpeNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxFdreNode : public FunctionalNode {
 public:
  XilinxFdreNode() {}
  virtual ~XilinxFdreNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxFdseNode : public FunctionalNode {
 public:
  XilinxFdseNode() {}
  virtual ~XilinxFdseNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxGndNode : public FunctionalNode {
 public:
  XilinxGndNode() {}
  virtual ~XilinxGndNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxIbufNode : public FunctionalNode {
 public:
  XilinxIbufNode() {}
  virtual ~XilinxIbufNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxLutNode : public FunctionalNode {
 public:
  virtual ~XilinxLutNode() {}
  virtual void AddConnection(const NamedConnection& connection);
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes) const;
  virtual double ComputeEntropy(
      const std::string& output_name, EdgeMap* edges, NodeMap* nodes,
      int bit_high, int bit_low) const;
  virtual double ComputeProbabilityOne(
      const std::string& output_name, int bit_pos, EdgeMap* edges,
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

class XilinxObufNode : public FunctionalNode {
 public:
  XilinxObufNode() {}
  virtual ~XilinxObufNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxMuxf7Node : public FunctionalNode {
 public:
  XilinxMuxf7Node() {}
  virtual ~XilinxMuxf7Node() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxMuxf8Node : public FunctionalNode {
 public:
  XilinxMuxf8Node() {}
  virtual ~XilinxMuxf8Node() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxRamb18e1Node : public FunctionalNode {
 public:
  XilinxRamb18e1Node() {}
  virtual ~XilinxRamb18e1Node() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxRamb36e1Node : public FunctionalNode {
 public:
  XilinxRamb36e1Node() {}
  virtual ~XilinxRamb36e1Node() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxRam64mNode : public FunctionalNode {
 public:
  XilinxRam64mNode() {}
  virtual ~XilinxRam64mNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxSrl16eNode : public FunctionalNode {
 public:
  XilinxSrl16eNode() {}
  virtual ~XilinxSrl16eNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

class XilinxVccNode : public FunctionalNode {
 public:
  XilinxVccNode() {}
  virtual ~XilinxVccNode() {}
  virtual void AddConnection(const NamedConnection& connection);
};

}  // namespace vivado

#endif /* XILINX_FUNCTIONAL_NODES_H_ */
