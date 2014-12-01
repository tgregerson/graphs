#ifndef VCD_PARSER_H_
#define VCD_PARSER_H_

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class VcdParser {
 public:
  VcdParser(const std::string& vcd_filename,
            const std::set<std::string>& signals)
    : vcd_filename_(vcd_filename), raw_monitored_signals_(signals) {
    for (const std::string& signal : raw_monitored_signals_) {
      name_fixed_monitored_signals_.insert(ConvertToVcdSignalName(signal));
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

  long long ParseDumpAll(std::ifstream& input_file);

  void PrintStats();

  void CheckSignalsExistOrDie();

  // Converts a signal name to the format used by the VCD file. Bit selects are
  // removed and stored in the second part of the pair. If the signal name was
  // escaped, removes the leading '\\' and the trailing space.
  std::pair<std::string, int> ConvertToVcdSignalName(
      const std::string& raw_signal_name);

  // Removes the leading '\' character that is present in signal names, but
  // not printed in var names by ModelSim.
  static std::string StripLeadingEscapeChar(const std::string& str);

  const std::string vcd_filename_;

  bool echo_status_;
  std::map<std::pair<std::string, int>, std::string>
      modified_signal_name_to_identifier_code_;
  std::unordered_map<std::string, std::vector<char>> identifier_code_to_values_;

  // These are the signals that were requested for monitoring using their raw
  // names from the parsed verilog file. VCD does not properly support escaped
  // variable names, so these names may need to be modified to match the
  // identifiers used in the VCD.
  std::set<std::string> raw_monitored_signals_;

  // Contains the monitored signals, with escaped signal names modified to
  // match the requirements of the VCD format. Bit selects are removed from
  // the name and included in the second element of the pair.
  std::set<std::pair<std::string, int>> name_fixed_monitored_signals_;

  std::set<std::string> monitored_identifier_codes_;

  std::set<std::pair<std::string, int>> all_vcd_identifier_names_;
};

#endif /* VCD_PARSER_H_ */
