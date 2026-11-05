#include "race.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
static void Require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(double sign:{-1.,1.})for(double speed:{15.,40.,65.})for(bool edge:{false,true})for(double longitudinal:{-4.35,-.5,.5,4.35}){
  nz::Vehicle v;nz::RaceSession::Rolling(v,nz::Cars[2],sign*(edge?5.99:2.20),true);v.speed=speed;
  nz::RivalDriver d;d.Reset(sign>0?(edge?3:2):(edge?0:1));
  std::vector<nz::SeenCar> seen={{longitudinal,v.lateral-sign*1.962,speed,sign*2.,4.485,1.976,true}};
  for(int i=0;i<8;i++){
   auto in=d.Control(v,seen,.125);
   Require(in.throttle>.99&&in.brake==0&&!d.PreparingGap()&&d.gapPreparations==0,"Side touch alone triggers braking/gap preparation");
   Require(d.laneChanges==0&&d.sideContest&&!d.forwardBlocked&&std::strcmp(d.decisionReason,"side_hold")==0,"Side pressure classified as a forward hazard or gratuitous lane escape");
  }
 }
 {
  nz::Vehicle v;v.lateral=1.75;v.speed=35;v.lateralSpeed=.3;nz::TrafficPlayer p{1,-.212,35,4.485,1.976,1.2};
  auto seen=nz::ObserveRaceTraffic(v,{},p);Require(seen.size()==1&&seen[0].racer&&seen[0].lateralSpeed==1.2,"Player identity/lateral motion missing from local observations");
 }
 // A genuine front obstacle remains actionable even while the player rubs a side.
 {
  nz::Vehicle v;nz::RaceSession::Rolling(v,nz::Cars[2],2.2,true);v.speed=15;nz::RivalDriver d;d.Reset();
  std::vector<nz::SeenCar> seen={{0,.238,15,0,4.485,1.976,true},{10,2.2,5,0,4.5,1.9}};
  auto in=d.Control(v,seen,.125);Require(d.forwardBlocked&&(in.brake>.1||d.laneChanges>0),"Side contest disables real forward-hazard response");
 }
 // A fully occupying player cutting across from ahead is not a harmless side brush.
 {
  nz::Vehicle v;nz::RaceSession::Rolling(v,nz::Cars[2],1.75,true);v.speed=35;nz::RivalDriver d;d.Reset();
  auto in=d.Control(v,{{9,1.75,15,-1,4.485,1.976,true}},.125);
  Require(!d.sideContest&&d.forwardBlocked&&(in.brake>0||d.laneChanges>0),"Direct racer cut-in ignored as side contact");
 }
 // The player can genuinely box the rival with traffic: preserve gap creation.
 {
  nz::Vehicle v;nz::RaceSession::Rolling(v,nz::Cars[2],1.75,true);v.speed=27;nz::RivalDriver d;d.Reset();double minimum=v.speed;bool merged=false;
  for(int i=0;i<1440;i++){
   double t=i/120.;std::vector<nz::SeenCar> seen;
   for(int lane:{1,2,3}){double x=27*t+(lane==2?25:0)-v.distance;if(x>=-25&&x<=nz::VisibleAhead)seen.push_back({x,nz::LaneCentres[lane],27,0,4.5,1.9,lane==2});}
   auto in=d.Control(v,seen,1./120.);minimum=std::min(minimum,v.speed);merged=merged||(d.laneChanges>0&&std::abs(v.lateral-1.75)>2.8);nz::Step(v,nz::Cars[2],in,1./120.);
  }
  Require(d.gapPreparations>0&&minimum<25&&merged,"Genuinely boxed player/traffic formation no longer permits dropping back to escape");
 }
 for(int car:{2,4})for(double sign:{-1.,1.}){
  nz::Vehicle p;nz::RaceSession r;r.Start(p,nz::Cars[car],true,false);r.phase=nz::RacePhase::Racing;
  p.lateral=sign>0?-1.75:1.75;r.opponent.lateral=-p.lateral;r.driver.Reset(sign>0?2:1);
  nz::TrafficWorld traffic;auto other=nz::RaceSession::Observer(r.opponent);traffic.Reset(nz::RaceSession::Observer(p),42,&other);traffic.cars.clear();
  double braking=0,minSpeed=r.opponent.speed;int sideDecisions=0;
  for(int i=0;i<240;i++){
   r.Step(p,nz::Cars[car],{1,0,sign},traffic,1./120.);traffic.cars.clear();
   braking+=(r.opponentInput.brake>.05)/120.;minSpeed=std::min(minSpeed,r.opponent.speed);sideDecisions+=r.driver.sideContest;
  }
  std::printf("Pressure fixture %s sign %.0f: %.3fs braking, min %.1fkmh, rival final %.1fkmh, gap %.2fm, %d counted touches\n",nz::Cars[car].id,sign,braking,minSpeed*3.6,r.opponent.speed*3.6,r.opponent.distance-p.distance,r.playerRivalContacts);
  Require(r.playerRivalContacts>0&&sideDecisions>0,"Pressure fixture never touches rival");
  Require(braking<.12&&minSpeed>nz::RollingSpeed-.20&&r.driver.gapPreparations==0,"Touch-and-retreat exploit remains in integrated race physics");
 }
 std::puts("PASS rival pressure: both sides, speed range, road edges, observed motion, genuine forward hazards, integrated early pressure without panic retreat");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL rival pressure: %s\n",e.what());return 1;}}
