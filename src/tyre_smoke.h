#pragma once
#include "vehicle.h"
#include <vector>
namespace nz {
struct TyreSmoke {
 struct Puff {double station,lateral,height,age,life,size,strength;};
 std::vector<Puff> puffs;double lastElapsed=-1,emitter=0,sustained=0;unsigned sequence=0;int emitted=0;
 void Clear(){puffs.clear();lastElapsed=-1;emitter=sustained=0;sequence=0;emitted=0;}
 void Update(const Vehicle &v,const CarSpec &car,double dt){
  if(v.elapsed<lastElapsed)Clear();
  if(v.elapsed==lastElapsed)return;
  lastElapsed=v.elapsed;
  for(auto &p:puffs){p.age+=dt;p.height+=dt*.13;p.lateral+=dt*.06;p.size+=dt*.18;}
  puffs.erase(std::remove_if(puffs.begin(),puffs.end(),[](const Puff&p){return p.age>=p.life;}),puffs.end());
  bool spinning=v.throttle>.1&&v.brake<.05&&v.slipRatio>.20&&DrivenWheelSpeed(v)-v.speed>1.5;
  sustained=spinning?sustained+dt:0;emitter=std::max(0.,emitter-dt);
  if(sustained<.12||emitter>0)return;
  emitter=.12;
  for(int axle:{-1,1}){
   if(!car.allWheelDrive&&((car.frontDrive&&axle<0)||(!car.frontDrive&&axle>0)))continue;
   for(int side:{-1,1}){
    double forward=axle*car.wheelbase*.5,lateral=side*.73,yaw=v.yaw;
    double jitter=((sequence++*1664525u+1013904223u)%100)/100.;
    puffs.push_back({v.distance+forward*std::cos(yaw)-lateral*std::sin(yaw),v.lateral+forward*std::sin(yaw)+lateral*std::cos(yaw),.08,0,.42+jitter*.14,.09+jitter*.025,std::clamp((DrivenWheelSpeed(v)-v.speed-1.5)/6.,.18,.75)});++emitted;
   }
  }
  if(puffs.size()>48)puffs.erase(puffs.begin(),puffs.begin()+(puffs.size()-48));
 }
};
}
