#pragma once
// McBoy v1 expansion: original, deterministic low-latency micro-effects.
// Each of these 71 additional modules has its own non-neutral signal behavior.
// Some studio-scale algorithms are explicitly approximated; see docs/AlgorithmNotes.md.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "Randomizer.h"
#include "SpectralBlur.h"

namespace mcboy {
class ExtraEffects {
public:
    void prepare(double rate) {
        sampleRate = float(std::clamp(rate, 8000.0, 384000.0));
        for (auto& ring : gremlin) ring.assign(size_t(sampleRate * .51f) + 8, 0.f);
        reset();
    }
    void reset() noexcept {
        for(auto& x : filters) x.fill(0.f);
        for(auto& x : allpass) x.fill(0.f);
        for(auto& x : envelope) x.fill(0.f);
        for(auto& x : other) x.fill(0.f);
        for(auto& r : gremlin) std::fill(r.begin(),r.end(),0.f);
        gremlinPos=0; shortHold.fill(0.f); holdPeriod=0; pastSeed=0;
        random=SplitMix64(0xDADA12348855ULL); phaseDrift=0.f; spectral.reset();
    }
    void setSeed(uint64_t seed) noexcept {
        if (pastSeed!=seed) {pastSeed=seed;random=SplitMix64(seed^UINT64_C(0xFEDCBA9876543210));}
    }
    template<class Read, class Add>
    void process(float a,float b,float sr,uint64_t tick,double bpm,
                 const std::array<float,100>& v,Read lag,Add add,
                 const float* reverb) noexcept {
        const float in[2]={a,b};
        const float sec=float(double(tick)/double(sr));
        const float beatSamples=sr*60.f/float(bpm);
        const float beat=sec*float(bpm)/60.f;
        const auto amt=[&](int i) noexcept {return v[size_t(i-1)];};
        const auto has=[&](int i) noexcept {return amt(i)>0.00001f;};
        const auto sn=[](float x) noexcept {return std::tanh(x);};
        const auto fract=[](float x) noexcept {return x-std::floor(x);};
        const auto tri=[&](float x) noexcept {return 1.f-std::abs(2.f*fract(x)-1.f);};
        const auto pulse=[&](float cycles,float duty) noexcept {return fract(cycles)<duty?1.f:0.f;};
        const auto one=[&](int c,int idx,float x,float hz) noexcept {
            float z=std::exp(-6.28318530718f*std::clamp(hz,10.f,sr*.43f)/sr);
            float& s=filters[size_t(c)][size_t(idx)];s=z*s+(1.f-z)*x;return s;
        };
        const auto ap=[&](int c,int idx,float x,float coeff) noexcept {
            float& z=allpass[size_t(c)][size_t(idx)];
            const float y=-coeff*x+z;z=x+coeff*y;return y;
        };
        const auto pitch=[&](int c,float ratio,float period=.055f) noexcept {
            // Two crossfaded triangular granular reading heads, arbitrary bounded ratio.
            ratio=std::clamp(ratio,.35f,2.3f);
            float span=std::clamp(sr*period,64.f,sr*.15f);
            float phase=fract((ratio-1.f)*float(tick)/span);
            float u0=phase,u1=fract(phase+.5f);
            float w0=tri(u0),w1=tri(u1);
            return (w0*lag(c,32.f+(1.f-u0)*span)+w1*lag(c,32.f+(1.f-u1)*span)) / std::max(.5f,w0+w1);
        };
        const auto hash=[&](uint64_t step,int salt) noexcept {
            SplitMix64 h(pastSeed^((step+uint64_t(salt+100))*UINT64_C(0x9e3779b97f4a7c15)));
            return h.unit();
        };
        float n = random.unit()*2.f-1.f;
        // A — TIME / ECHO (3-10). Feedback remains local to each bounded behavior.
        if(has(3)){
            const float d=sr*(.19f+.015f*std::sin(6.2831853f*.34f*sec));
            add(3,lag(0,d)+.20f*lag(0,d*1.9f),lag(1,d+17.f)+.20f*lag(1,d*1.8f));
        }
        if(has(4)){
            const float span=sr*.085f, p=std::fmod(float(tick),span);
            add(4,lag(0,1.f+2.f*p),lag(1,1.f+2.f*p+13.f));
        }
        if(has(5)){
            float slice=sr*(.018f+.075f*(1.f-amt(5))),p=std::fmod(float(tick),slice);
            add(5,lag(0,slice-p+6.f),lag(1,slice-p+8.f));
        }
        if(has(6))add(6,lag(0,sr*(.024f+.002f*std::sin(6.2831853f*.63f*sec))),
                             lag(1,sr*(.031f+.003f*std::sin(6.2831853f*.72f*sec))));
        if(has(7)){
            uint64_t grid=uint64_t(beat*12.f);
            add(7,lag(0,sr*(.08f+.31f*hash(grid,7))),lag(1,sr*(.12f+.29f*hash(grid,8))));
        }
        if(has(8))add(8,.53f*lag(0,sr*.131f)+.29f*lag(0,sr*.175f)+.18f*lag(0,sr*.211f),
                            .48f*lag(1,sr*.149f)+.32f*lag(1,sr*.187f)+.20f*lag(1,sr*.231f));
        if(has(9)){
            // Pre-echo *illusion* from an already recorded backward buffer.
            float span=sr*.090f,p=std::fmod(float(tick),span),rise=p/span;
            add(9,lag(0,1.f+2.f*p)*rise,lag(1,1.f+2.f*p)*rise);
        }
        if(has(10)){
            float phase=fract(beat*2.f),spacing=sr*(.21f-.17f*phase);
            add(10,.6f*lag(0,spacing)+.3f*lag(0,spacing*2.f),
                   .6f*lag(1,spacing*1.12f)+.3f*lag(1,spacing*2.07f));
        }
        // B — SPACE / REVERB (12, 14-20).
        if(has(12))add(12,.46f*lag(0,sr*.024f)+.36f*lag(0,sr*.0363f)-.28f*lag(0,sr*.0512f),
                             .46f*lag(1,sr*.023f)+.36f*lag(1,sr*.0371f)-.28f*lag(1,sr*.0498f));
        if(has(14))add(14,.5f*reverb[0]+.35f*lag(0,sr*.413f)+.27f*lag(1,sr*.79f)+.16f*lag(0,sr*1.31f),
                             .5f*reverb[1]+.35f*lag(1,sr*.461f)+.27f*lag(0,sr*.85f)+.16f*lag(1,sr*1.39f));
        if(has(15))add(15,.40f*reverb[0]+.45f*pitch(0,2.f),.40f*reverb[1]+.45f*pitch(1,2.f));
        if(has(16)){
            float g=pulse(beat*4.f,.30f);
            add(16,reverb[0]*g,reverb[1]*g);
        }
        if(has(17)){
            const float p=fract(sec/.42f),gain=p*p;
            add(17,lag(0,sr*(.04f+.38f*p))*gain,lag(1,sr*(.045f+.38f*p))*gain);
        }
        if(has(18)){
            uint64_t step=uint64_t(beat*3.f);
            const float d0=sr*(.012f+.045f*hash(step,180)),d1=sr*(.014f+.051f*hash(step,181));
            add(18,.55f*lag(0,d0)+.35f*lag(1,d1*.65f),.55f*lag(1,d1)+.35f*lag(0,d0*.72f));
        }
        if(has(19)){
            for(int c=0;c<2;++c){
                float& f=other[size_t(c)][0];
                float hold=amt(19);float alpha=(1.f-hold)*.007f+.00005f;
                f=(1.f-alpha)*f+alpha*reverb[c];
            }
            add(19,other[0][0],other[1][0]);
        }
        if(has(20)){
            for(int c=0;c<2;++c){
                float& env=envelope[size_t(c)][0];
                float v1=std::abs(in[c]);env+=.004f*(v1-env);
            }
            add(20,reverb[0]/(1.f+9.f*envelope[0][0]),reverb[1]/(1.f+9.f*envelope[1][0]));
        }
        // C — MODULATION (23, 24, 27, 29, 30).
        if(has(23)){
            float phase=std::sin(6.2831853f*.35f*sec);
            float coeff=.20f+.65f*(phase*.5f+.5f);
            float p0=a,p1=b;
            for(int i=0;i<4;++i){p0=ap(0,i,p0,coeff);p1=ap(1,i,p1,coeff*.96f);}
            add(23,p0,p1);
        }
        if(has(24)){
            float rotor=std::sin(6.2831853f*1.7f*sec);
            add(24,lag(0,sr*(.006f+.002f*rotor))*(.7f+.3f*rotor),
                   lag(1,sr*(.006f-.002f*rotor))*(.7f-.3f*rotor));
        }
        if(has(27)){
            float shift=std::sin(6.2831853f*3.5f*sec);
            add(27,lag(0,sr*(.009f+.002f*shift)),lag(1,sr*(.009f+.002f*shift)));
        }
        if(has(29)){
            float m=std::sin(6.2831853f*.18f*sec);
            float q0=ap(0,4,a,.45f+.25f*m),q1=ap(1,4,b,.45f-.25f*m);
            add(29,q0+.21f*lag(0,sr*.009f),q1-.21f*lag(1,sr*.011f));
        }
        if(has(30)){
            float step=std::floor(beat*4.f), hz=220.f+std::fmod(step*157.f,2400.f);
            add(30,one(0,0,a,hz),one(1,0,b,hz));
        }
        // D — PITCH / HARMONICS (32-40).
        if(has(32))add(32,pitch(0,.5f),pitch(1,.5f));
        if(has(33))add(33,pitch(0,1.498307f),pitch(1,1.498307f));
        if(has(34))add(34,pitch(0,1.259921f),pitch(1,1.259921f));
        if(has(35))add(35,pitch(0,.989f),.18f*lag(1,sr*.024f));
        if(has(36))add(36,.18f*lag(0,sr*.025f),pitch(1,1.011f));
        if(has(37)){
            static constexpr float ratios[6]={.75f,.84f,1.f,1.12246f,1.26f,1.498f};
            int id=int(std::floor(beat*3.f))%6;
            add(37,pitch(0,ratios[id]),pitch(1,ratios[(id+2)%6]));
        }
        if(has(38)){
            add(38,(pitch(0,.973f)+pitch(0,1.041f)+pitch(0,1.122f))*.30f,
                   (pitch(1,.961f)+pitch(1,1.051f)+pitch(1,1.141f))*.30f);
        }
        if(has(39)){
            float freq=420.f+330.f*(1.f+std::sin(6.2831853f*.77f*sec));
            float x0=one(0,1,a,freq),x1=one(1,1,b,freq);
            add(39,sn(4.f*(a-x0))*0.5f,sn(4.f*(b-x1))*0.5f);
        }
        if(has(40)){
            float phase=fract(beat*1.5f);
            float ratio=std::clamp(1.8f-phase*1.35f,.45f,2.f);
            add(40,pitch(0,ratio)*(.9f-.6f*phase),pitch(1,ratio)*(.9f-.6f*phase));
        }
        // E — DAMAGE (43,45,48-50). All branches are bounded before summing.
        if(has(43))add(43,sn(a*20.f)*.65f,sn(b*20.f)*.65f);
        if(has(45)){
            const auto fold=[](float x) noexcept {return 1.f-4.f*std::abs((x*.55f+.25f)-std::floor(x*.55f+.25f+.5f));};
            add(45,fold(a*4.f)*.55f,fold(b*4.f)*.55f);
        }
        if(has(48)){
            float x0=a-one(0,2,a,1600.f),x1=b-one(1,2,b,1600.f);
            add(48,sn(x0*13.f)*.48f,sn(x1*13.f)*.48f);
        }
        if(has(49))add(49,std::max(a,0.f)*.76f,std::max(b,0.f)*.76f);
        if(has(50)){
            float s0=one(0,3,sn(a*9.f),2000.f),s1=one(1,3,sn(b*9.f),2000.f);
            add(50,s0*.68f,s1*.68f);
        }
        // F — FILTER / SPECTRAL (54,55,57-60).
        if(has(54)){
            float freq=350.f+2500.f*(.5f+.5f*std::sin(6.2831853f*.21f*sec));
            auto z0=one(0,4,a,freq),z1=one(1,4,b,freq);
            float band0=one(0,5,a-z0,freq*.22f),band1=one(1,5,b-z1,freq*.22f);
            add(54,a-2.2f*band0,b-2.2f*band1);
        }
        if(has(55)){
            float freq=180.f+2100.f*(.5f+.5f*std::sin(6.2831853f*.83f*sec));
            float o0=one(0,6,a,freq),o1=one(1,6,b,freq);
            add(55,sn((a-o0)*3.f),sn((b-o1)*3.f));
        }
        if(has(57)){
            const float freq1=490.f+250.f*std::sin(6.2831853f*.39f*sec);
            const float freq2=1300.f+380.f*std::cos(6.2831853f*.39f*sec);
            float v0=one(0,7,a,freq1)-one(0,8,a,freq2);
            float v1=one(1,7,b,freq1)-one(1,8,b,freq2);
            add(57,sn(3.2f*v0),sn(3.2f*v1));
        }
        if(has(58)){
            float bass0=one(0,9,a,850.f),bass1=one(1,9,b,850.f);
            add(58,.45f*bass0+1.5f*(a-bass0),.45f*bass1+1.5f*(b-bass1));
        }
        if(has(59)){
            float hi0=a-one(0,10,a,3000.f),hi1=b-one(1,10,b,3000.f);
            add(59,a+.38f*sn(hi0*7.f),b+.38f*sn(hi1*7.f));
        }
        {
            float blurredL=0.f,blurredR=0.f;
            spectral.processSample(a,b,has(60),blurredL,blurredR);
            if(has(60))add(60,blurredL,blurredR);
        }
        // G — RHYTHM / GLITCH (62-65,67-70).
        if(has(62)){
            float period=beatSamples*.5f,p=std::fmod(float(tick),period);
            add(62,lag(0,period-p+sr*.03f),lag(1,period-p+sr*.031f));
        }
        if(has(63)){
            for(int c=0;c<2;++c){
                float& env=envelope[size_t(c)][1];
                float attack=std::abs(in[c]);env+=.03f*(attack-env);
                float& ph=other[size_t(c)][1];ph+=.97f/(sr*.09f);
                if(attack>env*2.6f && attack>.13f) ph=0.f;
            }
            const float g0=std::exp(-18.f*other[0][1]),g1=std::exp(-18.f*other[1][1]);
            add(63,std::sin(6.2831853f*140.f*sec)*g0*.5f,std::sin(6.2831853f*149.f*sec)*g1*.5f);
        }
        if(has(64))add(64,.55f*lag(0,beatSamples/3.f)+.36f*lag(0,beatSamples*2.f/3.f),
                             .55f*lag(1,beatSamples/3.f)+.36f*lag(1,beatSamples*2.f/3.f));
        if(has(65)){
            float p=fract(beat*2.f);
            float swing=(p<.5f ? p*.7f : .35f+(p-.5f)*1.3f);
            add(65,lag(0,sr*(.016f+.06f*swing)),lag(1,sr*(.019f+.062f*swing)));
        }
        if(has(67)){
            static constexpr float pattern[16]={1,0,0,1,1,0,1,0,1,0,1,1,0,0,1,0};
            int i=int(std::floor(beat*4.f))%16;
            add(67,a*pattern[i],b*pattern[(i+3)%16]);
        }
        if(has(68)){
            float pump=.2f+.8f*std::sin(1.5707963f*fract(beat*2.f));
            add(68,a*pump,b*pump);
        }
        if(has(69)){
            float phase=fract(beat*8.f),interval=sr*(.010f+.04f*(1.f-phase));
            add(69,lag(0,interval),lag(1,interval));
        }
        if(has(70)){
            float p=fract(beat*1.5f),distance=sr*(.02f+(.11f*(p<.5f ? p*2.f : (1.f-p)*2.f)));
            add(70,lag(0,distance),lag(1,distance+5.f));
        }
        // H — LO-FI (74,76-80).
        if(has(74)){
            float d=sr*(.004f+.0008f*std::sin(6.2831853f*10.8f*sec)+.0004f*std::sin(6.2831853f*14.1f*sec));
            add(74,lag(0,d),lag(1,d*1.11f));
        }
        if(has(76)){
            float drag=.5f+.5f*std::cos(6.2831853f*.22f*sec);
            add(76,lag(0,sr*(.008f+.075f*drag)),lag(1,sr*(.009f+.083f*drag)));
        }
        if(has(77)){
            float radio0=one(0,11,a-one(0,12,a,320.f),3200.f);
            float radio1=one(1,11,b-one(1,12,b,320.f),3200.f);
            add(77,radio0+.035f*n,radio1-.035f*n);
        }
        if(has(78)){
            uint64_t block=tick/127;
            if(block!=holdPeriod){holdPeriod=block;for(int c=0;c<2;++c)shortHold[size_t(c)]=hash(block,700+c)>.17f?in[c]:0.f;}
            add(78,shortHold[0],shortHold[1]);
        }
        if(has(79)){
            uint64_t k=tick/211;
            float d=sr*(.012f+.046f*hash(k,790));
            float gate=hash(k,791)>.42f?1.f:0.f;
            add(79,lag(0,d)*gate,lag(1,d*1.09f)*gate);
        }
        if(has(80)){
            uint64_t grid=uint64_t(beat*4.f);
            float gate=hash(grid,800)>.14f?1.f:.18f;
            add(80,one(0,13,a,1800.f)*gate,one(1,13,b,1800.f)*gate);
        }
        // I — STEREO (82,83,85-90). Mono returns are handled by the parent engine.
        if(has(82)){
            float mid=.5f*(a+b),side=.5f*(a-b);
            add(82,mid*.8f-side*.75f,mid*.8f+side*.75f);
        }
        if(has(83))add(83,a,lag(1,sr*.018f));
        if(has(85)){
            float p=std::sin(6.2831853f*.27f*sec),mid=.5f*(a+b);
            add(85,mid*(1.f-p)*.9f,mid*(1.f+p)*.9f);
        }
        if(has(86)){
            float far0=one(0,14,.85f*a+.15f*b,1600.f);
            float far1=one(1,14,.85f*b+.15f*a,1600.f);
            add(86,far0*.84f,far1*.84f);
        }
        if(has(87))add(87,ap(0,5,a,.71f),ap(1,5,b,-.71f));
        if(has(88))add(88,ap(0,6,a,.23f),ap(1,6,b,.79f));
        if(has(89)){
            float p=std::sin(6.2831853f*(.18f*sec+.04f*sec*sec/(1.f+sec/100.f)));
            add(89,.6f*((1.f-p)*a+(1.f+p)*b),.6f*((1.f+p)*a+(1.f-p)*b));
        }
        if(has(90)){
            uint64_t grid=tick/std::max(uint64_t{1},uint64_t(sr*.07f));
            float target=hash(grid,900)*2.f-1.f;
            phaseDrift+=.002f*(target-phaseDrift);
            add(90,a*(1.f-phaseDrift*.8f),b*(1.f+phaseDrift*.8f));
        }
        // J — DYNAMICS / CHAOS (93-100).
        if(has(93)){
            float gates[2];
            for(int c=0;c<2;++c){float& e=envelope[size_t(c)][2];e+=.002f*(std::abs(in[c])-e);
                gates[c]=std::clamp((e-.045f)*30.f,0.f,1.f);}
            add(93,a*gates[0],b*gates[1]);
        }
        if(has(94) || has(95)){
            float attack[2];
            for(int c=0;c<2;++c){
                float& fast=envelope[size_t(c)][3],& slow=envelope[size_t(c)][4];
                fast+=.07f*(std::abs(in[c])-fast);slow+=.0012f*(std::abs(in[c])-slow);
                attack[c]=std::clamp((fast-slow)*5.f,-1.f,1.f);
            }
            if(has(94))add(94,a*(1.f+.9f*std::max(0.f,attack[0])),b*(1.f+.9f*std::max(0.f,attack[1])));
            if(has(95))add(95,a*(1.f-.8f*std::max(0.f,attack[0])),b*(1.f-.8f*std::max(0.f,attack[1])));
        }
        if(has(96)){
            float readout[2];
            for(int c=0;c<2;++c){
                auto& buf=gremlin[size_t(c)];
                size_t r=(gremlinPos+buf.size()-std::min(buf.size()-2,size_t(sr*.143f+11.f*c)))%buf.size();
                float prior=buf[r];
                readout[c]=prior;
                buf[gremlinPos]=std::tanh(in[c]*.42f+prior*.72f)*.75f;
            }
            add(96,readout[0],readout[1]);
        }
        if(has(97)){
            uint64_t grid=tick/std::max(uint64_t{1},uint64_t(sr*.13f));
            float target=.3f+1.2f*hash(grid,970);
            for(int c=0;c<2;++c)other[size_t(c)][2]+=.0007f*(target-other[size_t(c)][2]);
            add(97,a*other[0][2],b*other[1][2]);
        }
        if(has(98)){
            float warped[2];
            for(int c=0;c<2;++c){
                float& z=other[size_t(c)][3],& env=envelope[size_t(c)][5];
                env+=.002f*(std::abs(in[c])-env);
                z=std::clamp(3.73f*z*(1.f-z)+.0002f*env,.0001f,.9999f);
                float cutoff=350.f+4000.f*z*env;
                warped[c]=one(c,15,in[c],cutoff);
            }
            add(98,warped[0],warped[1]);
        }
        if(has(99)){
            uint64_t step=tick/std::max(uint64_t{1},uint64_t(sr*.061f));
            float faulty=hash(step,990)>.24f?1.f:.09f;
            add(99,std::tanh(a*8.f)*faulty*.6f,std::tanh(b*8.f)*faulty*.6f);
        }
        if(has(100)){
            float phase=fract(beat*3.f);
            float d=sr*(.02f+.12f*(1.f-phase));
            float e0=one(0,16,a,1300.f),e1=one(1,16,b,1300.f);
            add(100,std::tanh((e0+lag(0,d))*(1.f+phase*3.f))*.53f,
                    std::tanh((e1+lag(1,d*1.09f))*(1.f+phase*3.f))*.53f);
        }
        gremlinPos=(gremlinPos+1)%gremlin[0].size();
    }
private:
    float sampleRate=48000.f;
    std::array<std::array<float,17>,2> filters{};
    std::array<std::array<float,8>,2> allpass{};
    std::array<std::array<float,8>,2> envelope{};
    std::array<std::array<float,8>,2> other{};
    std::array<std::vector<float>,2> gremlin{};
    size_t gremlinPos=0;
    std::array<float,2> shortHold{};
    uint64_t holdPeriod=~uint64_t{0},pastSeed=0;
    SplitMix64 random{0xDADA12348855ULL};
    float phaseDrift=0.f;
    SpectralBlur spectral;
};
} // namespace mcboy
