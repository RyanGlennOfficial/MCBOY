#include "../Source/Core/Engine.h"
#include "../Source/Presets.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Original synthesized fixture, not a copied recording or sample pack.
namespace {
void u16(std::ofstream& f,uint16_t x){for(int i=0;i<2;++i)f.put(char((x>>(8*i))&255));}
void u32(std::ofstream& f,uint32_t x){for(int i=0;i<4;++i)f.put(char((x>>(8*i))&255));}
void writeWav(const std::string& path,const std::vector<float>& l,const std::vector<float>& r,int rate){
 std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("cannot open WAV");
 uint32_t bytes=uint32_t(l.size()*4);
 f.write("RIFF",4);u32(f,36+bytes);f.write("WAVEfmt ",8);u32(f,16);u16(f,1);u16(f,2);u32(f,uint32_t(rate));u32(f,uint32_t(rate*4));u16(f,4);u16(f,16);
 f.write("data",4);u32(f,bytes);
 for(size_t i=0;i<l.size();++i){
  float a=std::clamp(l[i],-1.f,1.f),b=std::clamp(r[i],-1.f,1.f);
  u16(f,uint16_t(int16_t(std::lrint(a*32767.f))));u16(f,uint16_t(int16_t(std::lrint(b*32767.f))));
 }
}
}
int main(int argc,char** argv){
 if(argc<2){std::cerr<<"Usage: mcboy_render_demo output-prefix\n";return 2;}
 constexpr int rate=48000;
 constexpr size_t samples=rate*3;
 std::vector<float> dryL(samples),dryR(samples);
 const float twopi=6.283185307179586f;
 const float chord[]={110.f,164.81f,196.f,246.94f};
 mcboy::SplitMix64 rng(789123);
 for(size_t i=0;i<samples;++i){
  float t=float(i)/rate;
  float step=std::fmod(t,.375f),pluck=std::exp(-step*5.f);
  float l=0,r=0;
  for(int n=0;n<4;++n){
   const float hz=chord[n];
   l+=(.11f+.02f*n)*pluck*std::sin(twopi*hz*t+.2f*n);
   r+=(.10f+.02f*n)*pluck*std::sin(twopi*hz*t+.3f*n+.013f*hz);
  }
  float transient=std::fmod(t,.75f)<.035f ? (rng.unit()*2.f-1.f)*.10f*std::exp(-90.f*std::fmod(t,.75f)) : 0.f;
  dryL[i]=l+transient;dryR[i]=r+transient*.72f;
 }
 writeWav(std::string(argv[1])+"_dry.wav",dryL,dryR,rate);
 const auto presets=mcboy::factoryPresets();
 for(int index : {3,6,9,16}){
  auto l=dryL,r=dryR;
  const auto& patch=presets[size_t(index)];
  mcboy::CoreControls c;c.degree=patch.degree;c.mix=patch.mix;c.outputDb=patch.outputDb;c.guard=true;c.raw=patch.raw;c.seed=patch.seed;
  mcboy::Engine engine;engine.prepare(rate);engine.setControls(c);
  for(size_t pos=0;pos<samples;pos+=512){
   auto n=int(std::min(size_t(512),samples-pos));
   engine.process(l.data()+pos,r.data()+pos,n);
  }
  float peak=0;
  for(size_t i=0;i<samples;++i)peak=std::max({peak,std::abs(l[i]),std::abs(r[i])});
  if(!std::isfinite(peak)||peak>.892f){std::cerr<<"FAIL peak in render "<<index<<"\n";return 4;}
  auto path=std::string(argv[1])+"_preset_"+std::to_string(index)+".wav";
  writeWav(path,l,r,rate);
  std::cout<<path<<" peak "<<peak<<"\n";
 }
}
