#pragma once
#include <alice/alice.hpp>
#include <stp/lutxx/decompose.hpp>
#include <utility>

namespace alice {
class lutxx_command : public command {
public:
  explicit lutxx_command(const environment::ptr &env)
      : command(env, "44-LUT / 66-LUT pair decomposition (disjoint first)") {
    add_option("-f,--function", truth_table, "hexadecimal truth table; use 0b prefix for binary", true);
    add_option("-x", lut_k, "target LUT size for pair decomposition (supported: 4 or 6)", true);
  }
protected:
  void execute() override {
    auto text = std::exchange(truth_table, {});
    const int k = std::exchange(lut_k, 6);
    try {
      if (!is_set("-x")) throw std::invalid_argument("provide -x 4 or -x 6");
      auto result = stp::lutxx::decompose(stp::lutxx::parse_table(text), k);
      if (!result.found) { env->out() << "[e] no " << k << k << "-LUT decomposition found\n"; return; }
      env->out() << "[i] " << result.method << " (K=" << k << ")\n";
      if (!result.my.empty()) print("MY", result.my, result.my_vars);
      print("MX", result.mx, result.mx_vars);
    } catch (const std::exception &e) { env->out() << "[e] " << e.what() << '\n'; }
  }
private:
  static std::string to_hex(const std::string &bits) {
    std::string padded((4 - bits.size() % 4) % 4, '0');
    padded += bits;
    std::string hex = "0x";
    for (size_t i = 0; i < padded.size(); i += 4) {
      unsigned value = 0;
      for (size_t j = 0; j < 4; ++j)
        value = value * 2 + static_cast<unsigned>(padded[i + j] - '0');
      hex += "0123456789abcdef"[value];
    }
    return hex;
  }
  void print(const std::string &name, const std::string &tt, const std::vector<int> &vars) {
    env->out() << "[i] " << name << " = " << to_hex(tt) << "  inputs (MSB->LSB):";
    for (int v : vars) env->out() << ' ' << (v == 0 ? "MY" : "x" + std::to_string(v));
    env->out() << '\n';
  }
  std::string truth_table;
  int lut_k = 6;
};
ALICE_ADD_COMMAND(lutxx, "Decomposition");
}


