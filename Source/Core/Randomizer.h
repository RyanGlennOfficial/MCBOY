#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mcboy {
struct SplitMix64 {
    uint64_t state;
    explicit SplitMix64(uint64_t seed) : state(seed) {}
    uint64_t next() noexcept {
        uint64_t z = (state += UINT64_C(0x9e3779b97f4a7c15));
        z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
        return z ^ (z >> 31);
    }
    float unit() noexcept { return float((next() >> 40) * (1.0 / 16777216.0)); }
};
inline uint64_t nextPatchSeed(uint64_t prior) noexcept {
    SplitMix64 s(prior);
    return s.next();
}
// v1: At most four active modules in each bank. Random output is reproducible
// given seed, degree and lock state; inactive or locked values are preserved.
inline std::array<float,100> generatePatch(uint64_t seed, float degree,
                                           const std::array<float,100>& previous,
                                           const std::array<bool,100>& locked,
                                           const std::array<bool,100>& available) noexcept {
    SplitMix64 rng(seed);
    auto out = previous;
    degree = std::clamp(degree, 0.0f, 1.0f);
    const float density = 0.12f + 0.70f * degree;
    for (int bank=0; bank<10; ++bank) {
        int active = 0;
        for (int j=0; j<10; ++j) {
            const auto i = size_t(bank*10+j);
            // Always draw PRNG numbers, including locked slots, to preserve a
            // stable sequence when lock configuration changes.
            const auto select = rng.unit(), depth = rng.unit();
            if (locked[i] || !available[i]) continue;
            const bool enabled = select < density && active < 4;
            out[i] = enabled ? std::clamp(std::pow(depth, (degree < .4f ? 2.2f : 1.1f)),0.f,1.f) : 0.f;
            if (enabled) ++active;
        }
    }
    return out;
}
}
