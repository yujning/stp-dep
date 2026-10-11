// Copyright (c) 2023-2026 The STP Authors
// SPDX-License-Identifier: MIT
#pragma once
#include <alice/alice.hpp>
#include <stp/dep/dep.hpp>
#include <utility>

namespace alice {
class dep_command : public command {
public:
  explicit dep_command(const environment::ptr &env) : command(env, "Compute dependency using fast STP column row indices") {
    add_option("--function,-f", truth_table, "binary target (0/1/x), or 0x-prefixed hexadecimal", true);
    add_option("--divisors,-d", divisors, "semicolon-separated binary or 0x-prefixed hexadecimal truth tables", true);
  }
protected:
  void execute() override {
    // Move state out immediately so successive interactive calls start cleanly.
    const auto target_text = std::exchange(truth_table, {});
    const auto divisor_text = std::exchange(divisors, {});
    try {
      const auto f = stp::dep_table(target_text, true);
      auto specs = split(divisor_text, ';');
      std::vector<std::string> tables;
      for (const auto &spec : specs) tables.push_back(stp::dep_table(spec));
      const auto h = stp::dependency(f, tables);
      env->out() << "[i] dependency truth table (11...1 -> 00...0): " << h << '\n';
    } catch (const std::exception &e) { env->out() << "[e] " << e.what() << '\n'; }
  }
private:
  static std::vector<std::string> split(const std::string &text, char separator) {
    std::vector<std::string> parts;
    size_t start = 0;
    do {
      const auto end = text.find(separator, start);
      auto part = text.substr(start, end == std::string::npos ? end : end - start);
      const auto first = part.find_first_not_of(" \t\r\n");
      if (first == std::string::npos) throw std::invalid_argument("empty input or divisor");
      part = part.substr(first, part.find_last_not_of(" \t\r\n") - first + 1);
      parts.push_back(part);
      if (end == std::string::npos) break;
      start = end + 1;
    } while (true);
    return parts;
  }
  std::string truth_table, divisors;
};
ALICE_ADD_COMMAND(dep, "Decomposition");
} // namespace alice

