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
  string token;
  int width;
  string short_name;
  string net_name;

  input_file >> token; // Consume var type.
  input_file >> width; // Consume bitwidth of var.
  assert (width == 1); // TODO Support multi-bit nets.
  input_file >> short_name; // Consume short name.
  input_file >> net_name; // Consume base net name.
  input_file >> token; // Consume bit-select, range, or $end.

  // If it is a bit-select or range, append it to the net name.
  if (token != "$end") {
    assert(token[0] == '[');
    assert(token.find(':') == string::npos); // TODO Support ranges.
    // Append bit-select to net name.
    net_name.append(token);
    input_file >> token; // Consume $end.
  }

  all_var_names_.insert(net_name);
  if (monitored_signals_.find(net_name) == monitored_signals_.end()) {
    SkipToEndToken(input_file);
    return;
  }
  
  net_name_to_short_name_.insert(make_pair(net_name, short_name));
  short_name_to_values_.insert(make_pair(short_name, vector<char>()));
}

int VcdParser::ParseDumpAll(ifstream& input_file) {
  string token;
  input_file >> token;
  int vals_processed = 0;
  while (!input_file.eof() && (token != "$end")) {
    char val = token[0];
    assert_b(val == '0' || val == '1' || val == 'x' || val == 'X' ||
             val == 'z' || val == 'Z') {
      cout << "Unexpected value: " << val << " in value change: "
           << token << endl;
    }
    string short_name = token.substr(1, string::npos);
    auto it = short_name_to_values_.find(short_name);
    if (it != short_name_to_values_.end()) {
      it->second.push_back(val);
      ++vals_processed;
    }
    input_file >> token;
  }
  return vals_processed;
}

void VcdParser::PrintStats() {
  cout << "------------------------------" << endl;
  cout << "SUMMARY" << endl << endl;
  cout << "Processed " << short_name_to_values_.size() << " nets." << endl;
  cout << "Processed " << short_name_to_values_.begin()->second.size()
       << " cycles per net." << endl;
  long total_changes = 0;
  for (auto name_val_vector_pair : short_name_to_values_) {
    total_changes += name_val_vector_pair.second.size();
  }
  cout << "Processed " << total_changes << " total value changes." << endl;
}

void VcdParser::CheckSignalsExistOrDie() {
  bool die = false;
  for (const string& raw_signal : raw_signals_) {
    string signal = StripLeadingEscapeChar(raw_signal);
    bool not_found = false;
    if (net_name_to_short_name_.find(signal) == net_name_to_short_name_.end()) {
      // Maybe the signal is not found because it is written as a single bit and
      // the VCD specifies a bus. Detect this case:
      size_t index_start = signal.rfind('[');
      if (index_start != string::npos) {
        signal = signal.substr(0, index_start);
        if (net_name_to_short_name_.find(signal) !=
            net_name_to_short_name_.end()) {
          cout << "Warning: Got bus match rather than signal match for "
               << raw_signal << " and " << signal << endl;
        }
      }
    }
    die = not_found && die;
  }
  if (die) {
    cout << "Registered signals:" << endl;
    for (const string& name : all_var_names_) {
      cout << name << endl;
    }
  }
  assert(!die);
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
