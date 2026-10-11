#pragma once
// Pair-decomposition routines ported from my_s_t_p.
namespace stp::lutxx {
inline int LUT_PAIR_FANIN_LIMIT = 6;
inline int mx_lut_fanin_limit() { return LUT_PAIR_FANIN_LIMIT - 1; }
}
