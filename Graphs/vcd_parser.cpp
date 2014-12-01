#include "vcd_parser.h"

#include <utility>

#include "universal_macros.h"

using namespace std;

void VcdParser::Parse(bool echo_status) {
  echo_status_ = echo_status;

  ifstream input_file(vcd_filename_);
  assert_b(input_file.is_open()) {
    cout << "Failed to open VCD file: " << vcd_filename_
         << ". Terminating." << endl;
  }

  if (echo_status_) {
    cout << "Reading VCD from " << vcd_filename_ << endl;
  }
  string token;

  cout << "Process header" << endl;

  // Process header section.
  input_file >> token;
  while (!input_file.eof()) {
    if (token == "$date") {
      SkipToEndToken(input_file);
    } else if (token == "$version") {
      SkipToEndToken(input_file);
    } else if (token == "$timescale") {
      SkipToEndToken(input_file);
    } else if (token == "$scope") {
      ParseScope(input_file, "");
    } else if (token == "$enddefinitions") {
      SkipToEndToken(input_file);
      break;
    } else {
      assert_b(false) {
        cout << "Unexpected token: " << token << " while processing header."
             << endl;
      }
    }
    input_file >> token;
  }

  CheckSignalsExistOrDie();

  cout << "Process value changes." << endl;

  // Process value changes.
  long parsed_changes = 0;
  long last_tmil = 0;
  long clocks = 0;
  const long kTenMillion = 10000000;
  input_file >> token;
  string last_time = "#0";
  bool new_clock = false;
  while (!input_file.eof()) {
    if (token == "$dumpvars" ||
        token == "$dumpon" ||
        token == "$dumpoff") {
      SkipToEndToken(input_file);
    } else if (token == "$dumpall") {
      parsed_changes += ParseDumpAll(input_file);
      ++clocks;

    } else if (token[0] == '#') {
      // Token contains a timestamp. For now, we do nothing with these.
      last_time = token;
      new_clock = true;
    } else {
      assert_b(false) {
        cout << "Unexpected token: " << token << " while processing values."
             << endl;
      }
    }
    long tmil = parsed_changes / kTenMillion;
    if (tmil != last_tmil) {
      cout << "Parsed " << tmil << "0M value changes. Current sim time: "
           << last_time << endl;
      last_tmil = tmil;
    }
    if (new_clock && clocks % 1000 == 0) {
      cout << "Parsed " << clocks << " clock cycles. Current sim time: "
           << last_time << endl;
      new_clock = false;
    }
    input_file >> token;
  }

  PrintStats();
}

void VcdParser::SkipToEndToken(ifstream& input_file) {
  string token;
  do {
    input_file >> token;
  } while (token != "$end");
}

void VcdParser::ParseScope(ifstream& input_file, const string& parent_scope) {
  string token;
  string instance_name;
  input_file >> token; // Consume scope type.
  if (token == "module") {
    input_file >> instance_name; // Consume module instance name.
    SkipToEndToken(input_file);
  } else {
    assert(false);
  }
  int parsed = 0;
  while (true) {
    assert(!input_file.eof());
    input_file >> token;
    if (token == "$upscope") {
      SkipToEndToken(input_file);
      return;
    } else if (token == "$var") {
      ParseVar(input_file);
      ++parsed;
    } else if (token == "$scope") {
      ParseScope(input_file, parent_scope + instance_name);
    } else {
      assert(false);
    }
    if (parsed % 1000 == 0) {
      cout << "Parsed " << parsed << " vars." << endl;
    }
  }
}

void VcdParser::ParseVar(ifstream& input_file) {
  string temp_token;

  string var_type;
  int size;
  string identifier_code;
  string reference_identifier;
  int bit_select = 0;

  input_file >> var_type; // Consume var type.
  input_file >> size; // Consume bitwidth of var.
  assert (size == 1); // TODO Support multi-bit nets.
  input_file >> identifier_code; // Consume short name.
  input_file >> reference_identifier; // Consume base net name.
  input_file >> temp_token; // Consume bit-select, range, or $end.

  // If it is a bit-select or range, append it to the net name.
  if (temp_token != "$end") {
    assert(temp_token[0] == '[');
    assert(temp_token.find(':') == string::npos); // TODO Support ranges.
    bit_select = strtod(
        temp_token.substr(1, temp_token.length() - 2).c_str(), nullptr);
    input_file >> temp_token; // Consume $end.
    assert(temp_token == "$end");
  }

  pair<string, int> identifier = make_pair(reference_identifier, bit_select);

  all_vcd_identifier_names_.insert(identifier);
  if (name_fixed_monitored_signals_.find(identifier) !=
      name_fixed_monitored_signals_.end()) {
    modified_signal_name_to_identifier_code_.insert(
        make_pair(identifier, identifier_code));
    identifier_code_to_values_.insert(
        make_pair(identifier_code, vector<char>()));
  }
}

long long VcdParser::ParseDumpAll(ifstream& input_file) {
  string token;
  input_file >> token;
  long long vals_processed = 0ULL;
  while (!input_file.eof() && (token != "$end")) {
    char val = token[0];
    assert_b(val == '0' || val == '1' || val == 'x' || val == 'X' ||
             val == 'z' || val == 'Z') {
      cout << "Unexpected value: " << val << " in value change: "
           << token << endl;
    }
    string identifier_code = token.substr(1, string::npos);
    if (monitored_identifier_codes_.find(identifier_code) !=
        monitored_identifier_codes_.end()) {
      auto it = identifier_code_to_values_.find(identifier_code);
      if (it != identifier_code_to_values_.end()) {
        it->second.push_back(val);
        ++vals_processed;
      }
    }
    input_file >> token;
  }
  return vals_processed;
}

void VcdParser::PrintStats() {
  cout << "------------------------------" << endl;
  cout << "SUMMARY" << endl << endl;
  cout << "Processed " << identifier_code_to_values_.size() << " nets." << endl;
  cout << "Processed " << identifier_code_to_values_.begin()->second.size()
       << " cycles per net." << endl;
  long total_changes = 0;
  for (auto name_val_vector_pair : identifier_code_to_values_) {
    total_changes += name_val_vector_pair.second.size();
  }
  cout << "Processed " << total_changes << " total value changes." << endl;
}

void VcdParser::CheckSignalsExistOrDie() {
  bool die = false;
  for (const string& raw_signal : raw_monitored_signals_) {
    pair<string, int> modified_signal = ConvertToVcdSignalName(raw_signal);
    if (modified_signal_name_to_identifier_code_.find(modified_signal) ==
        modified_signal_name_to_identifier_code_.end()) {
      die = true;
    }
  }
  if (die) {
    cout << "Registered signals:" << endl;
    for (const pair<string, int>& name : all_vcd_identifier_names_) {
      cout << name.first << " " << name.second << endl;
    }
  }
  assert(!die);
}

pair<string, int> VcdParser::ConvertToVcdSignalName(
    const string& raw_signal_name) {
  int bit_select = 0;
  // Assume all raw names end in a bit select.
  assert(raw_signal_name.back() == ']');
  size_t bit_select_start_pos = raw_signal_name.rfind('[');
  assert(bit_select_start_pos != string::npos);
  string bit_select_str =
      raw_signal_name.substr(bit_select_start_pos, string::npos);
  string identifier = raw_signal_name.substr(0, bit_select_start_pos);

  if (identifier.at(0) == '\\') {
    // Escaped identifier
    // Remove both the leading '\\' and the trailing space.
    identifier = identifier.substr(1, identifier.length() - 2);
  }

  // Convert everything but the opening '[' and the closing ']'.
  bit_select = strtod(
      bit_select_str.substr(1, bit_select_str.length() - 2).c_str(), nullptr);
  return make_pair(identifier, bit_select);
}

string VcdParser::StripLeadingEscapeChar(const string& str) {
  if (str.empty() || str[0] != '\\') {
    return str;
  }
  return str.substr(1, string::npos);
}

void VcdParser::WriteDumpVars(const string& signal_filename,
                              const string& output_filename,
                              const string& scope) {
  ifstream input_file(signal_filename);
  assert_b(input_file.is_open()) {
    cout << "Couldn't open file " << signal_filename << endl;
  }

  ofstream output_file(output_filename);
  assert_b(output_file.is_open()) {
    cout << "Couldn't open file " << output_filename << endl;
  }

  output_file << "$dumpvars(1";
  string signal;
  input_file >> signal;
  while (!input_file.eof()) {
    if (!scope.empty()) {
      signal = scope + "." + StripLeadingEscapeChar(signal);
    }
    output_file << ", " << signal;
    input_file >> signal;
  }
  output_file << ");" << endl;
}
