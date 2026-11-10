#include "../Source/Core/Engine.h"
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>

// A deterministic 2-fixture test of whether every numbered knob is audible
// AND whether any two effect outputs appear identical on these fixtures.
// Not a proof of perceptual distinction on real music or quality of emulation.
int main(int argc, char** argv) {
    constexpr int n=24000;
    std::array<std::vector<float>,100> sounds;
    std::array<float,100> vsDry{};
    std::vector<float> dryL(n),dryR(n);
    for(int i=0;i<n;++i){
        float t=float(i);
        const float transient=(i%4011)<70 ? .48f*std::exp(-float(i%4011)/20.f) : 0.f;
        dryL[i]=.45f*std::sin(t*.031f)+.14f*std::sin(t*.143f)+transient;
        dryR[i]=.42f*std::cos(t*.043f)+.11f*std::cos(t*.211f)-transient*.6f;
    }
    std::vector<float> l,r;
    mcboy::Engine engine;
    engine.prepare(48000.);
    mcboy::CoreControls c;
    c.degree=1.f;c.mix=1.f;c.guard=false;
    for(int fx=0;fx<100;++fx){
        engine.reset();c.raw.fill(0);c.raw[size_t(fx)]=1.f;engine.setControls(c);
        l=dryL;r=dryR;engine.process(l.data(),r.data(),n);
        sounds[size_t(fx)].resize(size_t(n)*2);
        double ds=0;
        for(int i=0;i<n;++i){
            const float a=l[size_t(i)],b=r[size_t(i)];
            if(!std::isfinite(a)||!std::isfinite(b))return 20;
            sounds[size_t(fx)][size_t(i)]=a;
            sounds[size_t(fx)][size_t(i+n)]=b;
            ds+=double(a-dryL[size_t(i)])*double(a-dryL[size_t(i)]);
            ds+=double(b-dryR[size_t(i)])*double(b-dryR[size_t(i)]);
        }
        vsDry[size_t(fx)]=float(std::sqrt(ds/(2*n)));
        if(vsDry[size_t(fx)]<1.e-6f){std::cerr<<"Silent effect "<<fx+1<<"\n";return 10;}
    }
    std::array<float,100> closest{};
    std::array<int,100> neighbor{};
    closest.fill(std::numeric_limits<float>::max());
    for(int i=0;i<100;++i) for(int j=i+1;j<100;++j){
        double d=0;
        const auto& x=sounds[size_t(i)];const auto& y=sounds[size_t(j)];
        for(size_t k=0;k<x.size();++k){const double delta=double(x[k])-double(y[k]);d+=delta*delta;}
        float difference=float(std::sqrt(d/double(x.size())));
        if(difference<closest[size_t(i)]){closest[size_t(i)]=difference;neighbor[size_t(i)]=j+1;}
        if(difference<closest[size_t(j)]){closest[size_t(j)]=difference;neighbor[size_t(j)]=i+1;}
    }
    if(argc>1){
        std::ofstream f(argv[1]);
        f<<"id,name,rms_from_dry,closest_fx,closest_rms_difference,fixture\n";
        for(int i=0;i<100;++i){
            f<<"fx";f.width(3);f.fill('0');f<<i+1;
            f<<",\""<<mcboy::effectNames[size_t(i)]<<"\","<<vsDry[size_t(i)]<<",fx";
            f.width(3);f.fill('0');f<<neighbor[size_t(i)]<<","<<closest[size_t(i)]<<",multitone+transient stereo 48kHz 0.5sec\n";
        }
    }
    float smallest=*std::min_element(closest.begin(),closest.end());
    std::cout<<"All 100 controls non-neutral; closest pair RMS separation "<<smallest<<"\n";
    if(smallest<1.e-6f){std::cerr<<"Two controls produce indistinguishable test outputs\n";return 11;}
    return 0;
}
