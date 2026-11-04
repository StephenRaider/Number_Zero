#include "racer_contact.h"
#include "traffic.h"
#include <cstdio>
#include <stdexcept>
static void Require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static bool Near(double a,double b,double tolerance=1e-8){return std::abs(a-b)<tolerance;}
static double Energy(const nz::Vehicle &v,const nz::CarSpec &c){return .5*(c.mass+75.)*(v.speed*v.speed+v.lateralSpeed*v.lateralSpeed);}
struct PushResult {double offset,speed;};
static PushResult SustainedPush(double dt){
 nz::Vehicle a,b;a.speed=b.speed=50;a.lateral=0;b.lateral=1.97;
 for(int i=0;i<(int)std::lround(2/dt);i++){
  nz::Step(a,nz::Cars[4],{1,0,1},dt);
  nz::Step(b,nz::Cars[2],{1,0,std::clamp((1.97-b.lateral)*.8-b.lateralSpeed*.2,-1.,1.)},dt);
  nz::ResolveRacerContact(a,nz::Cars[4],b,nz::Cars[2],dt);
  Require(std::abs(b.lateral-a.lateral)>=nz::RacerContactWidth-1e-8||std::abs(b.distance-a.distance)>=nz::RacerContactLength-1e-8,"Persistent racer penetration");
 }
 return {b.lateral-1.97,b.speed};
}
int main(){try{
 const auto &light=nz::Cars[1],&heavy=nz::Cars[2];
 {
  nz::Vehicle a,b;a.speed=60;b.speed=30;a.distance=0;b.distance=4.4;a.lateral=b.lateral=0;
  double momentum=(light.mass+75)*a.speed+(heavy.mass+75)*b.speed,energy=Energy(a,light)+Energy(b,heavy);
  Require(nz::ResolveRacerContact(a,light,b,heavy),"Rear impact missing");
  Require(Near(a.speed,b.speed)&&Near((light.mass+75)*a.speed+(heavy.mass+75)*b.speed,momentum,1e-6),"Rear impact fails mass/momentum sharing");
  Require(a.speed>30&&b.speed>30&&Energy(a,light)+Energy(b,heavy)<=energy,"Rear contact hard-clamps speed or adds energy");
  Require(b.distance-a.distance>=nz::RacerContactLength&&-a.distance>b.distance-4.4,"Lighter body is not displaced more");
 }
 {
  nz::Vehicle a,b;a.speed=50;b.speed=55;a.lateral=0;b.lateral=1.9;a.lateralSpeed=-1;b.lateralSpeed=1;
  Require(nz::ResolveRacerContact(a,light,b,heavy),"Separating scrape missing");
  Require(a.speed==50&&b.speed==55&&a.lateralSpeed==-1&&b.lateralSpeed==1,"Separating scrape still bleeds speed or cancels escape");
  for(int i=0;i<240;i++){a.lateral=0;b.lateral=1.959;nz::ResolveRacerContact(a,light,b,heavy);}
  Require(a.speed==50&&b.speed==55,"Sustained overlap adds repeated speed loss");
 }
 {
  nz::Vehicle a,b;a.speed=b.speed=50;a.lateral=0;b.lateral=1.95;a.lateralSpeed=.08;a.steering=1;
  nz::ResolveRacerContact(a,light,b,heavy);
  Require(Near(b.lateral,1.95)&&Near(b.lateralSpeed,0)&&Near(a.lateralSpeed,0),"Gentle squeeze defeats supported tyres");
  auto c=a,d=b;c.lateral=0;d.lateral=1.95;c.lateralSpeed=3;c.steering=1;d.lateralSpeed=0;
  double energy=Energy(c,light)+Energy(d,heavy);nz::ResolveRacerContact(c,light,d,heavy);
  Require(d.lateral>1.95&&d.lateralSpeed>.3,"Tyre support makes rival immovable in a hard hit");
  Require(Energy(c,light)+Energy(d,heavy)<=energy+1e-6,"Side impact creates kinetic energy");
 }
 for(double edge:{-1.,1.}){
  nz::Vehicle a,b;a.speed=50;b.speed=52;a.lateral=edge*5.;b.lateral=edge*nz::RacerRoadEdge;a.lateralSpeed=edge*2;a.steering=edge;
  nz::ResolveRacerContact(a,light,b,heavy);
  Require(std::abs(a.lateral)<=nz::RacerRoadEdge&&std::abs(b.lateral)<=nz::RacerRoadEdge&&std::abs(a.lateral-b.lateral)>=nz::RacerContactWidth,"Edge contact leaves overlap or pushes racer off road");
  Require(std::abs(b.lateralSpeed)<1e-9&&a.lateralSpeed*edge<=0,"Pinned contact retains outward velocity");
 }
 for(int kind=0;kind<4;kind++){
  nz::Vehicle a,b;a.distance=0;b.distance=kind==0?4.3:.1;a.lateral=0;b.lateral=kind==0?0:1.9;a.speed=55;b.speed=50;a.lateralSpeed=1.7;b.lateralSpeed=.1;a.steering=1;
  if(kind==2){a.lateral=5.;b.lateral=nz::RacerRoadEdge;}
  if(kind==3){a.lateralSpeed=1;b.lateralSpeed=-1;b.steering=-1;}
  auto swappedA=a,swappedB=b,mirrorA=a,mirrorB=b;mirrorA.lateral*=-1;mirrorB.lateral*=-1;mirrorA.lateralSpeed*=-1;mirrorB.lateralSpeed*=-1;mirrorA.steering*=-1;mirrorB.steering*=-1;
  nz::ResolveRacerContact(a,light,b,heavy);nz::ResolveRacerContact(swappedB,heavy,swappedA,light);nz::ResolveRacerContact(mirrorA,light,mirrorB,heavy);
  Require(Near(a.speed,swappedA.speed)&&Near(b.speed,swappedB.speed)&&Near(a.lateral,swappedA.lateral)&&Near(b.lateral,swappedB.lateral)&&Near(a.distance,swappedA.distance)&&Near(b.distance,swappedB.distance)&&Near(a.lateralSpeed,swappedA.lateralSpeed)&&Near(b.lateralSpeed,swappedB.lateralSpeed),"Contact favours player/rival call order");
  Require(Near(a.speed,mirrorA.speed)&&Near(b.speed,mirrorB.speed)&&Near(a.lateral,-mirrorA.lateral)&&Near(b.lateral,-mirrorB.lateral),"Left and right impacts differ");
 }
 auto normal=SustainedPush(1./120.),fine=SustainedPush(1./240.);
 Require(normal.offset<.22&&normal.speed*3.6>170,"Sustained shove still displaces/slows rival too easily");
 Require(std::abs(normal.offset-fine.offset)<.03&&std::abs(normal.speed-fine.speed)<.3,"Contact response is excessively timestep-dependent");
 {
  nz::TrafficWorld world;nz::Vehicle a;a.speed=50;a.lateral=0;nz::TrafficCar t;t.station=4;t.lateral=0;t.speed=25;world.cars.push_back(t);
  Require(world.ResolveContact(a)&&Near(a.speed,23)&&world.contacts==1,"Existing traffic rear-impact severity changed");
  world.cars.clear();a=nz::Vehicle{};a.speed=50;a.lateral=0;t.station=0;t.lateral=1.8;world.cars.push_back(t);world.clock=0;
  nz::TrafficWorld side;side.cars=world.cars;Require(side.ResolveContact(a)&&Near(a.speed,44),"Existing traffic scrape penalty changed");
 }
 std::printf("PASS racer contact: mass sharing, grip resistance, hard hits, no repeated drag, separating escapes, road edges, symmetry, unchanged traffic; push %.3fm / %.1fkmh (240Hz %.3fm / %.1fkmh)\n",normal.offset,normal.speed*3.6,fine.offset,fine.speed*3.6);
 return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL racer contact: %s\n",e.what());return 1;}}
