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
// Deterministic, lock-safe, category-diverse extreme randomizer.
// Low degree selects a few small sounds. High degree activates 50+ strong
// transformations selected from all ten banks, with seed-driven anchors.
inline std::array<float,100> generatePatch(uint64_t seed, float degree,
                                           const std::array<float,100>& previous,
                                           const std::array<bool,100>& locked,
                                           const std::array<bool,100>& available) noexcept {
    SplitMix64 rng(seed);
    auto out=previous;
    degree=std::isfinite(degree)?std::clamp(degree,0.f,1.f):.5f;
    const float density=.10f+.75f*degree;
    const float minRaw=.08f+.72f*degree;
    const float topRaw=.32f+.68f*degree;
    static constexpr int anchors[10][5]={
        {0,2,4,7,9},{0,3,4,7,8},{0,1,2,6,7},{0,1,2,6,9},
        {0,2,3,4,6},{0,1,2,4,6},{0,1,4,6,8},{2,3,5,7,8},
        {0,2,4,7,9},{0,1,5,8,9}
    };
    for(int bank=0;bank<10;++bank){
        const int candidate=bank*10+anchors[bank][size_t(rng.next()%5)];
        int order[10]={0,1,2,3,4,5,6,7,8,9};
        for(int j=9;j>0;--j)std::swap(order[j],order[size_t(rng.next()%uint64_t(j+1))]);
        const int maximum=std::clamp(int(std::floor(.3f+4.65f*degree+1.6f*rng.unit())),0,7);
        int active=0;
        for(int k=0;k<10;++k){
            const size_t i=size_t(bank*10+order[k]);
            const float pick=rng.unit(),depth=rng.unit();
            if(locked[i] || !available[i])continue;
            const bool selected=pick<density && active<maximum;
            const float raw=minRaw+(topRaw-minRaw)*std::pow(depth,degree<.4f?1.8f:.72f);
            out[i]=selected?std::clamp(raw,0.f,1.f):0.f;
            if(selected)++active;
        }
        // One intense signature sound per available bank at high DEGREE.
        if(degree>=.62f && available[size_t(candidate)] && !locked[size_t(candidate)])
            out[size_t(candidate)]=std::max(out[size_t(candidate)],.67f+.33f*degree);
    }
    return out;
}
}
