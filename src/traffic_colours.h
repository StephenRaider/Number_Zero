#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
namespace nz {
struct TrafficPaint {const char *name;std::uint8_t r,g,b;double weight;};
// RGB values are artistic sRGB approximations. Weights are period market priors,
// NOT measured model-specific sales. Provenance: assets/traffic/colours.md.
inline const std::array<std::vector<TrafficPaint>,5> TrafficPalettes{{
 {{"Super White II",235,234,226,47},{"Silver Mica Metallic",172,178,180,53}},
 {{"Pearl White",231,229,218,24},{"Silky Silver Metallic",174,181,185,27},
  {"Bluish Black Pearl 3",27,32,40,16},{"Azure Grey Pearl Metallic",84,99,109,12},
  {"Kashmir Blue Pearl Metallic",71,114,154,10},{"Supreme Red Pearl 2",153,30,39,6},
  {"Lavender Metallic",148,147,171,3},{"Garnet Orange Metallic",168,85,49,2}},
 {{"Super White II",235,234,226,60},{"Black",25,26,28,40}},
 {{"White",232,233,225,90},{"Dark Blue",32,48,72,10}},
 {{"White (conservative provisional fleet finish)",231,231,220,100}}
}};
inline int ChooseTrafficPaint(int type,double unit){
 const auto &palette=TrafficPalettes.at(type);double sum=0;for(const auto &p:palette)sum+=p.weight;
 double value=std::clamp(unit,0.,.999999999)*sum;
 for(size_t i=0;i<palette.size();i++){value-=palette[i].weight;if(value<0)return (int)i;}return (int)palette.size()-1;
}
inline const TrafficPaint &GetTrafficPaint(int type,int index){const auto &p=TrafficPalettes.at(type);return p[(size_t)std::max(0,index)%p.size()];}
}
