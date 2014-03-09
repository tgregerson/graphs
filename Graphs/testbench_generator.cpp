#include "testbench_generator.h"

#include <cassert>

using namespace std;

void TestbenchGenerator::GenerateVerilog(
    const string& filename, const set<string>& edges) {

  ofstream output_file;
  if (!filename.empty()) {
    output_file.open(filename.c_str(), ios_base::out | ios_base::app);
  }
  ostream& os = filename.empty() ? cout : output_file;

  WriteModulePreamble(os);
  WriteClk(os);
  for (const string& edge_name : edges) {
    WriteOneStrobe(os, edge_name);
  }
  WriteModuleClosing(os);
}

void TestbenchGenerator::WriteModulePreamble(ostream& os) {
  os << "module generated_testbench();" << endl;
}

void TestbenchGenerator::WriteModuleClosing(ostream& os) {
  os << "endmodule" << endl;
}

void TestbenchGenerator::WriteClk(ostream& os) {
  os << "reg clk;" << endl;
  os << "initial begin" << endl;
  os << "  clk = 1'b0;" << endl;
  os << "  forever #10 clk = ~clk;" << endl;
  os << "end" << endl;
}

void TestbenchGenerator::WriteOneMonitor(ostream& os, const string& edge_name) {
  os << "initial $monitor(\"";
  // Must excape '\' character in names.
  for (const char character : edge_name) {
    os << character;
    if (character == '\\') {
      os << '\\';    
    }
  }
  os << "=%b\", " << edge_name << ");" << endl;
}

void TestbenchGenerator::WriteOneStrobe(ostream& os, const string& edge_name) {
  os << "initial $strobe(\"";
  // Must excape '\' character in names.
  for (const char character : edge_name) {
    os << character;
    if (character == '\\') {
      os << '\\';    
    }
  }
  os << "=%b\", " << edge_name << ");" << endl;
}
