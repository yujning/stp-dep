#pragma once
#include "66lut_bidec.hpp"
#include "66lut_dsd.hpp"
#include <stdexcept>

namespace stp::lutxx {
struct Result {
  bool found = false;
  std::string method, my, mx;
  std::vector<int> my_vars, mx_vars; // MSB to LSB; 0 denotes MY.
};

inline std::string parse_table(std::string text) {
  std::string bits;
  if (text.rfind("0b", 0) == 0 || text.rfind("0B", 0) == 0) {
    bits = text.substr(2);
    if (bits.find_first_not_of("01") != std::string::npos)
      throw std::invalid_argument("binary truth table must contain only 0 and 1");
  } else {
    if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) text.erase(0, 2);
    if (text.size() > (1u << 18)) throw std::invalid_argument("truth table exceeds 20 variables");
    for (char c : text) {
      auto value = std::string("0123456789abcdef").find(c);
      if (value == std::string::npos) value = std::string("0123456789ABCDEF").find(c);
      if (value == std::string::npos) throw std::invalid_argument("invalid hexadecimal truth table");
      for (int j = 3; j >= 0; --j) bits += ((value >> j) & 1) ? '1' : '0';
    }
  }
  if (bits.empty() || bits.size() > (1u << 20) || (bits.size() & (bits.size() - 1)))
    throw std::invalid_argument("truth table width must be a power of two, at most 2^20");
  return bits;
}

inline Result decompose(const std::string &bits, int k) {
  if (k != 4 && k != 6) throw std::invalid_argument("-x only supports 4 or 6");
  if (bits.empty() || bits.size() > (1u << 20) || (bits.size() & (bits.size()-1)) ||
      bits.find_first_not_of("01") != std::string::npos)
    throw std::invalid_argument("invalid binary truth table");
  struct Restore { int old; ~Restore() { LUT_PAIR_FANIN_LIMIT = old; } } restore{LUT_PAIR_FANIN_LIMIT};
  LUT_PAIR_FANIN_LIMIT = k;
  unsigned n = 0;
  while ((size_t{1} << n) < bits.size()) ++n;
  std::vector<int> order;
  // Remove variables whose paired cofactors are identical, retaining MSB order.
  for (int v = static_cast<int>(n); v > 0; --v) {
    bool active = false;
    for (size_t i = 0; i < bits.size(); ++i)
      if (bits[i] != bits[i ^ (size_t{1} << (v-1))]) { active = true; break; }
    if (active) order.push_back(v);
  }
  std::string table(size_t{1} << order.size(), '0');
  for (size_t i = 0; i < table.size(); ++i) {
    size_t old = 0;
    for (size_t j = 0; j < order.size(); ++j)
      old |= ((i >> (order.size()-1-j)) & 1) << (order[j]-1);
    table[i] = bits[old];
  }
  if (order.size() <= static_cast<size_t>(k)) return {true, "single LUT", {}, table, {}, order};
  if (order.size() > static_cast<size_t>(2*k-1)) return {};
  auto dsd = run_66lut_dsd_by_mx_subset(table, order);
  if (dsd.found) {
    auto mx_order = dsd.mx_vars_msb2lsb;
    mx_order.insert(mx_order.begin(), 0);
    return {true, "disjoint decomposition", dsd.My, dsd.Mx, dsd.my_vars_msb2lsb, mx_order};
  }
  for (auto [x,y,z] : enumerate_xyz(static_cast<int>(order.size()))) {
    for (const auto &[a,b,c] : enumerate_variable_partitions(order,x,y,z)) {
      auto reordered = a;
      reordered.insert(reordered.end(),b.begin(),b.end());
      reordered.insert(reordered.end(),c.begin(),c.end());
      auto mf = reorder_tt_by_var_order(table, static_cast<int>(order.size()), reordered, order);
      auto mxy = compute_MXYX_from_MF(mf,x,y,z);
      std::string mx, symbolic, my;
      if (!solve_MX_MY_from_MXY(mxy,x,y,z,mx,symbolic,my)) continue;
      auto my_order = a;
      my_order.insert(my_order.end(),b.begin(),b.end());
      std::vector<int> mx_order{0};
      mx_order.insert(mx_order.end(),b.begin(),b.end());
      mx_order.insert(mx_order.end(),c.begin(),c.end());
      // Use merged MX constraints; the legacy symbolic string can retain
      // x in positions constrained by later blocks.
      return {true, "bi-decomposition", my, mx, my_order, mx_order};
    }
  }
  return {};
}
}
