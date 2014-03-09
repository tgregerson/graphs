/* NOTE: To link with the libxml libraries, use `xml2-config --cflags --libs` when compiling with GCC.
** Must have LIBXML2 installed to compile this code: sudo apt-get install libxml */

#ifndef XML_CONFIG_READER_H_
#define XML_CONFIG_READER_H_

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "partitioner_config.h"

class XmlConfigReader {
 public:
  PartitionerConfig ReadFile(const char* filename);

 private:
  PartitionerConfig PopulatePartitionerConfig(xmlNodePtr rootNodePtr);
  void PopulateGeneralConfiguration(PartitionerConfig* partitioner_config,
                                    xmlNodePtr curNodePtr);
  void PopulatePreprocessorConfiguration(PartitionerConfig* partitioner_config,
                                         xmlNodePtr curNodePtr);
  void PopulateKlfmConfiguration(PartitionerConfig* partitioner_config,
                                 xmlNodePtr curNodePtr);
  void PopulatePostprocessorConfiguration(PartitionerConfig* partitioner_config,
                                          xmlNodePtr curNodePtr);
  xmlNodePtr NextNonComment(xmlNodePtr ptr);
  xmlNodePtr ChildNonComment(xmlNodePtr ptr);
};

#endif /* XML_CONFIG_READER_H_ */
