#include "../Source/Core/Engine.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
constexpr int N=48000;
using Audio=std::array<std::vector<float>,2>;
Audio dry() {
    Audio out{{std::vector<float>(N),std::vector<float>(N)}};
    for(int n=0;n<N;++n){
        float t=float(n);
        float onset=(n%8000)<60 ? .48f*std::exp(-float(n%8000)/12.f):0.f;
        out[0][size_t(n)]=.42f*std::sin(.047f*t)+.21f*std::sin(.151f*t)+onset;
        out[1][size_t(n)]=.38f*std::cos(.043f*t)+.20f*std::sin(.11f*t)-.5f*onset;
    }
    return out;
}
double distance(const Audio& a,const Audio& b){
    double sum=0.;
    for(int ch=0;ch<2;++ch)for(int n=0;n<N;++n){
        double d=double(a[ch][size_t(n)])-double(b[ch][size_t(n)]);
        sum+=d*d;
    }
    return std::sqrt(sum/(2*N));
}
Audio run(mcboy::CoreControls c){
    mcboy::Engine e;e.prepare(48000.);e.setControls(c);
    auto out=dry();
    e.process(out[0].data(),out[1].data(),N);
    for(int ch=0;ch<2;++ch)for(float v:out[ch]){
        if(!std::isfinite(v) || (c.guard&&c.mix>0.f&&std::abs(v)>.891251f)){
            std::cerr<<"Nonfinite/uncontrolled sample "<<v<<"\n";std::exit(17);
        }
    }
    return out;
}
}
int main(){
    using namespace mcboy;
    CoreControls c;c.raw.fill(1.f);c.degree=1.f;c.mix=0.f;c.guard=true;
    const auto unprocessed=dry();
    auto exact=run(c);
    for(int ch=0;ch<2;++ch)for(int n=0;n<N;++n)
        if(unprocessed[ch][size_t(n)]!=exact[ch][size_t(n)]){
            std::cerr<<"MIX=0 was not bit-exact dry at sample "<<n<<"\n";return 1;
        }
    std::cout<<"PASS exact dry at 0% MIX with guard ON\n";
    c.raw.fill(0.f);c.raw[0]=1.f;c.mix=1.f;c.guard=false;
    Engine e;e.prepare(48000.);e.setControls(c);
    std::vector<float> impulse(N,0.f),right(N,0.f);
    impulse[0]=1.f;right[0]=-.7f;
    e.process(impulse.data(),right.data(),N);
    for(int i=0;i<2500;++i)
        if(std::abs(impulse[size_t(i)])>1e-6f||std::abs(right[size_t(i)])>1e-6f){
            std::cerr<<"FAIL wet-only path emitted dry transient at "<<i<<"\n";return 2;
        }
    float delayed=0.f;
    for(int i=2500;i<15000;++i)delayed=std::max(delayed,std::abs(impulse[size_t(i)]));
    if(delayed<.05f){std::cerr<<"Wet slap missing\n";return 3;}
    std::cout<<"PASS 100% wet no dry transient, audible delayed impulse\n";
    std::array<float,100> previous{};std::array<bool,100> locks{};
    double tame=0.,wild=0.,closest=100.;
    std::vector<Audio> highAudio;
    for(uint64_t seed=1;seed<=7;++seed){
        auto low=generatePatch(seed*1543,.12f,previous,locks,implemented);
        auto high=generatePatch(seed*1543,.95f,previous,locks,implemented);
        int lowCount=0,highCount=0;
        for(int i=0;i<100;++i){lowCount+=low[size_t(i)]>.001f;highCount+=high[size_t(i)]>.001f;}
        if(lowCount>15||highCount<45){std::cerr<<"Bad Degree density "<<lowCount<<" "<<highCount<<"\n";return 4;}
        c.raw=low;c.degree=.12f;c.mix=1.f;c.guard=true;auto lowResult=run(c);
        c.raw=high;c.degree=.95f;auto highResult=run(c);
        double lowDelta=distance(lowResult,unprocessed),highDelta=distance(highResult,unprocessed);
        tame+=lowDelta;wild+=highDelta;
        highAudio.emplace_back(highResult);
        if(highDelta<.18){std::cerr<<"High Degree barely changes audio\n";return 5;}
        std::cout<<"seed "<<seed<<" low "<<lowCount<<" effects / RMS "<<lowDelta
                 <<" high "<<highCount<<" effects / RMS "<<highDelta<<"\n";
    }
    if(wild<tame*1.3){std::cerr<<"Insufficient intensity difference\n";return 6;}
    for(size_t i=0;i<highAudio.size();++i)for(size_t j=i+1;j<highAudio.size();++j)
        closest=std::min(closest,distance(highAudio[i],highAudio[j]));
    if(closest<.01){std::cerr<<"Shuffles too similar\n";return 7;}
    c.raw=generatePatch(1729,.95f,previous,locks,implemented);
    c.degree=.07f;auto quiet=run(c);c.degree=1.f;auto loud=run(c);
    if(distance(quiet,loud)<.12){std::cerr<<"DEGREE does not reshape actual audio\n";return 8;}
    std::cout<<"PASS Degree high/low ratio "<<wild/tame<<", minimum patch distinction "<<closest
             <<", and nondestructive DEGREE sweep\n";
}
