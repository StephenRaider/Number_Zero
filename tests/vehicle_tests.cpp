#include "vehicle.h"
#include "scene.h"
#include <cstdio>
#include <stdexcept>
static void Require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
int main(){try{
 // Projection must advance northeast while positive steering remains screen-right.
 auto forward=nz::NortheastCamera(1,0,0),right=nz::NortheastCamera(0,1,0);
 Require(forward[0]>0&&forward[1]>0&&right[0]>0&&right[1]<0,"Driving orientation invalid");
 for(float a:{-20.f,0.f,20.f})for(float b:{-7.f,0.f,7.f}){
  auto c=nz::NortheastCamera(a,b,0);auto ground=nz::NortheastGround(c[0],c[1]);
  Require(std::abs(ground.first-a)<.0001&&std::abs(ground.second-b)<.0001,"Projection roundtrip failed");
 }
 // Check every candidate world post across spawn boundaries, including far outside the screen.
 for(int tick=0;tick<1000;tick++){
  double distance=tick*.137;int total=2;
  for(int side:{-1,1}){
   auto [first,last]=nz::StreetRange(distance,-22,36,side);total+=last-first+1;
   for(int k=-10;k<30;k++){
    double s=k*24.+(side>0?12:0)-distance;
    if(s>=-22-25.3&&s<=36+25.3)Require(k>=first&&k<=last,"Contributing offscreen lamp missing");
   }
  }
  Require(total<=24,"Lamp capacity exceeded");
 }
 Require(nz::StreetFalloff(25.3)==0&&nz::StreetFalloff(25.299)<1e-8,"Lamp tail pops at despawn");
 Require(nz::LateralSpeedLimit(30./3.6)>2.5&&nz::LateralSpeedLimit(30./3.6)>nz::LateralSpeedLimit(160./3.6),"Steering speed envelope invalid");
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
  double lowRate=0,highRate=0;
  for(double speed:{30./3.6,160./3.6}){
   nz::Vehicle steering;steering.lateral=0;
   for(int i=0;i<120;i++){steering.speed=speed;steering.lateral=0;nz::Step(steering,car,{0,0,1},1./120.);}
   if(speed<10)lowRate=steering.lateralSpeed;else highRate=steering.lateralSpeed;
   double initial=steering.lateralSpeed;
   for(int i=0;i<60;i++){steering.speed=speed;steering.lateral=0;nz::Step(steering,car,{},1./120.);}
   Require(std::abs(steering.lateralSpeed)<.1&&steering.lateralSpeed<initial,"Steering release drifts");
  }
  Require(lowRate>2.5&&lowRate>highRate,"Low speed response is slower than high speed");
  nz::Vehicle stationary;for(int i=0;i<120;i++)nz::Step(stationary,car,{0,0,1},1./120.);
  Require(stationary.lateralSpeed==0&&stationary.lateral==1.75,"Stationary steering slides the car");
  std::printf("Steering %s: 30km/h %.2fm/s, 160km/h %.2fm/s\n",car.id,lowRate,highRate);
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
 std::puts("PASS: idle, throttle, gears, limiter, braking, continuous steering, road bounds timestep stability, speed-sensitive steering, release, northeast projection and persistent lamps");return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
