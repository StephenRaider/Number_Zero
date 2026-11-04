#include "vehicle.h"
#include "traffic.h"
#include <stdexcept>
#include <cstdio>
static void Require(bool b,const char *s){if(!b)throw std::runtime_error(s);}
int main(){try{
 for(int type=0;type<5;type++){
  const auto &palette=nz::TrafficPalettes[type];std::vector<int> counts(palette.size());
  for(int n=0;n<10000;n++){int index=nz::ChooseTrafficPaint(type,(n+.5)/10000.);Require(index>=0&&index<(int)palette.size(),"Traffic selected a non-factory colour");++counts[index];}
  double sum=0;for(const auto &p:palette)sum+=p.weight;
  for(size_t i=0;i<palette.size();i++)Require(std::abs(counts[i]-10000*palette[i].weight/sum)<=2,"Traffic paint weights differ from configured distribution");
 }
 Require(nz::TrafficPalettes[0].size()==2&&nz::TrafficPalettes[2].size()==2,"Assista/Comfort must not use generic colourful paints");
 nz::TrafficCar upcoming;upcoming.station=47.5;upcoming.speed=20;nz::TrafficPlayer observer{0,0,30};
 Require(nz::TrafficEntryWarning(upcoming,observer,42.5)>0,"Offscreen approaching traffic has no advance warning");
 upcoming.station=55;Require(nz::TrafficEntryWarning(upcoming,observer,42.5)==0,"Traffic warns more than one second before entry");
 upcoming.station=52.5;Require(nz::TrafficEntryWarning(upcoming,observer,42.5)==0,"Warning begins before its one-second boundary");
 upcoming.station=90;Require(nz::TrafficEntryWarning(upcoming,observer,42.5)==0,"Far traffic warns prematurely");
 upcoming.station=36;Require(nz::TrafficEntryWarning(upcoming,observer,42.5)==0,"Fully visible traffic keeps its entry warning");
 upcoming.station=60;upcoming.speed=35;Require(nz::TrafficEntryWarning(upcoming,observer,42.5)==0,"Receding traffic shows an entry warning");
 nz::TrafficWorld world;nz::TrafficPlayer player{0,1.75,0};world.Reset(player);
 for(auto &a:world.cars)Require(std::abs(a.station-player.station)>35,"Traffic spawned beside player");
 int signals=0;bool allTypes[5]={};
 for(int tick=0;tick<120*180;tick++){
  if(tick>120*15){player.speed=22;player.station+=player.speed/120.;}
  world.Step(player,1./120.);
  for(const auto &a:world.cars){
   allTypes[a.type]=true;Require(a.paint>=0&&a.paint<(int)nz::TrafficPalettes[a.type].size(),"Spawned paint is outside model palette");signals+=a.indicator!=0;Require(a.speed>=0&&a.speed<=nz::HighwaySpeedLimit+1e-8,"Traffic exceeded the road limit");Require(a.lateral>=-5.251&&a.lateral<=5.251,"Traffic left the highway");Require(std::abs(a.lateralSpeed)<1.5,"Lane change too abrupt");
   Require((a.change<0&&a.signal<=0)||a.indicator!=0,"Lane change without signalling");
  }
  for(size_t i=0;i<world.cars.size();i++)for(size_t j=i+1;j<world.cars.size();j++){
   const auto &a=world.cars[i],&b=world.cars[j];const auto &sa=nz::TrafficTypes[a.type],&sb=nz::TrafficTypes[b.type];
   Require(std::abs(a.lateral-b.lateral)>=(sa.width+sb.width)*.5||std::abs(a.station-b.station)>=(sa.length+sb.length)*.5-.05,"Traffic vehicles overlapped during lane changing/following");
  }
  Require(world.cars.size()<=28,"Unbounded traffic population");
 }
 Require(signals>0&&world.laneChanges>0,"No signalled overtaking in long simulation");for(bool present:allTypes)Require(present,"One of the five traffic types never spawns");
 nz::TrafficWorld a,b;a.Reset({0,0,0},42);b.Reset({0,0,0},42);for(int t=0;t<1200;t++){a.Step({0,0,0},1./120.);b.Step({0,0,0},1./120.);}Require(a.cars.size()==b.cars.size(),"Traffic replay is not deterministic");for(size_t i=0;i<a.cars.size();i++)Require(a.cars[i].station==b.cars[i].station&&a.cars[i].lateral==b.cars[i].lateral,"Traffic seed is unstable");
 // A faster player must not exhaust a fixed per-second vehicle supply.
 // Sample the same spatial window at three speeds, including quiet road sections.
 std::array<double,3> density{};int speedIndex=0;
 for(double kmh:{100.,180.,280.}){
  double total=0;int samples=0,empty=0,openLane=0;
  for(unsigned seed:{11u,42u,197u}){
   nz::TrafficWorld stream;nz::TrafficPlayer driver{0,1.75,kmh/3.6};stream.Reset(driver,seed);
   std::set<int> seen;for(const auto &car:stream.cars)seen.insert(car.id);
   for(int tick=0;tick<60*360;tick++){
    driver.station+=driver.speed/60.;stream.Step(driver,1./60.);
    Require(stream.cars.size()<=28,"Spatial traffic exceeds population budget");
    for(const auto &car:stream.cars)if(seen.insert(car.id).second){double relative=car.station-driver.station;Require(relative>100||relative< -80,"Traffic replenishes inside the visible driving area");}
    if(tick>60*20&&tick%30==0){
     int count=0;bool occupied[4]={};for(const auto &car:stream.cars)if(car.station>driver.station+5&&car.station<driver.station+150){++count;occupied[car.lane]=true;}
     total+=count;++samples;empty+=count==0;openLane+=count>0&&!(occupied[0]&&occupied[1]&&occupied[2]&&occupied[3]);
    }
   }
  }
  density[speedIndex++]=total/samples;
  Require(empty>0&&openLane>samples/2,"Spatial traffic loses quiet stretches or open lanes");
  std::printf("Spatial traffic %.0fkm/h: %.2f cars per 150m, %.1f%% empty samples\n",kmh,total/samples,100.*empty/samples);
 }
 Require(density[1]>density[0]*.8&&density[2]>density[1]*.8&&density[2]<density[1]*1.2,"Traffic density changes materially with player speed");
 // Traffic passed by the leader must survive outside both local windows,
 // until the follower reaches it. Otherwise emitted slots turn into empty road.
 {
  nz::TrafficWorld stream;nz::TrafficPlayer back{0,-1.75,50},front{1200,1.75,65};stream.Reset(back,42,&front);
  nz::TrafficCar bridge;bridge.id=100000;bridge.station=600;bridge.speed=bridge.cruise=25;bridge.lane=bridge.targetLane=0;bridge.lateral=nz::LaneCentres[0];bridge.decision=10;stream.cars.push_back(bridge);
  stream.Step(back,1./60.,&front);
  Require(std::any_of(stream.cars.begin(),stream.cars.end(),[](const nz::TrafficCar &a){return a.id==100000;}),"Leader-passed traffic deleted between racers");
 }
 for(bool playerLeads:{false,true}){
  double nearTotal=0;int samples=0,quiet=0;
  for(unsigned seed:{11u,42u,197u}){
   nz::TrafficWorld stream;nz::TrafficPlayer p{0,-1.75,50},r{0,1.75,65};if(playerLeads)std::swap(p.speed,r.speed);stream.Reset(p,seed,&r);
   for(int tick=0;tick<60*240;tick++){
    p.station+=p.speed/60.;r.station+=r.speed/60.;stream.Step(p,1./60.,&r);
    Require(stream.cars.size()<=nz::TrafficWorld::PopulationLimit(p,&r),"Separated-racer traffic exceeds distance-based budget");
    if(tick>=60*70&&tick%30==0){
     double station=std::min(p.station,r.station);int count=0;for(const auto &a:stream.cars)if(a.station>station+5&&a.station<station+150)++count;
     nearTotal+=count;++samples;quiet+=count==0;
    }
   }
  }
  double density=nearTotal/samples;
  Require(density>2.1&&density<4.5,"Trailing racer loses traffic after the gap exceeds 1000m");
  Require(quiet>0,"Separated-racer traffic loses quiet stretches");
  std::printf("PASS %s leads: trailing density %.2f / 145m with gaps 1-3.6km; quiet samples %d\n",playerLeads?"player":"rival",density,quiet);
 }
 nz::Vehicle v;v.distance=60;v.lateral=nz::LaneCentres[0];v.speed=40;nz::TrafficWorld impact;impact.Reset({0,0,0});impact.cars.resize(1);impact.cars[0].station=62;impact.cars[0].lateral=v.lateral;impact.cars[0].speed=20;
 Require(impact.ResolveContact(v)&&v.speed<=20&&v.distance<60,"Traffic collision did not constrain player");
 v.distance=impact.cars[0].station;v.lateral=6.05;v.speed=30;impact.cars[0].lateral=5.25;
 Require(impact.ResolveContact(v)&&v.lateral<=6.05&&impact.cars[0].station-v.distance>4.3,"Guardrail-side contact left the player inside traffic");
 std::printf("PASS traffic: 5 types, four forward lanes, <=100km/h, slow signalled changes (%d), bounded spawn/replay and contact\n",world.laneChanges);return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"FAIL traffic: %s\n",e.what());return 1;}}
