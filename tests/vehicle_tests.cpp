#include "vehicle.h"
#include <cstdio>
#include <stdexcept>
static void Require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
int main(){try{
 for(const auto &car:nz::Cars){
  nz::Vehicle rest;for(int i=0;i<1200;i++)nz::Step(rest,car,{},1./120.);
  Require(rest.speed==0 && rest.lateral==1.75,"Car moved without input");
  nz::Vehicle v;for(int i=0;i<120*70;i++)nz::Step(v,car,{1,0,0},1./120.);
  Require(v.timeTo100>5.5 && v.timeTo100<10.5,"Unreasonable 0-100 acceleration");
  Require(v.speed*3.6>178 && v.speed*3.6<181,"Stock limiter failed");
  Require(v.gear>=3 && v.shifts>=3,"Automatic shifting failed");
  v.speed=100./3.6;v.lateralSpeed=0;double start=v.distance,seconds=0;
  while(v.speed>.01 && seconds<10){nz::Step(v,car,{0,1,0},1./120.);seconds+=1./120.;}
  Require(v.speed<.1 && v.distance-start>30 && v.distance-start<50,"100-0 braking invalid");
  nz::Vehicle a,b;a.speed=b.speed=25;a.lateral=b.lateral=0;
  for(int i=0;i<120;i++){nz::Step(a,car,{.3,0,1},1./120.);nz::Step(b,car,{.3,0,-1},1./120.);}
  Require(a.lateral>1 && std::abs(a.lateral+b.lateral)<1e-6,"Steering symmetry failed");
  for(int i=0;i<120*10;i++)nz::Step(a,car,{1,0,1},1./120.);
  Require(std::abs(a.lateral)<=6.05,"Highway bounds failed");
  nz::Vehicle x,y;for(int i=0;i<1200;i++)nz::Step(x,car,{1,0,0},1./120.);
  for(int i=0;i<2400;i++)nz::Step(y,car,{1,0,0},1./240.);
  Require(std::abs(x.speed-y.speed)<.2,"Physics timestep dependence too high");
  std::printf("PASS %s: 0-100 %.2fs, 100-0 %.1fm in %.2fs, stock max %.1fkm/h\n",car.name,v.timeTo100,v.distance-start,seconds,180.0);
 }
 std::puts("PASS: idle, throttle, gears, limiter, braking, continuous steering, road bounds and timestep stability");return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
