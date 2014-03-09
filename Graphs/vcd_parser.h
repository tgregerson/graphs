#ifndef VCD_PARSER_H_
#define VCD_PARSER_H_

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

class VcdParser {
 public:
  VcdParser(const std::string& vcd_filename,
            const std::set<std::string>& signals)
    : vcd_filename_(vcd_filename), raw_signals_(signals) {
    for (const std::string& signal : raw_signals_) {
      monitored_signals_.insert(StripLeadingEscapeChar(signal));
    }
  }

  // Parses input VCD file. Parses nets in 'signals'. Writes status messages
  // to cout if 'echo_status'.
  void Parse(bool echo_status);

  static void WriteDumpVars(const std::string& signal_filename,
                            const std::string& output_filename,
                            const std::string& scope);

 private:
  void SkipToEndToken(std::ifstream& input_file);

  void ParseScope(std::ifstream& input_file,
                  const std::string& parent_scope);


  void ParseVar(std::ifstream& input_file);

  int ParseDumpAll(std::ifstream& input_file);

  void PrintStats();

  void CheckSignalsExistOrDie();

  // Removes the leading '\' character that is present in signal names, but
  // not printed in var names by ModelSim.
  static std::string StripLeadingEscapeChar(const std::string& str);

  const std::string vcd_filename_;

  bool echo_status_;
  std::map<std::string, std::string> net_name_to_short_name_;
  std::map<std::string, std::vector<char>> short_name_to_values_;

  // Only store data from these signals, even if VCD defines additional vars.
  std::set<std::string> raw_signals_;
  // Monitored signals are the raw signals modified to match the format of the
  // VCD file (i.e. bus versus bit-select, removal of escape codes).
  std::set<std::string> monitored_signals_;

  // Includes names of vars that are not selected signals. Used for debugging.
  std::set<std::string> all_var_names_;
};

#endif /* VCD_PARSER_H_ */
