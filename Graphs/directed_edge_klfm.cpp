#include "directed_edge_klfm.h"

#include <cassert>
#include <cstdio>
#include <string>

using std::string;

DirectedEdgeKlfm::DirectedEdgeKlfm(DirectedEdge* edge) {
  is_critical = true;
  locked_noncritical = false;
  CopyFrom(edge);
}

void DirectedEdgeKlfm::Print() {
  DirectedEdge::Print();
  string val = (is_critical) ? "true" : "false";
  printf("Critical: %s\n", val.c_str());
  printf("Part A Node IDs: ");
  for (auto it : part_a_connected_unlocked_nodes) {
    printf("%d ", it);
  }
  for (auto it : part_a_connected_locked_nodes) {
    printf("%d ", it);
  }
  printf("\n");
  printf("Part B Node IDs: ");
  for (auto it : part_b_connected_unlocked_nodes) {
    printf("%d ", it);
  }
  for (auto it : part_b_connected_locked_nodes) {
    printf("%d ", it);
  }
  printf("\n");
}

void DirectedEdgeKlfm::KlfmReset(
    const NodeIdSet& part_a_connected_nodes,
    const NodeIdSet& part_b_connected_nodes) {
  part_a_connected_unlocked_nodes = part_a_connected_nodes;
  part_b_connected_unlocked_nodes = part_b_connected_nodes;
  part_a_connected_locked_nodes.clear();
  part_b_connected_locked_nodes.clear();

  locked_noncritical = false;

  SetInitialCriticality();
}

void DirectedEdgeKlfm::SetInitialCriticality() {
  // No nodes are locked initially, so this can be done simply.
  is_critical = ((part_a_connected_unlocked_nodes.size() < 2) ||
                 (part_b_connected_unlocked_nodes.size() < 2));
}

void DirectedEdgeKlfm::MoveNode(int node_id,
    NodeIdVector* nodes_to_increase_gain,
    NodeIdVector* nodes_to_reduce_gain) {
  assert(nodes_to_increase_gain != NULL);
  assert(nodes_to_reduce_gain != NULL);
  assert((part_a_connected_unlocked_nodes.count(node_id) != 0) &&
         (part_b_connected_unlocked_nodes.count(node_id) != 0));

  /* Note: There is a certain case where this algorithm incorrectly sets
     an edge as critical when it is not: when all nodes are locked in only
     one partition. However once this case occurs, the criticality of the
     edge should no longer matter in the KLFM algorithm, as it should only
     be checked when moving a node connected to the edge, which will never
     happen once they are all locked. Therefore it should not be necessary
     to detect this case. */

  // Check if node is moving from a to b.
  bool from_part_a = (part_a_connected_unlocked_nodes.count(node_id) != 0);
  auto& from_part_connected_locked_nodes = (from_part_a) ?
      part_a_connected_locked_nodes : part_b_connected_locked_nodes;
  auto& to_part_connected_locked_nodes = (from_part_a) ?
      part_b_connected_locked_nodes : part_a_connected_locked_nodes;
  auto& from_part_connected_unlocked_nodes = (from_part_a) ?
      part_a_connected_unlocked_nodes : part_b_connected_unlocked_nodes;
  auto& to_part_connected_unlocked_nodes = (from_part_a) ?
      part_b_connected_unlocked_nodes : part_a_connected_unlocked_nodes;

  // Determine which connected nodes need their gain updated due to the
  // node movement on this edge. If the edge was not previously critical,
  // no gain updates are needed.
  if (is_critical) {
    // Handle changes due to TO PART going from 0->1 or 1->2.
    if (to_part_connected_locked_nodes.empty()) {
      if (to_part_connected_unlocked_nodes.empty()) {
        // TO PART will no longer be empty, so increase the gain of all
        // unlocked nodes on FROM PART other than the one moving.
        // (increase from negative to zero)
        for (auto it : from_part_connected_unlocked_nodes) {
          if (it != node_id) {
            nodes_to_increase_gain->push_back(it);
          }
        }
      } else if (to_part_connected_unlocked_nodes.size() == 1) {
        // TO PART used to have a solo unlocked node, but will be getting a
        // locked node partner, so unlocked node's gain is decreased.
        // (decrease from positive to zero)
        nodes_to_reduce_gain->push_back(
            *to_part_connected_unlocked_nodes.begin());
      }
    }
    // Handle changes due to FROM PART going from 2->1 or 1->0.
    if (from_part_connected_locked_nodes.empty()) {
      if (from_part_connected_unlocked_nodes.size() == 1) {
        // FROM is about to become empty, so decrease the gain of any
        // unlocked nodes on TO PART.
        // (decrease from zero to negative)
        for (auto it : to_part_connected_unlocked_nodes) {
          nodes_to_reduce_gain->push_back(it);
        }
      } else if (from_part_connected_unlocked_nodes.size() == 2) {
        // FROM PART will have a lone unlocked node left behind, so increase
        // that node's gain.
        nodes_to_increase_gain->push_back(
            *from_part_connected_unlocked_nodes.begin());
      }
    }
  }

  // Now perform the move.
  from_part_connected_unlocked_nodes.erase(node_id);
  to_part_connected_locked_nodes.insert(node_id);

  // Update critical status of the edge.
  is_critical = false;
  if (!locked_noncritical) {
    if (!from_part_connected_locked_nodes.empty()) {
      // Edge now has locked nodes in both partitions. It is permanently
      // non-critical for the remainder of this iteration of KLFM, but the gain
      // updates for this move still need to take place.
      locked_noncritical = true;
    } else if (from_part_connected_unlocked_nodes.size() < 3) {
      is_critical = true;
    }
  }
}
