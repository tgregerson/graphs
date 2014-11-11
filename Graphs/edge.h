#ifndef EDGE_H_
#define EDGE_H_

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

const int kMaxNameChars = 20;

class Edge {
 public:
  typedef std::set<int> NodeIdSet;
  typedef std::vector<int> NodeIdVector;

  static bool use_entropy;

  Edge() {}
  Edge(int edge_id, const std::string& edge_name = "");
  virtual ~Edge() {}

  // Add/remove a connection to a node or port.
  void AddConnection(int cnx_id);
  void RemoveConnection(int cnx_id);

  // Copies all data from 'src'. NOTE: This includes the ID. Care must be
  // taken not to have separate objects with duplicate IDs in the same
  // container, as the assumption is that IDs are unique.
  virtual void CopyFrom(Edge* src);

  // Returns the total number of connected nodes/ports for the hyperedge.
  int degree() const;

  double Width() const {
    return width_;
  }
  void SetWidth(double width) {
    width_ = width;
  }

  double Entropy() const {
    return entropy_;
  }
  void SetEntropy(double entropy) {
    entropy_ = entropy;
  }

  double Weight() const {
    if (use_entropy) {
      return Entropy();
    } else {
      return Width();
    }
  }
  void SetWeight(double weight) {
    if (use_entropy) {
      SetEntropy(weight);
    } else {
      SetWidth(weight);
    }
  }

  const std::string GenerateSplitEdgeName(int new_id) const;

  // Print debug information about the edge.
  virtual void Print() const;

  // Reduces the memory size of the object by removing its name and resizing its containers.
  virtual void Compress();

  const NodeIdVector& connection_ids() const {
    return connection_ids_;
  }

  // connection_ids is kept sorted for fast searching.
  int id_{-1};
  double entropy_{0.1};
  double width_{1.0};
  std::string name;
 protected:
  NodeIdVector connection_ids_;
};

#endif /* EDGE_H_ */
