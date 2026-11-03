#pragma once
#include "vehicle.h"
#include <array>
#include <vector>
namespace nz {
struct TrailPoint {float x,y,z;};
struct TailHistory {
 struct Sample {double time;std::array<TrailPoint,2> point;float strength;bool connected;};
 std::vector<Sample> samples;int car=-1;bool continuation=false;
 void Clear(){samples.clear();car=-1;continuation=false;}
 void Update(const Vehicle &v,int selected,bool tails,bool brake,const std::array<TrailPoint,2> &local){
  if(car!=selected||(!samples.empty()&&(v.elapsed<samples.back().time||v.distance+5<samples.back().point[0].x))){Clear();car=selected;}
  while(!samples.empty()&&v.elapsed-samples.front().time>.14)samples.erase(samples.begin());
  if(v.speed<2||(!tails&&!brake)){continuation=false;return;}
  if(!samples.empty()&&v.elapsed<=samples.back().time)return;
  Sample sample;sample.time=v.elapsed;sample.strength=brake?1.f:.5f;
  sample.connected=continuation&&!samples.empty()&&v.elapsed-samples.back().time<.06;
  for(int side=0;side<2;side++)sample.point[side]={(float)v.distance+local[side].x,(float)v.lateral+local[side].y,local[side].z};
  samples.push_back(sample);continuation=true;if(samples.size()>40)samples.erase(samples.begin());
 }
};
}
