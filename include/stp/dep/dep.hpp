#pragma once
#include <stp/io/logic_expr_parser.hpp>
#include <stdexcept>

namespace stp {
// Tables are printed from assignment 11...1 to 00...0, like STP LUTs.
inline std::string dep_table(std::string s, bool target = false) {
  if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
    std::string bits;
    for (size_t i = 2; i < s.size(); ++i) {
      const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
      const auto p = std::string("0123456789abcdef").find(c);
      if (p == std::string::npos) throw std::invalid_argument("invalid hexadecimal table");
      for (int j = 3; j >= 0; --j) bits += ((p >> j) & 1) ? '1' : '0';
    }
    s = bits;
  } else if (s.rfind("0b", 0) == 0) s.erase(0, 2);
  if (s.empty() || s.size() > (1u << 20) || (s.size() & (s.size()-1)))
    throw std::invalid_argument("table length must be a power of two, at most 2^20");
  for (char &c : s) {
    if (target && (c == 'X' || c == '-')) c = 'x';
    if (c != '0' && c != '1' && !(target && c == 'x'))
      throw std::invalid_argument("table contains invalid values");
  }
  return s;
}

inline std::string dep_expression(const std::string &expr,
                                  const std::vector<std::string> &names, size_t count) {
  CircuitGraph graph;
  LogicExprParser parser;
  std::string error;
  if (!parser.parse(expr, graph, error)) throw std::invalid_argument(error);
  std::vector<size_t> shifts;
  for (auto id : graph.get_inputs()) {
    const auto it = std::find(names.begin(), names.end(), graph.get_line(id).name);
    if (it == names.end()) throw std::invalid_argument("unknown divisor variable: " + graph.get_line(id).name);
    shifts.push_back(names.size() - 1 - static_cast<size_t>(it - names.begin()));
  }
  std::string table(count, '0');
  std::vector<unsigned> values(graph.get_lines().size());
  for (size_t k = 0; k < count; ++k) {
    const size_t assignment = count - 1 - k;
    for (size_t j = 0; j < shifts.size(); ++j)
      values[graph.get_inputs()[j]] = (assignment >> shifts[j]) & 1;
    for (const auto &gate : graph.get_gates()) {
      size_t column = 0;
      for (auto it = gate.get_inputs().begin(); it != gate.get_inputs().end(); ++it)
        column = column * 2 + (1 - values[*it]);
      values[gate.get_output()] = 1 - gate.get_type()(column + 1);
    }
    table[k] = static_cast<char>('0' + values[graph.get_outputs().front()]);
  }
  return table;
}

inline std::string dependency(const std::string &target,
                              const std::vector<std::string> &divisors) {
  const auto f = dep_table(target, true);
  if (divisors.empty() || divisors.size() > 20)
    throw std::invalid_argument("provide between 1 and 20 divisors");
  std::vector<std::string> tables;
  for (const auto &d : divisors) {
    tables.push_back(dep_table(d));
    if (tables.back().size() != f.size()) throw std::invalid_argument("divisor and target widths differ");
  }
  std::string result(size_t{1} << tables.size(), 'x');
  for (size_t k = 0; k < f.size(); ++k) {
    size_t row = 0;
    // Zero-based form of gamma=(alpha-1)*r2+beta, with r2=2.
    for (const auto &d : tables) row = row * 2 + (d[k] == '0');
    if (f[k] == 'x') continue;
    if (result[row] != 'x' && result[row] != f[k])
      throw std::invalid_argument("no dependency exists: conflicting values at dependency column " + std::to_string(row + 1));
    result[row] = f[k];
  }
  return result;
}
} // namespace stp

