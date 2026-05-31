#pragma once

#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mce {

class CsvLogger {
public:
  CsvLogger() = default;

  explicit CsvLogger(const std::string& path, bool enabled = true, bool append = false)
      : enabled_(enabled), stream_(path, append ? std::ios::app : std::ios::trunc) {
    if (enabled_ && !stream_) {
      throw std::runtime_error("failed to open csv log file: " + path);
    }
  }

  bool enabled() const { return enabled_; }

  void write_header(std::initializer_list<std::string_view> columns) { write_row(columns); }

  void write_row(std::initializer_list<std::string_view> fields) {
    if (!enabled_) {
      return;
    }
    write_csv_line(fields.begin(), fields.end());
  }

  void write_row(const std::vector<std::string>& fields) {
    if (!enabled_) {
      return;
    }
    write_csv_line(fields.begin(), fields.end());
  }

  void flush() {
    if (enabled_) {
      stream_.flush();
    }
  }

private:
  template <typename Iter>
  void write_csv_line(Iter begin, Iter end) {
    bool first = true;
    for (auto it = begin; it != end; ++it) {
      if (!first) {
        stream_ << ',';
      }
      first = false;
      stream_ << escape(*it);
    }
    stream_ << '\n';
  }

  static std::string escape(std::string_view value) {
    const bool needs_quotes = value.find_first_of(",\"\n\r") != std::string_view::npos;
    if (!needs_quotes) {
      return std::string(value);
    }

    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (char ch : value) {
      if (ch == '"') {
        out.push_back('"');
      }
      out.push_back(ch);
    }
    out.push_back('"');
    return out;
  }

  bool enabled_ = false;
  std::ofstream stream_;
};

}  // namespace mce