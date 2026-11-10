#pragma once
// Deterministic short-time Fourier transform smear for fx060.
// 256-point FFT, 50% overlap, periodic Hann analysis/synthesis windows;
// ~256 wet-signal sample latency with a no-lookahead dry path.
// No allocation/locking/logging in processSample().
#include <array>
#include <complex>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace mcboy {
class SpectralBlur {
public:
    static constexpr int size=256, hop=128, outputRingSize=1024;
    using Complex=std::complex<float>;
    void reset() noexcept {
        for(auto& row : input) row.fill(0.f);
        for(auto& row : output)row.fill(0.f);
        for(auto& row : pastMagnitude)row.fill(0.f);
        position=0;outputPos=0;counter=0;
    }
    void processSample(float a,float b,bool running,float& left,float& right) noexcept {
        if(!running) {left=right=0.f;return;}
        const float in[2]={a,b};
        float* out[2]={&left,&right};
        for(int c=0;c<2;++c){
            input[size_t(c)][size_t(position)]=in[c];
            *out[c]=output[size_t(c)][size_t(outputPos)];
            output[size_t(c)][size_t(outputPos)]=0.f;
        }
        position=(position+1)%size;
        ++counter;
        if(counter%hop==0)for(int c=0;c<2;++c)renderFrame(c);
        outputPos=(outputPos+1)%outputRingSize;
    }
private:
    static void fft(std::array<Complex,size>& data,bool inverse) noexcept {
        for(int i=1,j=0;i<size;++i){
            int bit=size>>1;
            for(;j&bit;bit>>=1)j^=bit;
            j^=bit;
            if(i<j)std::swap(data[size_t(i)],data[size_t(j)]);
        }
        for(int len=2;len<=size;len<<=1){
            const float angle=(inverse?1.f:-1.f)*6.28318530718f/float(len);
            const Complex rotation(std::cos(angle),std::sin(angle));
            for(int i=0;i<size;i+=len){
                Complex phase(1.f,0.f);
                for(int j=0;j<len/2;++j){
                    auto u=data[size_t(i+j)],v=data[size_t(i+j+len/2)]*phase;
                    data[size_t(i+j)]=u+v;
                    data[size_t(i+j+len/2)]=u-v;
                    phase*=rotation;
                }
            }
        }
        if(inverse)for(auto& z:data)z*=1.f/float(size);
    }
    static float window(int sample) noexcept {
        return .5f-.5f*std::cos(6.28318530718f*float(sample)/float(size));
    }
    void renderFrame(int channel) noexcept {
        auto& frame=work[size_t(channel)];
        for(int k=0;k<size;++k){
            float x=input[size_t(channel)][size_t((position+k)%size)];
            frame[size_t(k)]=Complex(x*window(k),0.f);
        }
        fft(frame,false);
        std::array<float,size/2+1> newMag{};
        for(int k=0;k<=size/2;++k){
            auto mag=std::abs(frame[size_t(k)]);
            auto left=std::abs(frame[size_t(std::max(0,k-1))]);
            auto right=std::abs(frame[size_t(std::min(size/2,k+1))]);
            const float spectral=.40f*mag+.30f*(left+right);
            auto& old=pastMagnitude[size_t(channel)][size_t(k)];
            old=.82f*old+.18f*spectral;
            // Mix original-bin energy and blurred-bin magnitude.
            newMag[size_t(k)]=.30f*mag+.70f*old;
        }
        for(int k=0;k<=size/2;++k){
            const float magnitude=std::abs(frame[size_t(k)]);
            frame[size_t(k)]*=newMag[size_t(k)]/(magnitude+1e-9f);
            if(k>0 && k<size/2)frame[size_t(size-k)]=std::conj(frame[size_t(k)]);
        }
        fft(frame,true);
        for(int k=0;k<size;++k){
            // Overlap-add 2 Hann windows; sqrt-Hann COLA would give unity;
            // this bounded prototype stays below unity for safety.
            const int slot=(outputPos+1+k)%outputRingSize;
            output[size_t(channel)][size_t(slot)]+=frame[size_t(k)].real()*window(k);
        }
    }
    std::array<std::array<float,size>,2> input{};
    std::array<std::array<float,outputRingSize>,2> output{};
    std::array<std::array<Complex,size>,2> work{};
    std::array<std::array<float,size/2+1>,2> pastMagnitude{};
    int position=0,outputPos=0;
    uint64_t counter=0;
};
} // namespace mcboy
