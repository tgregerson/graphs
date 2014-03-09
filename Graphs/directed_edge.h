#ifndef DIRECTED_EDGE_H_
#define DIRECTED_EDGE_H_

#include <string>
#include <unordered_set>

#include "edge.h"

class DirectedEdge : public Edge {
 public:
  DirectedEdge() {}
  DirectedEdge(int edge_id, int weight = 1, std::string name = "");
  virtual ~DirectedEdge() {}

  // Add a source node to the hyperedge. Logically, there should be one, and
  // only one, source node for each edge unless the edge is driven by
  // tri-state buffers. Edge must not already contain 'source_id' as a source.
  void AddSource(int source_id);
  void RemoveSource(int source_id);

  // Add a sink node to the hyperedge. A hyperedge may contain multiple sinks.
  // Edge must not already contain 'sink_id' as a sink.
  void AddSink(int sink_id);
  void RemoveSink(int sink_id);

  // Copies all data from 'src'. NOTE: This includes the ID. Care must be
  // taken not to have separate objects with duplicate IDs in the same
  // container, as the assumption is that IDs are unique.
  //TODO: Rework copying in a way that doesn't have problems with
  // polymorphism
  virtual void CopyFrom(Edge* src);

  // Returns the number of sink nodes for the hyperedge.
  int fanout() const;

  // Print debug information about the edge.
  virtual void Print() const;

  std::unordered_set<int> source_ids;
  std::unordered_set<int> sink_ids;
};

#endif /* DIRECTED_EDGE_H_ */
