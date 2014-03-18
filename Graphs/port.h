#ifndef PORT_H_
#define PORT_H_

#include <string>

class Port {
 public:
  typedef enum {
    kInputType,
    kOutputType,
    kDontCareType
  } PortType;

  Port() {}
  explicit Port(int id) : id(id) {}
  Port(int id, int internal_edge_id, int external_edge_id, PortType type,
       std::string name = "")
    : id(id), internal_edge_id(internal_edge_id),
      external_edge_id(external_edge_id), type(type), name(name) {}

  void Print() const;

  int id;
  int internal_edge_id;
  int external_edge_id;
  PortType type;
  std::string name;
};

#endif /* PORT_H_ */
