#include "directed_edge.h"

#include <cassert>
#include <cstdio>
#include <sstream>

using std::ostringstream;
using std::string;

DirectedEdge::DirectedEdge(int edge_id, int weight, string name)
  : Edge(edge_id, weight, name) {
  if (name.empty()) {
    // Create generic name.
    name = "Edge";
    ostringstream oss;
    oss << edge_id;
    name.append(oss.str());
  }
}

void DirectedEdge::AddSource(int source_id) {
  // This check should be removed to support tri-state nets. Currently
  // left in for debugging purposes.
  assert(source_ids.empty());
  // Assert that the ID hasn't already been added.
  assert(source_ids.insert(source_id).second);
  assert(connection_ids.insert(source_id).second);
}

void DirectedEdge::AddSink(int sink_id) {
  // Assert that the ID hasn't already been added.
  assert(sink_ids.insert(sink_id).second);
  assert(connection_ids.insert(sink_id).second);
}

void DirectedEdge::RemoveSource(int source_id) {
  assert(source_ids.erase(source_id) == 1);
  assert(connection_ids.erase(source_id) == 1);
}

void DirectedEdge::RemoveSink(int sink_id) {
  assert(sink_ids.erase(sink_id) == 1);
  assert(connection_ids.erase(sink_id) == 1);
}

void DirectedEdge::CopyFrom(Edge* src) {
  DirectedEdge* d_src = dynamic_cast<DirectedEdge*>(src);
  id = d_src->id;
  weight = d_src->weight;
  name = d_src->name;
  for (auto it : d_src->source_ids) {
    AddSource(it);
  }
  for (auto it : d_src->sink_ids) {
    AddSink(it);
  }
}

void DirectedEdge::Print() const {
  printf("Edge  ID=%d  Name=%s\n", id, name.c_str()); 
  printf("Weight: %d\n", weight);
  printf("Degree: %d\n", degree());
  printf("Source IDs: ");
  for (auto it : source_ids) {
    printf("%d ", it);
  }
  printf("\n");
  printf("Sink IDs: ");
  for (auto it : sink_ids) {
    printf("%d ", it);
  }
  printf("\n");
}

int DirectedEdge::fanout() const {
  return sink_ids.size();
}
