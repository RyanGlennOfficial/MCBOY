#pragma once
// Original McBoy wet-only intensity stage. NO dry input is provided.
// Every control consumes the already DEGREE-limited effective value.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>
#include "Randomizer.h"

namespace mcboy {
class IntensityStage {
public:
    void prepare(double rate) {
        sr=float(std::clamp(rate,8000.,384000.));
        for(auto& c:history)c.assign(size_t(sr*2.f)+8,0.f);
        reset();
    }
    void reset() noexcept {
        for(auto& c:history)std::fill(c.begin(),c.end(),0.f);
        tone.fill(0.f);pos=0;gate=1.f;
    }
    void setSeed(uint64_t seed) noexcept {
        patchSeed=seed;
        SplitMix64 rng(seed ^ UINT64_C(0xA17A5EBEB00BCAFE));
        for(auto& x:accent)x=.55f+1.7f*rng.unit();
    }
    float categoryGain(int category,float amount) const noexcept {
        static constexpr float focus[10]={1.65f,1.65f,.95f,2.8f,1.6f,
                                           1.05f,2.2f,1.1f,.75f,1.4f};
        const size_t id=size_t(std::clamp(category,0,9));
        return (1.f+(focus[id]-1.f)*std::clamp(amount*1.4f,0.f,1.f))*
               (1.f+(accent[id]-1.f)*std::clamp(amount*1.2f,0.f,1.f));
    }
    void inactive() noexcept {gate=1.f;}
    std::array<float,2> process(float l,float r,uint64_t tick,double bpm,
                                const std::array<float,100>& effective) noexcept {
        const auto at=[&](int fx) noexcept {return effective[size_t(fx-1)];};
        const auto peak=[&](std::initializer_list<int> ids) noexcept {
            float m=0.f;for(int i:ids)m=std::max(m,at(i));return m;
        };
        const float time=peak({3,4,5,7,8,10,62,65,69,70,76,79});
        const float wobble=peak({27,37,40,73,74,76});
        const float pitch=peak({31,32,33,34,37,38,40});
        const float damage=peak({41,42,43,44,45,48,49,50,99});
        const float rhythm=peak({5,61,62,66,67,68,69,70,78,79,99,100});
        const float filter=peak({51,52,53,55,57,77,80});
        const float space=peak({11,12,14,15,17,19,20});
        // All nonlinear maps have exact neutral at zero effective amount.
        const float timeDepth=std::clamp((time-.23f)*1.35f,0.f,.95f);
        const float wobbleDepth=std::clamp((wobble-.22f)*1.3f,0.f,.88f);
        const float sec=float(double(tick)/sr);
        const float original[2]={l,r};
        std::array<float,2> result{l,r};
        for(int c=0;c<2;++c){
            const float wobbleSamples=sr*(.007f+.013f*(.5f+.5f*
                std::sin(6.283185307f*(1.7f+11.f*wobble)*sec+float(c)*.4f)));
            const float timeSamples=sr*(.023f+.21f*time+
                .09f*time*std::sin(6.283185307f*.29f*sec+float(c)));
            const float recent=read(history[size_t(c)],wobbleSamples);
            const float distant=read(history[size_t(c)],timeSamples);
            result[size_t(c)]=(1.f-timeDepth)*((1.f-wobbleDepth)*result[size_t(c)]
                               +wobbleDepth*recent)+timeDepth*distant;
        }
        // A rhythmic knob chops the whole effected sound, not a faint return.
        const double beats=double(tick)*bpm/(60.0*double(sr));
        const double steps=beats*4.;
        const int phaseShift=int(patchSeed&15ULL);
        const int step=(int(std::floor(steps))+phaseShift)&15;
        const float fraction=float(steps-std::floor(steps));
        static constexpr float pattern[16]={1.f,0.f,.75f,0.f,1.f,.12f,0.f,1.f,
                                            .7f,0.f,1.f,0.f,.15f,1.f,0.f,.5f};
        const float depth=std::clamp((rhythm-.2f)*1.4f,0.f,.98f);
        const float selected=(pattern[step]>.5f&&fraction<.82f)?pattern[step]:.03f;
        const float targetGate=1.f-depth*(1.f-selected);
        gate+=std::min(1.f,1.f/(sr*.0018f))*(targetGate-gate);
        for(int c=0;c<2;++c){
            float v=result[size_t(c)]*gate;
            if(damage>.15f){
                const float drive=1.f+36.f*std::pow(damage,2.f);
                const float clipped=std::tanh(v*drive)*.78f;
                v+=std::clamp((damage-.15f)*1.2f,0.f,1.f)*(clipped-v);
            }
            if(filter>.22f){
                const float cutoff=std::max(120.f,12500.f*std::pow(1.f-filter,2.2f));
                const float coeff=std::exp(-6.283185307f*cutoff/sr);
                tone[size_t(c)]=coeff*tone[size_t(c)]+(1.f-coeff)*v;
                v+=std::clamp((filter-.22f)*1.3f,0.f,.97f)*(tone[size_t(c)]-v);
            }
            v*=1.f+.52f*space+.43f*pitch;
            result[size_t(c)]=std::isfinite(v)?std::clamp(v,-16.f,16.f):0.f;
            history[size_t(c)][pos]=std::isfinite(original[c])?
                       std::clamp(original[c],-16.f,16.f):0.f;
        }
        pos=(pos+1)%history[0].size();
        return result;
    }
private:
    float read(const std::vector<float>& c,float samples) const noexcept {
        samples=std::clamp(samples,1.f,float(c.size()-3));
        const auto whole=size_t(samples);
        const auto frac=samples-float(whole);
        const auto p=(pos+c.size()-whole)%c.size();
        const auto q=(p+c.size()-1)%c.size();
        return c[p]*(1.f-frac)+c[q]*frac;
    }
    float sr=48000.f,gate=1.f;
    size_t pos=0;
    uint64_t patchSeed=0;
    std::array<float,10> accent{};
    std::array<float,2> tone{};
    std::array<std::vector<float>,2> history;
};
} // namespace mcboy
