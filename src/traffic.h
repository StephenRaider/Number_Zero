#pragma once
#include "vehicle.h"
#include "traffic_colours.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace nz {
constexpr double HighwaySpeedLimit=100./3.6;
constexpr std::array<double,4> LaneCentres{{-5.25,-1.75,1.75,5.25}};
struct TrafficSpec {const char *id;double length,width;};
inline constexpr std::array<TrafficSpec,5> TrafficTypes{{{"corolla",4.365,1.695},{"swift",3.695,1.690},{"crown",4.695,1.695},{"hiace",4.690,1.690},{"rosa",6.990,2.010}}};
struct TrafficPlayer {double station,lateral,speed;double length=4.5,width=1.95;};
struct TrafficCar {
 int id=0,type=0,lane=0,targetLane=0,paint=0;double station=0,lateral=0,speed=0,cruise=0,distance=0,lateralSpeed=0;
 double decision=0,signal=0,change=-1;bool braking=false;int indicator=0;
};
// Predict entry into the common visibility corridor, independent of screen width.
// Continue the marker during the sprite's initial fade-in, then fade it out.
inline double TrafficEntryWarning(const TrafficCar &car,const TrafficPlayer &player,double visibleAhead){
 double relative=car.station-player.station,closing=player.speed-car.speed;
 if(closing<=.5||relative<=visibleAhead-5.)return 0.;
 double time=(relative-visibleAhead)/closing;
 if(time>=3.)return 0.;
 return std::clamp((3.-time)/.6,0.,1.)*std::clamp((relative-(visibleAhead-5.))/5.,0.,1.);
}
class TrafficWorld {
 struct Gap {double front=1e6,rear=1e6,frontSpeed=HighwaySpeedLimit,rearSpeed=0;};
 std::uint32_t rng=12345;int nextId=1;double spawnClock=0,contactCooldown=0;
 double Random(){rng=1664525u*rng+1013904223u;return (rng>>8)/16777216.;}
 bool Occupies(const TrafficCar &a,int lane)const{return a.lane==lane||((a.signal>0||a.change>=0)&&a.targetLane==lane);}
 Gap Neighbours(const TrafficCar &subject,int lane,const TrafficPlayer &player,bool withPlayer=true)const{
  Gap result;const auto &spec=TrafficTypes[subject.type];
  auto add=[&](double station,double speed,double length){double delta=station-subject.station,gap=std::abs(delta)-(length+spec.length)*.5;if(delta>=0&&gap<result.front){result.front=gap;result.frontSpeed=speed;}if(delta<0&&gap<result.rear){result.rear=gap;result.rearSpeed=speed;}};
  for(const auto &a:cars)if(a.id!=subject.id&&Occupies(a,lane))add(a.station,a.speed,TrafficTypes[a.type].length);
  if(withPlayer&&std::abs(player.lateral-LaneCentres[lane])<(player.width+spec.width)*.5+.10)add(player.station,player.speed,player.length);
  return result;
 }
 bool Safe(const TrafficCar &a,int lane,const TrafficPlayer &player)const{
  if(lane<0||lane>=4)return false;
  auto gap=Neighbours(a,lane,player);
  return gap.front>8+a.speed*.60+std::max(0.,a.speed-gap.frontSpeed)*2&&gap.rear>9+gap.rearSpeed+std::max(0.,gap.rearSpeed-a.speed)*2;
 }
 bool Add(int lane,double station,const TrafficPlayer &player){
  if(std::abs(station-player.station)<35)return false;
  for(const auto &a:cars)if(Occupies(a,lane)&&std::abs(a.station-station)<42)return false;
  double choice=Random();int type=choice<.32?0:choice<.56?1:choice<.76?2:choice<.93?3:4;
  TrafficCar a;a.id=nextId++;a.type=type;a.lane=a.targetLane=lane;a.station=station;a.lateral=LaneCentres[lane];a.paint=ChooseTrafficPaint(type,Random());
  double kmh=Random()<.18?72+Random()*16:90+Random()*10;
  if(type>=3)kmh=std::min(kmh,84+Random()*12);
  a.speed=a.cruise=kmh/3.6;a.decision=.3+Random()*3;cars.push_back(a);return true;
 }
public:
 std::vector<TrafficCar> cars;double clock=0;int laneChanges=0,contacts=0;
 void Reset(const TrafficPlayer &player,std::uint32_t seed=12345){
  cars.clear();rng=seed;nextId=1;clock=spawnClock=contactCooldown=0;laneChanges=contacts=0;
  for(int lane=0;lane<4;lane++){Add(lane,player.station+62+lane*11,player);Add(lane,player.station+112+lane*13,player);Add(lane,player.station-46-lane*14,player);}
 }
 void Step(const TrafficPlayer &player,double dt){
  clock+=dt;spawnClock+=dt;contactCooldown=std::max(0.,contactCooldown-dt);
  cars.erase(std::remove_if(cars.begin(),cars.end(),[&](const TrafficCar&a){return a.station<player.station-170||a.station>player.station+450;}),cars.end());
  if(spawnClock>=1.8&&cars.size()<28){spawnClock=0;int lane=(int)(Random()*4);Add(lane,player.station+(Random()<.35?-105-Random()*40:165+Random()*100),player);}
  std::vector<double> acceleration(cars.size());
  for(size_t i=0;i<cars.size();i++){
   auto &a=cars[i];a.decision-=dt;auto gap=Neighbours(a,a.lane,player);
   if(a.signal<=0&&a.change<0&&a.decision<=0){
    a.decision=8+Random()*12;
    if(gap.front<65&&gap.frontSpeed<a.cruise-1.0){
     int preferred=Random()<.5?-1:1;
     for(int direction:{preferred,-preferred})if(Safe(a,a.lane+direction,player)){
      a.targetLane=a.lane+direction;a.signal=1.3;a.indicator=direction;break;
     }
    }
   }
   if(a.signal>0){
    if(!Safe(a,a.targetLane,player)){a.signal=0;a.targetLane=a.lane;a.indicator=0;a.decision=2;}
    else{a.signal=std::max(0.,a.signal-dt);if(a.signal==0)a.change=0;}
   }
   if(a.change>=0){
    double duration=a.type==4?5.0:a.type==3?4.6:3.8,old=a.lateral;a.change+=dt;double t=std::clamp(a.change/duration,0.,1.);a.lateral=LaneCentres[a.lane]+(LaneCentres[a.targetLane]-LaneCentres[a.lane])*(t*t*(3-2*t));a.lateralSpeed=(a.lateral-old)/dt;
    if(t>=1){a.lane=a.targetLane;a.change=-1;a.indicator=0;a.lateralSpeed=0;++laneChanges;}
   }else a.lateralSpeed=0;
   if(a.signal>0||a.change>=0){auto target=Neighbours(a,a.targetLane,player);if(target.front<gap.front)gap=target;}
   double desired=5+a.speed*1.35+std::max(0.,a.speed-gap.frontSpeed)*1.0;
   acceleration[i]=std::clamp((a.type>=3?1.0:1.45)*(1-std::pow(a.speed/a.cruise,4)-std::pow(desired/std::max(.2,gap.front),2)),-7.5,1.45);
   a.braking=acceleration[i]<-.6;
  }
  for(size_t i=0;i<cars.size();i++){auto &a=cars[i];a.speed=std::clamp(a.speed+acceleration[i]*dt,0.,HighwaySpeedLimit);double move=a.speed*dt;a.station+=move;a.distance+=move;}
  // Hard safety envelope supplements the soft following controller after a severe stop.
  std::vector<size_t> order(cars.size());for(size_t i=0;i<order.size();i++)order[i]=i;
  std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return cars[a].station>cars[b].station;});
  for(size_t i:order){auto &a=cars[i];auto gap=Neighbours(a,a.lane,player,false);if(a.change>=0||a.signal>0){auto g=Neighbours(a,a.targetLane,player,false);if(g.front<gap.front)gap=g;}
   if(gap.front<.3){a.station-=.3-gap.front;a.speed=std::min(a.speed,gap.frontSpeed);a.braking=true;}
  }
 }
 bool ResolveContact(Vehicle &player){
  bool contact=false;for(auto &a:cars){const auto &spec=TrafficTypes[a.type];double halfLength=(4.5+spec.length)*.5,halfWidth=(1.95+spec.width)*.5;
   if(std::abs(player.distance-a.station)>=halfLength||std::abs(player.lateral-a.lateral)>=halfWidth)continue;
   contact=true;double longitudinal=halfLength-std::abs(player.distance-a.station),lateral=halfWidth-std::abs(player.lateral-a.lateral);
   if(longitudinal<lateral*2){
    if(player.distance<=a.station){player.distance=a.station-halfLength-.05;player.speed=std::min(player.speed,std::max(0.,a.speed-2));}
    else{a.station=player.distance-halfLength-.10;a.speed=std::min(a.speed,player.speed);a.braking=true;}
   }else{
    double separated=a.lateral+(player.lateral<a.lateral?-1:1)*(halfWidth+.03);
    if(separated< -6.05||separated>6.05){player.distance=a.station-halfLength-.08;player.speed=std::min(player.speed,a.speed);}else player.lateral=separated;
    player.lateralSpeed=0;if(contactCooldown<=0)player.speed*=.88;a.braking=true;
   }
  }
  if(contact&&contactCooldown<=0){++contacts;contactCooldown=.5;}return contact;
 }
};
}
