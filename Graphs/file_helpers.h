/*
 * file_helpers.h
 *
 *  Created on: Dec 17, 2014
 *      Author: gregerso
 */

#ifndef FILE_HELPERS_H_
#define FILE_HELPERS_H_

#include <cstdio>

#include <istream>
#include <sstream>
#include <string>

namespace fhelp {
// Methods to allow code reuse regardless of whether input is istream or
// FILE.
inline void SeekToPosition(std::istream& in, std::streampos pos) {
  in.seekg(pos);
}
inline void SeekToPosition(FILE* in, off_t pos) {
  fseeko(in, pos, SEEK_SET);
}

inline void SeekToEnd(std::istream& in) {
  in.seekg(0, std::ios::end);
}
inline void SeekToEnd(FILE* in) {
  fseeko(in, 0, SEEK_END);
}
inline std::streampos GetPosition(std::istream& in) {
  return in.tellg();
}
inline off_t GetPosition(FILE* in) {
  return ftello(in);
}

inline char GetChar(std::istream& in) {
  return (char)in.get();
}
inline char GetChar(FILE* in) {
  return (char)fgetc(in);
}

inline void UngetChar(char, std::istream& in) {
  in.unget();
}
inline void UngetChar(char c, FILE* in) {
  ungetc(c, in);
}

inline bool IsEof(std::istream& in) {
  return in.eof();
}
inline bool IsEof(FILE* in) {
  return feof(in);
}

inline bool IsError(std::istream& in) {
  return in.bad();
}
inline bool IsError(FILE* in) {
  return (in == nullptr) || ferror(in);
}

inline char PeekChar(std::istream& in) {
  return (char)in.peek();
}
inline char PeekChar(FILE* in) {
  char c = (char)fgetc(in);
  ungetc(c, in);
  return c;
}

inline void GetLine(std::istream& in, char* buffer, int length) {
  in.getline(buffer, length);
}
inline void GetLine(FILE* in, char* buffer, int length) {
  char* res = fgets(buffer, length, in);
  if (res == nullptr) {
    buffer[0] = '\0';
  }
}

template <typename T>
inline std::string PeekNextLine(T& in) {
  const int MAX_LINE_CHARS = 4096;
  char line_buffer[MAX_LINE_CHARS];
  const auto initial_pos = GetPosition(in);
  GetLine(in, line_buffer, MAX_LINE_CHARS - 1);
  SeekToPosition(in, initial_pos);
  return std::string(line_buffer);
}

template <typename T>
inline std::string PeekNextNonEmptyLine(T& in) {
  const auto initial_pos = GetPosition(in);
  char c = GetChar(in);
  while (isspace(c) && c != EOF) {
    c = GetChar(in);
  }
  if (EOF != c) {
    UngetChar(c, in);
  }
  std::string line = PeekNextLine(in);
  SeekToPosition(in, initial_pos);
  return line;
}

template <typename T>
inline std::string PrintNextLineInfo(T& in) {
  std::string stream_line = PeekNextLine(in);
  std::stringstream ss;
  ss << "Next stream line of size " << stream_line.size()
      << " at offset " << GetPosition(in) << ":\n"
      << stream_line;
  return ss.str();
}

}  // namespace fhelp

#endif /* FILE_HELPERS_H_ */
