#ifndef PARSER_INTERFACE_H_
#define PARSER_INTERFACE_H_

class Node;

class ParserInterface {
 public:
  virtual ~ParserInterface() {}

  virtual bool Parse(Node* top_level_graph, const char* filename) = 0;
};

#endif /* PARSER_INTERFACE_ */
