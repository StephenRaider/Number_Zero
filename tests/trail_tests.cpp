#include "trail_history.h"
#include <cstdio>
#include <stdexcept>
static void Require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
int main(){try{
 Require(nz::OuterRearLamp(-2.15f,-.65f)&&nz::OuterRearLamp(-2.14f,.79f),"Outer tail clusters rejected");
 Require(!nz::OuterRearLamp(-1.62f,0)&&!nz::OuterRearLamp(-2.1f,.01f),"Centre engine/stop detail produces a trail");
 Require(!nz::OuterRearLamp(2.f,.65f),"Front marker produces a tail trail");

 nz::TailHistory history;nz::Vehicle v;v.speed=30;v.distance=100;v.elapsed=1;
 std::array<nz::TrailPoint,2> lamps{{{-2,-.6f,.5f},{-2,.6f,.5f}}};
 history.Update(v,0,true,false,lamps);Require(history.samples.size()==1&&!history.samples.back().connected,"Initial trail connects to a stale car");
 v.elapsed+=.016;v.distance+=.5;history.Update(v,0,true,false,lamps);
 Require(history.samples.size()==2&&history.samples.back().connected,"Moving lamps do not form a trail");
 Require(std::abs(history.samples.back().point[0].x-98.5)<.001&&std::abs(history.samples.back().point[1].y-2.35)<.001,"Trail does not follow real world lamp positions");
 auto strength=history.samples.back().strength;
 v.elapsed+=.016;history.Update(v,0,false,true,lamps);Require(history.samples.back().strength>strength,"Brake lamps do not strengthen trails");
 v.elapsed+=.016;history.Update(v,0,false,false,lamps);auto count=history.samples.size();Require(!history.continuation,"Unlit car retains a light streak connection");
 v.elapsed+=.016;history.Update(v,0,true,false,lamps);Require(history.samples.size()==count+1&&!history.samples.back().connected,"Light switch gap is bridged by a false trail");
 v.elapsed+=.3;v.speed=0;history.Update(v,0,true,false,lamps);Require(history.samples.empty(),"Stationary trails fail to expire");
 v.speed=30;v.elapsed=2;history.Update(v,0,true,false,lamps);v.elapsed=0;v.distance=0;history.Update(v,0,true,false,lamps);Require(history.samples.size()==1&&!history.samples.back().connected,"Reset retains stale trails");
 v.elapsed+=.01;history.Update(v,1,true,false,lamps);Require(history.car==1&&history.samples.size()==1,"Changing cars retains previous lamp history");
 for(int i=0;i<1000;i++){v.elapsed+=.001;v.distance+=.03;history.Update(v,1,true,false,lamps);}Require(history.samples.size()<=40,"Trail history grows without bound");
 std::puts("PASS tail trails: world position, brake strength, lamp-off gaps, expiry, reset/car change and bounded history");return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
