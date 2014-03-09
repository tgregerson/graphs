#ifndef TESTBENCH_GENERATOR_H_
#define TESTBENCH_GENERATOR_H_

#include <iostream>
#include <fstream>
#include <set>
#include <string>

class TestbenchGenerator {
 public:
   static void GenerateVerilog(
       const std::string& filename, const std::set<std::string>& edges);

 private:
   static void WriteModulePreamble(std::ostream& os);
   static void WriteModuleClosing(std::ostream& os);
   static void WriteClk(std::ostream& os);
   static void WriteOneMonitor(std::ostream& os, const std::string& edge_name);
   static void WriteOneStrobe(std::ostream& os, const std::string& edge_name);

};

#endif /* TESTBENCH_GENERATOR_H_ */
