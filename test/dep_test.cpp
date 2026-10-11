#include <stp/dep/dep.hpp>
#include <cassert>
#include <iostream>
int main() {
  assert(stp::dependency("1000", {"1000"}) == "10");
  assert(stp::dependency("1000", {"1000", "1000"}) == "1xx0");
  assert(stp::dependency("xxxx", {"1000"}) == "xx");
  assert(stp::dep_expression("(and a b)", {"b", "a"}, 4) == "1000");
  assert(stp::dep_expression("(imply a b)", {"b", "a"}, 4) == "1101");
  assert(stp::dep_expression("a", {"b", "a"}, 4) == "1010");
  bool failed = false;
  try { stp::dependency("1010", {"1000"}); } catch (const std::invalid_argument &) { failed = true; }
  assert(failed);
  // Exhaustively compare every two-input target and pair of divisors to
  // an independent grouping by Boolean values, including inconsistent groups.
  for (unsigned f=0; f<16; ++f) for (unsigned a=0; a<16; ++a) for (unsigned b=0; b<16; ++b) {
    auto table=[](unsigned v) { std::string s; for(int j=3;j>=0;--j) s += ((v>>j)&1)?'1':'0'; return s; };
    std::string expected(4,'x'); bool conflict=false;
    for (unsigned j=0;j<4;++j) {
      unsigned index=3-((((a>>j)&1)*2)+((b>>j)&1));
      char value=((f>>j)&1)?'1':'0';
      if(expected[index]!='x' && expected[index]!=value) conflict=true;
      expected[index]=value;
    }
    try { auto actual=stp::dependency(table(f),{table(a),table(b)}); assert(!conflict); assert(actual==expected); }
    catch(const std::invalid_argument &) { assert(conflict); }
  }
  std::cout << "4096 exhaustive cases and expression/don't-care checks passed\n";
}

