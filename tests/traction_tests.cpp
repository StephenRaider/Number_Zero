#include "vehicle.h"
#include "tyre_smoke.h"
#include <cstdio>
#include <stdexcept>
static void Require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct Result {double to60=0,maxSlip=0,spinTime=0,tcTime=0;int puffs=0;};
static Result Launch(const nz::CarSpec &car,bool tcs,bool feather=false,double dt=1./120.,bool assist=false){
 nz::Vehicle v;v.tcsEnabled=tcs;v.tractionAssist=assist;nz::TyreSmoke smoke;Result r;
 for(int tick=0;tick<(int)(15/dt);tick++){
  double before=v.speed;nz::Step(v,car,{feather?nz::DriverTractionThrottle(v,1.):1.,0,0},dt);smoke.Update(v,car,dt);
  Require(std::isfinite(v.speed)&&std::isfinite(v.wheelSpeed)&&v.wheelSpeed>=0&&v.speed>=0,"Non-finite or reversing tyre state");
  Require(v.clutch>=0&&v.clutch<=1&&v.tcsCut>=0&&v.tcsCut<=1,"Invalid clutch/TCS state");
  r.maxSlip=std::max(r.maxSlip,v.slipRatio);r.spinTime+=(v.slipRatio>.20)*dt;r.tcTime+=v.tcsActive*dt;
  if(!r.to60&&v.speed>=60*.44704)r.to60=tick*dt+dt*(60*.44704-before)/(v.speed-before);
  Require(smoke.puffs.size()<=48,"Unbounded tyre particles");
 }
 r.puffs=smoke.emitted;return r;
}
int main(){try{
 Require(nz::TyreGrip(.06)>nz::TyreGrip(.5)&&nz::TyreGrip(0)==0,"Tyres do not lose grip beyond peak slip");
 auto equipped=nz::Cars[2];equipped.factoryTcs=true;
 auto off=Launch(nz::Cars[2],false),on=Launch(equipped,true),skilled=Launch(nz::Cars[2],false,true);
 auto assisted=Launch(nz::Cars[2],false,false,1./120.,true);
 auto unavailable=Launch(nz::Cars[2],true);
 auto combined=Launch(equipped,true,false,1./120.,true);
 Require(combined.tcTime>0&&combined.maxSlip<assisted.maxSlip&&combined.to60>0,"Independent assist and factory controls cannot coexist");
 Require(std::abs(unavailable.to60-off.to60)<.001&&unavailable.tcTime==0,"Factory controller operates on unequipped car");
 Require(assisted.to60<off.to60&&assisted.maxSlip>.08&&assisted.maxSlip<off.maxSlip&&assisted.tcTime==0,"Game pedal aid does not remain imperfect and independent of TCS");
 auto sports=equipped;sports.electronicsTune=nz::ElectronicsTune::Sport;auto sport=Launch(sports,true);
 std::printf("Controller comparison: off %.3fs slip %.3f, assist %.3fs slip %.3f, road TCS %.3fs slip %.3f, sport TCS %.3fs slip %.3f\n",off.to60,off.maxSlip,assisted.to60,assisted.maxSlip,on.to60,on.maxSlip,sport.to60,sport.maxSlip);
 Require(sport.to60<on.to60&&sport.maxSlip>on.maxSlip&&sport.maxSlip<.3,"Sport factory tuning has no useful distinction from road tuning");
 for(const auto &car:nz::Cars)Require(!nz::HasFactoryElectronics(car),"Existing period trim gained invented factory electronics");
 {
  nz::Vehicle a;a.speed=20;a.lateralSpeed=5;a.tcsEnabled=true;a.wheelsInitialized=true;a.wheelSpeed=20;a.clutch=1;
  auto b=a;auto esc=equipped;esc.factoryEsc=true;
  for(int i=0;i<20;i++){nz::Step(a,esc,{1,0,-1},1./120.);nz::Step(b,equipped,{1,0,-1},1./120.);}
  Require(a.escActive&&a.escDemand>.02&&!b.escActive&&b.escDemand==0,"ESC stability authority missing or incorrectly given to TCS-only car");
  a.tcsEnabled=false;nz::Step(a,esc,{1,0,0},1./120.);
  Require(!a.escActive&&!a.tcsActive&&a.escDemand==0&&a.tcsCut==0,"Disabling factory package leaves control authority active");
 }
 Require(off.maxSlip>.3&&off.spinTime>.3&&off.puffs>=2,"Floored non-TCS Ferrari has no meaningful wheelspin/smoke");
 Require(on.maxSlip<.3&&on.tcTime>.3,"TCS does not control slip and prevent sustained smoke");
 Require(off.to60>on.to60+.15&&off.to60>skilled.to60+.15,"Excessive throttle has no traction cost");
 auto awd=Launch(nz::Cars[4],false),rear=Launch(nz::Cars[0],false);
 Require(awd.maxSlip<.12&&rear.maxSlip>.3&&awd.to60<off.to60,"AWD does not retain its traction advantage");
 auto fine=Launch(nz::Cars[2],false,false,1./240.);Require(std::abs(fine.to60-off.to60)<.12,"Slip model is timestep dependent");
 {
  nz::Vehicle v;for(int i=0;i<120;i++)nz::Step(v,nz::Cars[2],{},1./120.);
  Require(v.speed==0&&v.wheelSpeed==0,"No-input wheel creep");
  bool flare=false;for(int i=0;i<360;i++){nz::Step(v,nz::Cars[2],{1,0,0},1./120.);double roadRpm=v.speed/(2*nz::Pi*nz::Cars[2].radius)*60*nz::Cars[2].gears[v.gear]*nz::Cars[2].finalDrive;flare=flare||(v.slipRatio>.2&&v.rpm>roadRpm*1.15);}
  Require(flare&&v.wheelDistance>v.distance,"Wheelspin does not drive RPM/animation independently of road movement");
  for(int i=0;i<480;i++)nz::Step(v,nz::Cars[2],{0,1,0},1./120.);
  Require(v.speed<.01&&v.wheelSpeed<.1,"Wheels fail to settle after lift/braking");
 }
 {
  nz::Vehicle v;v.elapsed=1;v.distance=100;v.lateral=1.75;v.throttle=1;v.slipRatio=.8;v.wheelSpeed=12;v.wheelsInitialized=true;nz::TyreSmoke smoke;
  for(int i=0;i<60;i++){v.elapsed+=1./120.;smoke.Update(v,nz::Cars[2],1./120.);}
  Require(!smoke.puffs.empty(),"Sustained spin emits no smoke");for(const auto&p:smoke.puffs)Require(p.station<v.distance&&std::abs(p.lateral-v.lateral)<1,"RWD smoke is not at rear tyre contact patches");
  auto count=smoke.puffs.size();auto age=smoke.puffs[0].age;smoke.Update(v,nz::Cars[2],1./120.);Require(smoke.puffs.size()==count&&smoke.puffs[0].age==age,"Frozen simulation advances smoke");
  v.throttle=0;v.slipRatio=0;for(int i=0;i<120;i++){v.elapsed+=1./120.;smoke.Update(v,nz::Cars[2],1./120.);}Require(smoke.puffs.empty(),"Smoke persists after grip recovery");
  smoke.Clear();Require(smoke.emitted==0&&smoke.puffs.empty(),"Smoke persists after reset");
 }
 for(int kind:{1,4}){
  nz::Vehicle v;v.elapsed=1;v.distance=100;v.throttle=1;v.slipRatio=.8;v.wheelSpeed=12;v.wheelsInitialized=true;nz::TyreSmoke smoke;
  for(int i=0;i<45;i++){v.elapsed+=1./120.;smoke.Update(v,nz::Cars[kind],1./120.);}
  bool front=false,rear=false;for(const auto&p:smoke.puffs){front=front||p.station>v.distance;rear=rear||p.station<v.distance;}
  Require(front&&(kind==4?rear:!rear),"FWD/AWD smoke uses the wrong axle");
 }
 std::printf("PASS traction: Ferrari floored %.3fs / TCS %.3fs / feathered %.3fs to 60mph; slip %.2f vs %.2f; puffs %d vs %d; R34 %.3fs\n",off.to60,on.to60,skilled.to60,off.maxSlip,on.maxSlip,off.puffs,on.puffs,awd.to60);
 return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL traction: %s\n",e.what());return 1;}}
