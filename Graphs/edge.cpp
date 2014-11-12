#include "edge.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <sstream>

using namespace std;

Edge::Edge(int edge_id, const string& edge_name)
  : id_(edge_id), name(edge_name) {
  // TODO - Omitting name saves memory.
  if (name.empty()) {
    // Create generic name.
    name = "Edge";
    ostringstream oss;
    oss << edge_id;
    name.append(oss.str());
  }
}

const std::string Edge::GenerateSplitEdgeName(int new_id) const {
  string new_edge_name;
  ostringstream oss;
  oss << "_split_";
  oss << new_id;
  if (!name.empty()) {
    // Make a new name that is 'ORIGINALNAME_split_NEWID'.
    new_edge_name = name;
    string separator = "_split_";

    // Strip off the '_split_ID' if it is already in the name. Prevents
    // names from growing in an unbounded fashion if an edge is repeatedly
    // split.
    size_t pos = new_edge_name.find(separator);
    if (pos != string::npos) {
      new_edge_name.resize(pos);
    }
  } else {
    assert(false);
    new_edge_name = "Edge";
  }
  new_edge_name.append(oss.str());
  return new_edge_name;
}

void Edge::AddConnection(int cnx_id) {
  // Insert keeps connection_ids sorted
  if (connection_ids_.empty() || connection_ids_.back() <= cnx_id) {
    connection_ids_.push_back(cnx_id);
  } else {
    connection_ids_.insert(
        lower_bound(connection_ids_.begin(), connection_ids_.end(), cnx_id),
        cnx_id);
  }
}

void Edge::RemoveConnection(int cnx_id) {
  auto erase_it =
      lower_bound(connection_ids_.begin(), connection_ids_.end(), cnx_id);
  assert(erase_it != connection_ids_.end() && *erase_it == cnx_id);
  connection_ids_.erase(erase_it);
}

void Edge::CopyFrom(Edge* src) {
  id_ = src->id_;
  width_ = src->width_;
  entropy_ = src->entropy_;
  name = src->name;
  connection_ids_ = src->connection_ids();
}

void Edge::Print() const {
  printf("Edge  ID=%d  Name=%s\n", id_, name.c_str());
  printf("Weight: %f\n", this->Weight());
  printf("Degree: %d\n", degree());
  printf("Connected IDs: ");
  for (auto it : connection_ids_) {
    printf("%d ", it);
  }
  printf("\n");
}

void Edge::Compress() {
  //name.clear();
  connection_ids_.shrink_to_fit();
}

int Edge::degree() const {
  return connection_ids_.size();
}

bool Edge::use_entropy_ = false;
