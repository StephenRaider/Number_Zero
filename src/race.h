#pragma once
#include "vehicle.h"
#include "traffic.h"
#include "scene.h"
#include <limits>
namespace nz {
constexpr double RaceDistance=10000.,RollingSpeed=50./3.6;
struct SeenCar {double station,lateral,speed,lateralSpeed,length,width;};
inline std::vector<SeenCar> ObserveRaceTraffic(const Vehicle &v,const std::vector<TrafficCar> &traffic,const TrafficPlayer &player){
 std::vector<SeenCar> seen;seen.reserve(16);
 for(const auto &a:traffic){double delta=a.station-v.distance;if(delta>VisibleAhead||delta< -25)continue;const auto &spec=TrafficTypes[a.type];seen.push_back({delta,a.lateral,a.speed,a.lateralSpeed,spec.length,spec.width});}
 double pd=player.station-v.distance;if(pd<=VisibleAhead&&pd>=-25)seen.push_back({pd,player.lateral,player.speed,0,player.length,player.width});
 return seen;
}
// This driver receives only a locally filtered kinematic snapshot. It cannot
// access traffic seeds, bands, reserved lanes or future spawn information.
class RivalDriver {
 double reaction=0,laneCooldown=0;int targetLane=2,prefer=-1;Input held{};
public:
 int laneChanges=0;
 void Reset(int lane=2){reaction=0;laneCooldown=0;targetLane=lane;prefer=-1;held={};laneChanges=0;}
 Input Control(const Vehicle &v,const std::vector<SeenCar> &seen,double dt){
  reaction-=dt;laneCooldown=std::max(0.,laneCooldown-dt);
  if(reaction>0)return held;
  reaction=.12;
  int current=std::clamp((int)std::lround((v.lateral+5.25)/3.5),0,3);
  bool settled=std::abs(v.lateral-LaneCentres[targetLane])<.22&&std::abs(v.lateralSpeed)<.65;
  double nearest=1e6,leadSpeed=235./3.6;
  for(const auto &a:seen)if(a.station>0&&std::abs(a.lateral-v.lateral)<(1.976+a.width)*.5+.18){double gap=a.station-(4.485+a.length)*.5;if(gap<nearest){nearest=gap;leadSpeed=a.speed;}}
  double closing=std::max(0.,v.speed-leadSpeed);
  bool blocked=nearest<14+closing*1.45;
  if(settled&&blocked&&laneCooldown<=0){
   // Take the first safe neighbouring gap. No lane-density ranking or route planning.
   for(int direction:{prefer,-prefer}){
    int lane=current+direction;if(lane<0||lane>3)continue;bool safe=true;
    for(const auto &a:seen){
     double predicted=a.lateral+a.lateralSpeed*.45;
     if(std::min(std::abs(a.lateral-LaneCentres[lane]),std::abs(predicted-LaneCentres[lane]))>(1.976+a.width)*.5+.25)continue;
     double gap=std::abs(a.station)-(4.485+a.length)*.5;
     if(gap<(a.station>=0?8+std::max(0.,v.speed-a.speed)*.85:8+std::max(0.,a.speed-v.speed)*.8))safe=false;
    }
    if(safe){targetLane=lane;prefer=-direction;laneCooldown=1.5;++laneChanges;break;}
   }
  }
  held={};held.steer=std::clamp((LaneCentres[targetLane]-v.lateral)*.85-v.lateralSpeed*.20,-1.,1.);
  double target=235./3.6;held.throttle=std::clamp((target-v.speed)*.45+.18,0.,1.);
  if(nearest<1e5&&closing>.2){
   double required=(v.speed*v.speed-leadSpeed*leadSpeed)/(2*std::max(1.,nearest-6-closing*.25));
   if(required>1.2){held.brake=std::clamp(required/(9.81*1.05),0.,1.);held.throttle=0;}
   if(nearest<8){held.brake=1;held.throttle=0;}
  }
  return held;
 }
};
enum class RacePhase {Countdown,Racing,Finished};
enum class RaceWinner {None,Player,Opponent,Tie};
struct RaceSession {
 Vehicle opponent;RivalDriver driver;RacePhase phase=RacePhase::Countdown;RaceWinner winner=RaceWinner::None;
 double countdown=3,elapsed=0,startLine=0,opponentContactCooldown=0,playerFinish=-1,opponentFinish=-1;int opponentContacts=0;Input opponentInput{};
 static TrafficPlayer Observer(const Vehicle &v){return {v.distance,v.lateral,v.speed,4.485,1.976};}
 static void Rolling(Vehicle &v,const CarSpec &spec,double lateral,bool automatic){v=Vehicle{};v.speed=RollingSpeed;v.lateral=lateral;v.gear=1;v.automatic=automatic;v.rpm=v.speed/(2*Pi*spec.radius)*60*spec.gears[1]*spec.finalDrive;}
 void Start(Vehicle &player,const CarSpec &spec,bool automatic,bool deleted){
  Rolling(player,spec,-1.75,automatic);player.speedLimiterDeleted=deleted;Rolling(opponent,Cars[2],1.75,true);driver.Reset(2);
  phase=RacePhase::Countdown;winner=RaceWinner::None;countdown=3;elapsed=startLine=opponentContactCooldown=0;playerFinish=opponentFinish=-1;opponentContacts=0;opponentInput={};
 }
 double PlayerProgress(const Vehicle &v)const{return phase==RacePhase::Countdown?0.:std::clamp(v.distance-startLine,0.,RaceDistance);}
 double OpponentProgress()const{return phase==RacePhase::Countdown?0.:std::clamp(opponent.distance-startLine,0.,RaceDistance);}
 void FinishStep(double playerBefore,double opponentBefore,const Vehicle &player,double dt){
  auto crossing=[&](double before,double after){double goal=startLine+RaceDistance;if(before<goal&&after>=goal)return elapsed+dt*std::clamp((goal-before)/std::max(1e-9,after-before),0.,1.);return -1.;};
  if(playerFinish<0)playerFinish=crossing(playerBefore,player.distance);
  if(opponentFinish<0)opponentFinish=crossing(opponentBefore,opponent.distance);
  elapsed+=dt;
  if(playerFinish>=0||opponentFinish>=0){phase=RacePhase::Finished;if(playerFinish>=0&&opponentFinish>=0)winner=std::abs(playerFinish-opponentFinish)<1e-7?RaceWinner::Tie:playerFinish<opponentFinish?RaceWinner::Player:RaceWinner::Opponent;else winner=playerFinish>=0?RaceWinner::Player:RaceWinner::Opponent;}
 }
 void ResolveRacers(Vehicle &player){
  if(std::abs(player.distance-opponent.distance)>=4.50||std::abs(player.lateral-opponent.lateral)>=1.96)return;
  if(std::abs(player.distance-opponent.distance)>2.5){
   if(player.distance<opponent.distance){player.distance=opponent.distance-4.55;player.speed=std::min(player.speed,opponent.speed);}else{opponent.distance=player.distance-4.55;opponent.speed=std::min(opponent.speed,player.speed);}
  }else{
   double direction=player.lateral<opponent.lateral?-1.:1.;double push=(1.98-std::abs(player.lateral-opponent.lateral))*.5;
   player.lateral=std::clamp(player.lateral+direction*push,-6.05,6.05);opponent.lateral=std::clamp(opponent.lateral-direction*push,-6.05,6.05);player.lateralSpeed=opponent.lateralSpeed=0;player.speed*=.997;opponent.speed*=.997;
  }
 }
 void Step(Vehicle &player,const CarSpec &spec,Input input,TrafficWorld &traffic,double dt){
  if(phase==RacePhase::Finished)return;
  opponentContactCooldown=std::max(0.,opponentContactCooldown-dt);
  if(phase==RacePhase::Countdown){
   double advance=std::min(dt,countdown);player.speed=opponent.speed=RollingSpeed;player.distance+=RollingSpeed*advance;opponent.distance+=RollingSpeed*advance;countdown=std::max(0.,countdown-advance);
   auto other=Observer(opponent);traffic.Step(Observer(player),advance,&other);
   if(countdown<1e-8){countdown=0;phase=RacePhase::Racing;startLine=player.distance;}
   return;
  }
  double pb=player.distance,ob=opponent.distance;opponentInput=driver.Control(opponent,ObserveRaceTraffic(opponent,traffic.cars,Observer(player)),dt);
  nz::Step(player,spec,input,dt);nz::Step(opponent,Cars[2],opponentInput,dt);
  auto other=Observer(opponent);traffic.Step(Observer(player),dt,&other);traffic.ResolveContact(player);traffic.ResolveContact(opponent,&opponentContactCooldown,&opponentContacts);ResolveRacers(player);
  FinishStep(pb,ob,player,dt);
 }
};
}
