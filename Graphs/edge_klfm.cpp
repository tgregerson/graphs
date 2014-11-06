#include "edge_klfm.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>

using namespace std;

EdgeKlfm::EdgeKlfm(Edge* edge) {
  CopyFrom(edge);
  Compress();
}

EdgeKlfm::EdgeKlfm(int edge_id, const string& edge_name)
  : Edge(edge_id, edge_name) {}

void EdgeKlfm::Print() const {
  Edge::Print();
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

void EdgeKlfm::SetInitialCriticality() {
  // No nodes are locked initially, so this can be done simply.
  is_critical = ((part_a_connected_unlocked_nodes.size() < 2) ||
                 (part_b_connected_unlocked_nodes.size() < 2));
}

void EdgeKlfm::MoveNode(int node_id, NodeIdVector* nodes_to_increase_gain,
                        NodeIdVector* nodes_to_reduce_gain) {
  /* Note: There is a certain case where this algorithm incorrectly sets
     an edge as critical when it is not: when all nodes are locked in only
     one partition. However once this case occurs, the criticality of the
     edge should no longer matter in the KLFM algorithm, as it should only
     be checked when moving a node connected to the edge, which will never
     happen once they are all locked. Therefore it should not be necessary
     to detect this case. */

  // Check if node is moving from a to b.
  bool from_part_a = find(part_a_connected_unlocked_nodes.begin(),
                          part_a_connected_unlocked_nodes.end(), node_id) !=
                          part_a_connected_unlocked_nodes.end();
  auto& from_part_connected_locked_nodes = (from_part_a) ?
      part_a_connected_locked_nodes : part_b_connected_locked_nodes;
  auto& to_part_connected_locked_nodes = (from_part_a) ?
      part_b_connected_locked_nodes : part_a_connected_locked_nodes;
  auto& from_part_connected_unlocked_nodes = (from_part_a) ?
      part_a_connected_unlocked_nodes : part_b_connected_unlocked_nodes;
  auto& to_part_connected_unlocked_nodes = (from_part_a) ?
      part_b_connected_unlocked_nodes : part_a_connected_unlocked_nodes;

  // Perform the move
  auto erase_it = find(from_part_connected_unlocked_nodes.begin(),
                       from_part_connected_unlocked_nodes.end(), node_id);
  assert(erase_it != from_part_connected_unlocked_nodes.end());
  from_part_connected_unlocked_nodes.erase(erase_it);
  to_part_connected_locked_nodes.push_back(node_id);

  // Determine which connected nodes need their gain updated due to the
  // node movement on this edge. If the edge was not previously critical,
  // no gain updates are needed.
  if (is_critical) {
    // Handle changes due to TO PART previously being empty or having a single,
    // unlocked node.
    if (to_part_connected_locked_nodes.size() == 1) {
      if (to_part_connected_unlocked_nodes.empty()) {
        // TO PART is no longer empty, so increase the gain of all
        // unlocked nodes on FROM PART (increase from negative to zero).
        for (auto it : from_part_connected_unlocked_nodes) {
          nodes_to_increase_gain->push_back(it);
        }
      } else if (to_part_connected_unlocked_nodes.size() == 1) {
        // TO PART used to have a solo unlocked node, but now has a
        // locked node partner, so the unlocked node's gain is decreased.
        // (decrease from positive to zero)
        nodes_to_reduce_gain->push_back(
            *to_part_connected_unlocked_nodes.begin());
      }
    }
    // Handle changes due to FROM PART going from 2->1 or 1->0.
    if (from_part_connected_locked_nodes.empty()) {
      if (from_part_connected_unlocked_nodes.size() == 0) {
        // FROM is now empty, so decrease the gain of any
        // unlocked nodes on TO PART.
        // (decrease from zero to negative)
        for (auto it : to_part_connected_unlocked_nodes) {
          nodes_to_reduce_gain->push_back(it);
        }
      } else if (from_part_connected_unlocked_nodes.size() == 1) {
        // FROM PART has a lone unlocked node left behind, so increase
        // that node's gain.
        nodes_to_increase_gain->push_back(
            *from_part_connected_unlocked_nodes.begin());
      }
    }
  }

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

double EdgeKlfm::GainContributionToNode(int node_id) {
  return 0.0;
}
