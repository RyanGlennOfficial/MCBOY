#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>
namespace mcboy {
struct Preset {
  std::string name;
  float degree=.30f,mix=.75f,outputDb=0.f;
  bool guard=true,bypass=false,labels=false;
  uint64_t seed=0xA74E12B50B0C0101ULL;
  std::array<float,100> raw{};
  std::array<bool,100> locked{};
};
inline std::vector<Preset> factoryPresets() {
  std::vector<Preset> p;
  p.reserve(17);
  { Preset s;
    s.name="Start Here"; s.degree=0.3000f; s.mix=0.7500f; s.outputDb=0.00f;
    s.seed=0xaef48190855da820ULL;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Oops, All Reverb"; s.degree=0.8000f; s.mix=0.8000f; s.outputDb=-3.00f;
    s.seed=0xc929015519bde5a1ULL;
    s.raw[10]=0.8500f;
    s.raw[13]=0.7300f;
    s.raw[14]=0.3900f;
    s.raw[19]=0.5000f;
    s.raw[18]=0.3200f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Ghost Radio"; s.degree=0.7000f; s.mix=0.8200f; s.outputDb=-2.00f;
    s.seed=0x0cf3073f14980298ULL;
    s.raw[76]=0.9000f;
    s.raw[70]=0.4700f;
    s.raw[65]=0.5300f;
    s.raw[56]=0.3000f;
    s.raw[79]=0.2600f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Wobbly Guitar Shop"; s.degree=0.6200f; s.mix=0.8000f; s.outputDb=-1.00f;
    s.seed=0x238fdb3bd760c585ULL;
    s.raw[20]=0.7200f;
    s.raw[72]=0.4200f;
    s.raw[34]=0.3600f;
    s.raw[11]=0.6000f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Broken VHS Prom"; s.degree=0.7300f; s.mix=0.8200f; s.outputDb=-3.00f;
    s.seed=0x41f13f2101283322ULL;
    s.raw[2]=0.6600f;
    s.raw[73]=0.7000f;
    s.raw[50]=0.5300f;
    s.raw[34]=0.3400f;
    s.raw[77]=0.1800f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="That Can't Be Legal"; s.degree=0.9800f; s.mix=0.6800f; s.outputDb=-6.00f;
    s.seed=0x9e38f3f867924f7dULL;
    s.raw[42]=0.8000f;
    s.raw[44]=0.7800f;
    s.raw[68]=0.5500f;
    s.raw[77]=0.4300f;
    s.raw[48]=0.2800f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Alien Church"; s.degree=0.8600f; s.mix=0.8100f; s.outputDb=-4.00f;
    s.seed=0x1661dbeee48c96c5ULL;
    s.raw[13]=0.9500f;
    s.raw[56]=0.5500f;
    s.raw[32]=0.6400f;
    s.raw[14]=0.4700f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Cosmic Pillow Fight"; s.degree=0.7700f; s.mix=0.8200f; s.outputDb=-4.00f;
    s.seed=0xfb1e39b1b5420ab8ULL;
    s.raw[14]=0.6800f;
    s.raw[37]=0.4800f;
    s.raw[26]=0.6200f;
    s.raw[10]=0.6800f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Tiny Slap"; s.degree=0.1900f; s.mix=0.6400f; s.outputDb=0.00f;
    s.seed=0x0837a17ed2fc2d92ULL;
    s.raw[0]=0.9100f;
    s.raw[40]=0.7500f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Drum Shredder"; s.degree=0.8400f; s.mix=0.8100f; s.outputDb=-4.00f;
    s.seed=0xf68b9b6e9641e84fULL;
    s.raw[60]=0.9100f;
    s.raw[68]=0.7300f;
    s.raw[93]=0.5600f;
    s.raw[62]=0.5200f;
    s.raw[91]=0.3000f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Spooky Telephone"; s.degree=0.6600f; s.mix=0.8400f; s.outputDb=-3.00f;
    s.seed=0x5391d424c2d2ce05ULL;
    s.raw[76]=0.9300f;
    s.raw[52]=0.9200f;
    s.raw[0]=0.6300f;
    s.raw[72]=0.4000f;
    s.raw[70]=0.3000f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Overcaffeinated Spaceship"; s.degree=0.9100f; s.mix=0.8400f; s.outputDb=-5.00f;
    s.seed=0x3867bc08949c4aeeULL;
    s.raw[25]=0.9700f;
    s.raw[4]=0.8100f;
    s.raw[9]=0.6500f;
    s.raw[23]=0.7000f;
    s.raw[69]=0.3000f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="The Room Is Breathing"; s.degree=0.6600f; s.mix=0.8300f; s.outputDb=-3.00f;
    s.seed=0xdf5f566cc7e10992ULL;
    s.raw[19]=0.9600f;
    s.raw[96]=0.6500f;
    s.raw[13]=0.7300f;
    s.raw[67]=0.4200f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="No More Straight Lines"; s.degree=0.8200f; s.mix=0.8200f; s.outputDb=-4.00f;
    s.seed=0xda0d91c4b8f95ed2ULL;
    s.raw[22]=0.7800f;
    s.raw[55]=0.8400f;
    s.raw[29]=0.5400f;
    s.raw[64]=0.7200f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Petty Theft of Physics"; s.degree=0.8600f; s.mix=0.8600f; s.outputDb=-4.00f;
    s.seed=0xfd3ba0d2cf2763e1ULL;
    s.raw[37]=0.8300f;
    s.raw[27]=0.8300f;
    s.raw[54]=0.6100f;
    s.raw[38]=0.4200f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="Distant Parade"; s.degree=0.6600f; s.mix=0.8600f; s.outputDb=-2.00f;
    s.seed=0x836d9413a0202da0ULL;
    s.raw[17]=0.7900f;
    s.raw[2]=0.6300f;
    s.raw[84]=0.5000f;
    s.raw[19]=0.6700f;
    p.emplace_back(std::move(s)); }
  { Preset s;
    s.name="McBoy Ate the Mix"; s.degree=0.9600f; s.mix=0.6800f; s.outputDb=-8.00f;
    s.seed=0xa86efe6de2d62fefULL;
    s.raw[99]=0.9000f;
    s.raw[58]=0.6600f;
    s.raw[42]=0.5900f;
    s.raw[78]=0.6200f;
    s.raw[88]=0.4800f;
    s.raw[97]=0.5400f;
    s.raw[14]=0.6400f;
    p.emplace_back(std::move(s)); }
  return p;
}
} // namespace mcboy
