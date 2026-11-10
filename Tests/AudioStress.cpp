#include "../Source/Core/Engine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#ifndef MCBOY_STRESS_PASSES
#define MCBOY_STRESS_PASSES 200
#endif

int main(){
    mcboy::SplitMix64 rng(0x1234567);
    for(double sampleRate:{44100.,48000.,88200.,96000.,192000.}){
        mcboy::Engine e;e.prepare(sampleRate);
        mcboy::CoreControls c;
        std::array<bool,100> locks{};
        for(int pass=0;pass<MCBOY_STRESS_PASSES;++pass){
            c.seed=mcboy::nextPatchSeed(c.seed);
            c.degree=rng.unit();c.mix=rng.unit();
            c.raw=mcboy::generatePatch(c.seed,c.degree,c.raw,locks,mcboy::implemented);
            c.outputDb=-10.f+16.f*rng.unit();c.bypass=(pass%17==0);c.guard=true;
            e.setControls(c);
            const int n=1+(pass*43)%2048;
            std::vector<float> l(static_cast<size_t>(n),0.f),r(static_cast<size_t>(n),0.f);
            for(int j=0;j<n;++j){
                l[size_t(j)]=rng.unit()*2.f-1.f;
                r[size_t(j)]=rng.unit()*2.f-1.f;
            }
            e.process(l.data(),r.data(),n,pass%2?90.:140.);
            for(int j=0;j<n;++j){
                if(!std::isfinite(l[size_t(j)]) || !std::isfinite(r[size_t(j)]))return 2;
                if(std::abs(l[size_t(j)])>16.f || std::abs(r[size_t(j)])>16.f)return 3;
            }
        }
        // Silence and no noise-generating effects: output stays silent after reset.
        e.reset();c.raw.fill(0.f);c.degree=0.f;c.bypass=false;c.outputDb=0.f;
        e.setControls(c);
        std::vector<float> silence(1024,0.f);
        e.process(silence.data(),nullptr,int(silence.size()));
        for(auto x:silence)if(x!=0.f)return 4;
        std::cout << "PASS stress / mono silence @ " << sampleRate << " Hz\n";
    }
}
