#include "../Source/Core/Engine.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <set>

int main(){
    using namespace mcboy;
    static_assert(effectNames.size()==100);
    static_assert(implemented.size()==100);
    const int N=24000;
    Engine engine;engine.prepare(48000.);
    CoreControls c;c.degree=0;c.mix=1;c.guard=false;
    std::vector<float> ref(N),refR(N);
    for(int i=0;i<N;++i){ref[i]=.62f*std::sin(float(i)*.034f)+.27f*std::sin(float(i)*.089f);refR[i]=.57f*std::cos(float(i)*.061f);}
    auto l=ref,r=refR;
    for(auto& a:c.raw)a=1.f;
    engine.setControls(c);engine.process(l.data(),r.data(),N);
    for(int i=0;i<N;++i){if(std::abs(l[i]-ref[i])>1e-6f || std::abs(r[i]-refR[i])>1e-6f) {std::cerr<<"degree zero failed "<<i<<' '<<l[i]<<' '<<ref[i]<<'\n';return 1;}}
    std::cout << "PASS: DEGREE zero is dry (at guard OFF, unity output)\n";
    // At 100% MIX, the undelayed original impulse must NOT pass through.
    // Exercise the exact bug reported by users with the Slap delay alone.
    c.raw.fill(0.f);c.raw[0]=1.f;c.degree=1.f;c.mix=1.f;c.guard=false;
    engine.reset();engine.setControls(c);
    std::vector<float> warm(24000,0.f),warmR(24000,0.f);
    engine.process(warm.data(),warmR.data(),int(warm.size()));
    l.assign(N,0.f);r.assign(N,0.f);l[0]=.4f;r[0]=.4f;
    engine.process(l.data(),r.data(),N);
    for(int i=0;i<1000;++i)if(std::abs(l[size_t(i)])>1e-4f || std::abs(r[size_t(i)])>1e-4f){
        std::cerr<<"FAIL: dry input leaks through 100% wet at sample "<<i<<" level "<<l[size_t(i)]<<"\n";return 16;
    }
    float delayedPeak=0.f;
    for(int i=2500;i<10000;++i)delayedPeak=std::max(delayedPeak,std::abs(l[size_t(i)]));
    if(delayedPeak<.08f){std::cerr<<"FAIL: wet delay disappeared at 100% mix\n";return 17;}
    std::cout<<"PASS: MIX=100% suppresses direct dry impulse and retains audible wet delay\n";
    engine.reset();c.mix=0.f;engine.setControls(c);
    l.assign(N,0.f);r.assign(N,0.f);l[0]=.4f;r[0]=.4f;
    engine.process(l.data(),r.data(),N);
    if(std::abs(l[0]-.4f)>1e-5f || std::abs(r[0]-.4f)>1e-5f)return 18;
    std::cout<<"PASS: MIX=0% preserves dry impulse\n";
    int measured=0;
    for(int fx=0;fx<100;++fx)if(implemented[size_t(fx)]){
        engine.reset();c.raw.fill(0.f);c.raw[size_t(fx)]=1.f;c.degree=1.f;c.mix=1.f;c.guard=false;
        engine.setControls(c);l=ref;r=refR;engine.process(l.data(),r.data(),N);
        double diff=0;
        for(int i=0;i<N;++i){if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 3;
            diff+=double(l[i]-ref[i])*(l[i]-ref[i])+double(r[i]-refR[i])*(r[i]-refR[i]);}
        std::cout << "fx" << (fx+1) << " RMS-change " << std::sqrt(diff/(N*2.)) << "\n";
        if(diff<1e-8){std::cerr<<"FAIL inert effect "<<fx+1<<"\n";return 4;}
        ++measured;
    }
    if(measured!=100) {std::cerr<<"Only "<<measured<<" of 100 effects enabled\n";return 15;}
    std::cout<<"PASS: "<<measured<<" implemented knobs change output\n";
    std::array<float,100> previous{},random;std::array<bool,100> lock{};
    lock[0]=true;previous[0]=.71f;
    random=generatePatch(1234,.45f,previous,lock,implemented);
    auto again=generatePatch(1234,.45f,previous,lock,implemented);
    if (random!=again || random[0]!=.71f) return 9;
    for(int i=0;i<100;++i)if(!implemented[size_t(i)])if(random[size_t(i)]!=previous[size_t(i)]) return 10;
    std::cout << "PASS: deterministic patches, locks, unavailable slots\n";
    engine.reset();c.raw.fill(1.f);c.degree=1.f;c.guard=true;c.mix=1.f;
    engine.setControls(c);l.assign(N,8.f);r.assign(N,-8.f);engine.process(l.data(),r.data(),N);
    for(int i=0;i<N;++i)if(!std::isfinite(l[i])||!std::isfinite(r[i])||std::abs(l[i])>.891251f||std::abs(r[i])>.891251f){std::cerr<<"FAIL guard "<<i<<' '<<l[i]<<' '<<r[i]<<'\n';return 5;}
    std::cout<<"PASS: all-on high-level 48 kHz digital sample peak bound\n";
    for(double rate:{44100.,48000.,88200.,96000.,192000.}){
        engine.prepare(rate);engine.setControls(c);
        l.assign(128,.23f);r.assign(128,.35f);engine.process(l.data(),r.data(),128);
        for(float x:l) if(!std::isfinite(x)) return 6;
    }
    std::cout<<"PASS: rate re-preparation test 44.1k to 192k\n";
}
