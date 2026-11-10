#pragma once
// JUCE-independent realtime-safe core, all 100 numbered behaviors.
// Algorithmic approximations and validation limits: docs/AlgorithmNotes.md.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "../EffectNames.h"
#include "Randomizer.h"
#include "ExtraEffects.h"

namespace mcboy {
constexpr float pi = 3.14159265358979323846f;
inline float sat(float x) noexcept { return std::tanh(x); }
inline float clampSample(float x) noexcept { return std::isfinite(x) ? std::clamp(x, -16.f, 16.f) : 0.f; }
struct CoreControls {
    std::array<float,100> raw{};
    float degree = .30f, mix = .75f, outputDb = 0.f;
    bool guard = true, bypass = false;
    uint64_t seed = 0xA74E12B50B0C0101ULL;
};
class Engine {
public:
    void prepare(double newRate) {
        sr = std::clamp(newRate, 8000.0, 384000.0);
        const auto ringSize = size_t(std::ceil(sr*2.0)) + 8;
        for (auto& h : history) h.assign(ringSize, 0.f);
        for (auto& h : ping) h.assign(ringSize, 0.f);
        for (auto& h : flanger) h.assign(size_t(sr*.075)+16, 0.f);
        const double lengths[4] = {.0297,.0371,.0411,.0437};
        for (int c=0;c<2;++c) for (int i=0;i<4;++i) comb[c][i].assign(size_t(sr*lengths[i])+3+size_t(c*17),0.f);
        extra.prepare(sr);
        reset();
    }
    void reset() noexcept {
        for(auto& v: history) std::fill(v.begin(),v.end(),0.f);
        for(auto& v: ping) std::fill(v.begin(),v.end(),0.f);
        for(auto& v: flanger) std::fill(v.begin(),v.end(),0.f);
        for(auto& side: comb) for(auto& v: side) std::fill(v.begin(),v.end(),0.f);
        hpos=ppos=fpos=0;
        cpos.fill(0); smooth.fill(0.f); lp.fill(0.f); hpState.fill(0.f);
        teleHP.fill(0.f); teleLP.fill(0.f); revDamp.fill(0.f);
        env.fill(0.f); brickEnv.fill(0.f); dcState.fill(0.f);
        sampleHold.fill(0.f); holdCount.fill(0); chopGain=1.f; dropoutGain=1.f;
        tick=0; lastGrid=~uint64_t{0}; bypassWet = controls.bypass ? 0.f : 1.f;
        noise = SplitMix64(controls.seed);
        extra.reset();
        extra.setSeed(controls.seed);
        noiseSeed=controls.seed;
    }
    void setControls(const CoreControls& next) noexcept {
        controls = next;
        const auto safe=[](float x,float fallback,float lo,float hi) noexcept {
            return std::isfinite(x) ? std::clamp(x,lo,hi) : fallback;
        };
        controls.degree=safe(controls.degree,.3f,0.f,1.f);
        controls.mix=safe(controls.mix,.75f,0.f,1.f);
        controls.outputDb=safe(controls.outputDb,0.f,-18.f,6.f);
        for(auto& v:controls.raw) v=safe(v,0.f,0.f,1.f);
        // A stored patch seed governs all procedural glitches/noise.
        if (noiseSeed != controls.seed) {
            noiseSeed=controls.seed;
            noise=SplitMix64(controls.seed);
            extra.setSeed(controls.seed);
        }
    }
    // Inputs are modified in-place. If right==nullptr, process mono.
    // No allocation, file operation or lock occurs in this method.
    void process(float* left, float* right, int count, double bpm=120.0) noexcept {
        if (left==nullptr || count<=0 || history[0].empty()) return;
        bpm = std::clamp(std::isfinite(bpm) ? bpm : 120.0, 30.0, 300.0);
        const auto alpha = float(1.0-std::exp(-1.0/(sr*.028)));
        const auto smoothToggle = float(1.0-std::exp(-1.0/(sr*.025)));
        const auto outputGain=std::pow(10.f,controls.outputDb/20.f);
        const auto srf=float(sr);
        const float cfLP = std::exp(-2*pi*750.f/srf);
        const float cfHP = std::exp(-2*pi*1700.f/srf);
        const float cfTeleHP = std::exp(-2*pi*350.f/srf);
        const float cfTeleLP = std::exp(-2*pi*2900.f/srf);
        const float dcCoeff = std::exp(-2*pi*8.f/srf);
        for(int n=0;n<count;++n) {
            for (int j=0;j<100;++j) if (implemented[size_t(j)]) {
                const auto target=controls.raw[size_t(j)]*controls.degree;
                smooth[size_t(j)] += alpha*(target-smooth[size_t(j)]);
            }
            bypassWet += smoothToggle*((controls.bypass ? 0.f : 1.f)-bypassWet);
            const float a=clampSample(left[n]), b=right?clampSample(right[n]):a;
            const float input[2]={a,b};
            const double beats=double(tick)*bpm/(60.0*sr);
            const auto lag=[&](int c, float samples)->float { return read(history[c],hpos,samples); };
            float wetL=0.f, wetR=0.f, weights=0.f;
            const auto amount=[&](int number) noexcept ->float {return smooth[size_t(number-1)];};
            const auto add=[&](int number,float l,float r,float coefficient=1.5f) noexcept {
                const auto w=coefficient*amount(number);
                if(w<1.0e-6f) return;
                wetL+=clampSample(l)*w; wetR+=clampSample(r)*w; weights+=w;
            };
            // TIME: independent slap and cross-feedback ping-pong.
            if(amount(1)>1e-5f){const float t=srf*(.075f+.09f*amount(1));add(1,lag(0,t),lag(1,t));}
            const float pingTime=srf*float(60.0/bpm)*.5f;
            float p0=read(ping[0],ppos,pingTime),p1=read(ping[1],ppos,pingTime);
            if(amount(2)>1e-5f) add(2,p0,p1);
            ping[0][ppos]=clampSample(a + p1*.45f);ping[1][ppos]=clampSample(b + p0*.45f);
            // SPACE: four-comb plate; room is an independent early-reflection network.
            float reverb[2]={0,0};
            for(int c=0;c<2;++c) for(int k=0;k<4;++k){
                auto& v=comb[c][k];float old=v[cpos[size_t(k)]%v.size()];
                v[cpos[size_t(k)]%v.size()]=.21f*input[c]+.77f*old;
                reverb[c]+=.25f*old;
            }
            if(amount(11)>1e-5f){
                for(int c=0;c<2;++c)revDamp[c]+=.2f*(reverb[c]-revDamp[c]);
                add(11,revDamp[0],revDamp[1]);
            }
            if(amount(13)>1e-5f){
                float r0=.4f*lag(0,srf*.014f)+.3f*lag(1,srf*.031f)+.3f*lag(0,srf*.067f);
                float r1=.4f*lag(1,srf*.019f)+.3f*lag(0,srf*.038f)+.3f*lag(1,srf*.061f);
                add(13,r0,r1);
            }
            // MODULATION: chorus, regenerative flanger, tremolo, autopan, ringmod.
            const float sec=float(tick/sr);
            if(amount(21)>1e-5f){
                const auto d0=srf*(.020f+.004f*std::sin(2*pi*.47f*sec));
                const auto d1=srf*(.023f+.005f*std::sin(2*pi*.57f*sec+1.7f));
                add(21,lag(0,d0),lag(1,d1));
            }
            const float flD = srf*(.0035f+.002f*std::sin(2*pi*.29f*sec));
            const float f0=read(flanger[0],fpos,flD),f1=read(flanger[1],fpos,flD);
            flanger[0][fpos]=clampSample(a+f0*.50f);flanger[1][fpos]=clampSample(b+f1*.50f);
            if(amount(22)>1e-5f)add(22,.5f*(a+f0),.5f*(b+f1));
            if(amount(25)>1e-5f){const float g=.5f+.5f*std::sin(2*pi*5.4f*sec);add(25,a*g,b*g);}
            if(amount(26)>1e-5f){
                const float pan=std::sin(2*pi*.74f*sec);
                const float mid=.5f*(a+b);
                add(26,mid*std::sqrt(1.f-pan),mid*std::sqrt(1.f+pan));
            }
            if(amount(28)>1e-5f){const float carrier=std::sin(2*pi*83.2f*sec);add(28,a*carrier,b*carrier);}
            // PITCH: two overlapping granular heads for +12 semitones (lo-fi algorithm).
            if(amount(31)>1e-5f){
                const float period=srf*.04f;
                const float phase=std::fmod(float(tick)/period,1.f);
                const auto shifted=[&](int c) noexcept {
                    const float u0=phase,u1=std::fmod(phase+.5f,1.f);
                    const float w0=1.f-std::abs(2.f*u0-1.f),w1=1.f-std::abs(2.f*u1-1.f);
                    return w0*lag(c,24.f+(1.f-u0)*period)+w1*lag(c,24.f+(1.f-u1)*period);
                };
                add(31,shifted(0),shifted(1));
            }
            // DAMAGE: five distinct nonlinear/quantized behaviors.
            if(amount(41)>1e-5f)add(41,sat(a*3.0f)*.42f,sat(b*3.0f)*.42f);
            if(amount(42)>1e-5f){
                const auto tube=[](float x){return (sat(x*4.f+.22f)-sat(.22f))*.42f;};
                add(42,tube(a),tube(b));
            }
            if(amount(44)>1e-5f)add(44,std::clamp(a,-.22f,.22f)*2.f,std::clamp(b,-.22f,.22f)*2.f);
            if(amount(46)>1e-5f){
                const auto q=[](float x){return std::round(x*24.f)*(1.f/24.f);};
                add(46,q(a),q(b));
            }
            if(amount(47)>1e-5f){
                for(int c=0;c<2;++c) if(--holdCount[c]<=0){ sampleHold[c]=input[c];holdCount[c]=std::max(2,int(sr/6500.)); }
                add(47,sampleHold[0],sampleHold[1]);
            }
            // SPECTRAL: independent LP, HP, voice-band, short comb.
            float lo[2],hi[2],tele[2];
            for(int c=0;c<2;++c){
                lp[c]=cfLP*lp[c]+(1.f-cfLP)*input[c];lo[c]=lp[c];
                hpState[c]=cfHP*hpState[c]+(1.f-cfHP)*input[c];hi[c]=input[c]-hpState[c];
                teleHP[c]=cfTeleHP*teleHP[c]+(1.f-cfTeleHP)*input[c];
                teleLP[c]=cfTeleLP*teleLP[c]+(1.f-cfTeleLP)*(input[c]-teleHP[c]);tele[c]=teleLP[c];
            }
            if(amount(51)>1e-5f)add(51,lo[0],lo[1]);
            if(amount(52)>1e-5f)add(52,hi[0],hi[1]);
            if(amount(53)>1e-5f)add(53,tele[0],tele[1]);
            if(amount(56)>1e-5f)add(56,a-.86f*lag(0,srf*.0033f),b-.86f*lag(1,srf*.0041f));
            // RHYTHM: 16-step chopper versus seed-stable dropouts.
            const int step=int(std::floor(beats*4.))%16;
            static constexpr float gatePattern[16]={1,0,1,1,0,1,0,0,1,1,0,1,0,1,1,0};
            const float gc = gatePattern[step<0?step+16:step];
            chopGain+=std::min(1.f,1.f/(srf*.002f))*(gc-chopGain);
            if(amount(61)>1e-5f)add(61,a*chopGain,b*chopGain);
            const auto grid=uint64_t(std::floor(beats*8.));
            if (grid!=lastGrid){
                lastGrid=grid;
                SplitMix64 dropNoise(controls.seed ^ (grid*UINT64_C(0x9e3779b97f4a7c15)));
                dropTarget = dropNoise.unit()<.18f ? 0.f:1.f;
            }
            dropoutGain+=std::min(1.f,1.f/(srf*.003f))*(dropTarget-dropoutGain);
            if(amount(66)>1e-5f)add(66,a*dropoutGain,b*dropoutGain);
            // LO-FI: dust, rare pops, slow wow, hiss.
            const float noiseValue=2.f*noise.unit()-1.f;
            if(amount(71)>1e-5f){const float dust=noise.unit()<.003f ? noiseValue*.09f:0.f;add(71,a+dust,b+dust);}
            if(amount(72)>1e-5f){const float pop=noise.unit()<.0008f ? noiseValue*.35f:0.f;add(72,a+pop,b+pop);}
            if(amount(73)>1e-5f){
                const float wowlag=srf*(.007f+.003f*std::sin(2*pi*.63f*sec));
                add(73,lag(0,wowlag),lag(1,wowlag+31.f));
            }
            if(amount(75)>1e-5f)add(75,a+noiseValue*.018f,b-noiseValue*.018f);
            // DYNAMICS: conventional and extreme, separately enveloped detectors.
            float compressed[2],brick[2];
            for(int c=0;c<2;++c){
                const float val=std::abs(input[c]);
                const float rate=val>env[c] ? .015f : .0007f;
                env[c]+=rate*(val-env[c]);
                const float reduction=env[c]>.28f ? std::pow(.28f/env[c],.66f) : 1.f;
                compressed[c]=input[c]*reduction;
                brickEnv[c]+=(val>brickEnv[c] ? .12f:.004f)*(val-brickEnv[c]);
                brick[c]=input[c]*(brickEnv[c]>.06f ? std::pow(.06f/brickEnv[c],.87f) : 1.f);
            }
            if(amount(91)>1e-5f)add(91,compressed[0],compressed[1]);
            if(amount(92)>1e-5f)add(92,brick[0]*1.6f,brick[1]*1.6f);
            // Additional 71 unique modules share the same calibrated parallel bus.
            extra.process(a,b,srf,tick,bpm,smooth,lag,add,reverb);
            // Wet-only DC blocking leaves the neutral dry signal untouched.
            dcState[0] = dcCoeff*dcState[0] + (1.f-dcCoeff)*wetL;
            dcState[1] = dcCoeff*dcState[1] + (1.f-dcCoeff)*wetR;
            wetL -= dcState[0]; wetR -= dcState[1];
            // Normalize category returns instead of stacking 29 effects serially.
            float mixL=(a+wetL)/(1.f+weights),mixR=(b+wetR)/(1.f+weights);
            if(amount(81)>1e-5f){
                const float mid=.5f*(mixL+mixR),side=.5f*(mixL-mixR);
                const float width=1.f+amount(81)*.85f;
                mixL=mid+width*side;mixR=mid-width*side;
            }
            if(amount(84)>1e-5f){
                const float mono=.5f*(mixL+mixR), w=amount(84);
                mixL=(1.f-w)*mixL+w*mono;mixR=(1.f-w)*mixR+w*mono;
            }
            const float mix=controls.mix;
            float outL=((1.f-mix)*a+mix*mixL)*outputGain;
            float outR=((1.f-mix)*b+mix*mixR)*outputGain;
            if(controls.guard){outL=digitalPeakGuard(outL);outR=digitalPeakGuard(outR);}
            left[n]=clampSample(a*(1.f-bypassWet)+outL*bypassWet);
            if(right)right[n]=clampSample(b*(1.f-bypassWet)+outR*bypassWet);
            else left[n]=.5f*(left[n]+clampSample(b*(1.f-bypassWet)+outR*bypassWet));
            history[0][hpos]=a;history[1][hpos]=b;
            hpos=(hpos+1)%history[0].size();ppos=(ppos+1)%ping[0].size();fpos=(fpos+1)%flanger[0].size();
            for(int k=0;k<4;++k)++cpos[size_t(k)];
            ++tick;
        }
    }
    static float digitalPeakGuard(float x) noexcept {
        // Identity below -2.85 dBFS; asymptotic -1 dBFS ceiling.
        constexpr float knee=.72f, ceiling=.89125094f;
        if(!std::isfinite(x)) return 0.f;
        const float mag=std::abs(x);
        if(mag<=knee) return x;
        const float y=knee+(ceiling-knee)*std::tanh((mag-knee)/(ceiling-knee));
        return std::copysign(y,x);
    }
private:
    static float read(const std::vector<float>& v, size_t pos, float samples) noexcept {
        if(v.empty())return 0.f;
        samples=std::clamp(samples,1.f,float(v.size()-3));
        const int integer=int(samples);const float frac=samples-float(integer);
        const size_t i=(pos+v.size()-size_t(integer))%v.size();
        const size_t j=(i+v.size()-1)%v.size();
        return v[i]*(1.f-frac)+v[j]*frac;
    }
    CoreControls controls{};
    double sr=48000.;uint64_t tick=0,noiseSeed=0,lastGrid=~uint64_t{0};
    size_t hpos=0,ppos=0,fpos=0;
    std::array<std::vector<float>,2> history,ping,flanger;
    std::array<std::array<std::vector<float>,4>,2> comb;
    std::array<size_t,4> cpos{};
    std::array<float,100> smooth{};
    std::array<float,2> lp{},hpState{},teleHP{},teleLP{},revDamp{},env{},brickEnv{},dcState{},sampleHold{};
    std::array<int,2> holdCount{};
    float chopGain=1.f,dropoutGain=1.f,dropTarget=1.f,bypassWet=1.f;
    SplitMix64 noise{0xA74E12B50B0C0101ULL};
    ExtraEffects extra;
};
}
