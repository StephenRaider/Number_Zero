#pragma once
#include "vehicle.h"
#include "traffic.h"
#include "scene.h"
#include <limits>
#include <random>
#include <chrono>
namespace nz {
constexpr double RaceDistance=10000.,RollingSpeed=50./3.6;
// Entropy is sampled once per launch; every race/rematch consumes a new seed.
// Explicit construction keeps tests reproducible and does not expose seeds to AI.
class RaceSeedSequence {
 std::mt19937 generator;std::uint32_t previous=0;bool used=false;
 static std::uint32_t Entropy(){
  auto clock=std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::uint32_t seed=(std::uint32_t)clock^(std::uint32_t)((std::uint64_t)clock>>32);
  try{std::random_device source;seed^=source();}catch(...){/* Clock entropy remains available. */}
  return seed;
 }
public:
 RaceSeedSequence():RaceSeedSequence(Entropy()){}
 explicit RaceSeedSequence(std::uint32_t seed):generator(seed){}
 std::uint32_t Next(){std::uint32_t next;do{next=generator();}while(used&&next==previous);used=true;previous=next;return next;}
};
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
 const CarSpec *spec=&Cars[2];double confidence=.9,reaction=0,boxedTime=0;int targetLane=2;Input held{};bool preparingGap=false;
 struct Plan {bool safe=false;double score=-1e9;Input input{};};
 static double Steering(const Vehicle &v,int lane){return std::clamp((LaneCentres[lane]-v.lateral)*1.35-v.lateralSpeed*.28,-1.,1.);}
 // Continuous relative AABB sweep between prediction samples. This also catches
 // crossing a fast car between endpoints, rather than only checking each frame.
 static bool Sweep(double x,double y,double dx,double dy,double length,double width){
  double enter=0,leave=1;
  auto slab=[&](double p,double d,double half){if(std::abs(d)<1e-8)return std::abs(p)<half;double a=(-half-p)/d,b=(half-p)/d;if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);return enter<leave;};
  return slab(x,dx,length)&&slab(y,dy,width)&&enter<leave;
 }
 Plan Predict(const Vehicle &initial,const std::vector<SeenCar> &seen,int first,int next,int style)const{
  Plan plan;Vehicle v=initial;v.distance=0;double brakeCost=0;bool second=false;int lane=first;
  constexpr double step=.05,horizon=2.6;
  for(int tick=0;tick<52;tick++){
   double t=tick*step;
   if(t>.7&&std::abs(v.lateral-LaneCentres[first])<.40&&std::abs(v.lateralSpeed)<1.4){lane=next;second=next!=first;}
   Input input{1,0,Steering(v,lane)};
   if(style==1||(style==2&&t>=.5)){
    double nearest=1e6,lead=v.speed;
    for(const auto &a:seen){double x=a.station+a.speed*t-v.distance,y=a.lateral+a.lateralSpeed*std::min(t,.6);
     if(x>0&&std::abs(y-v.lateral)<(1.976+a.width)*.5+.10){double gap=x-(4.485+a.length)*.5;if(gap<nearest){nearest=gap;lead=a.speed;}}}
    double closing=std::max(0.,v.speed-lead);
    if(closing>.1&&nearest<1e5){double required=(v.speed*v.speed-lead*lead)/(2*std::max(.5,nearest-2.5-closing*.13));if(required>.8){input.throttle=0;input.brake=std::clamp(required/(spec->brakeG*GameBrakeBoost*9.81),0.,1.);}}
   }
   if(tick==0)plan.input=input;
   Vehicle before=v;nz::Step(v,*spec,input,step);brakeCost+=input.brake*step;
   for(const auto &a:seen){
    double x=a.station+a.speed*t-before.distance,y=a.lateral+a.lateralSpeed*std::min(t,.6)-before.lateral;
    double dx=a.speed*step-(v.distance-before.distance),dy=a.lateralSpeed*(std::min(t+step,.6)-std::min(t,.6))-(v.lateral-before.lateral);
    double length=(4.485+a.length)*.5,width=(1.976+a.width)*.5;
    // A contact can leave cars inside the preferred buffer. Permit paths that
    // increase that clearance; otherwise every escape is rejected at t=0.
    double longitudinalMargin=std::min(.85+(1-confidence)*2.,std::max(0.,std::abs(a.station)-length-.02));
    double lateralMargin=std::min(.08+(1-confidence)*.15,std::max(0.,std::abs(a.lateral-initial.lateral)-width-.02));
    if(Sweep(x,y,dx,dy,length+longitudinalMargin,width+lateralMargin))return plan;
   }
  }
  double room=40;for(const auto &a:seen){double x=a.station+a.speed*horizon-v.distance;if(x>0&&std::abs(a.lateral+a.lateralSpeed*.6-v.lateral)<(1.976+a.width)*.5+.1)room=std::min(room,x);}
  plan.safe=true;plan.score=v.distance+v.speed*.35+room*.025-brakeCost*.6-(first!=targetLane?.55:0)-(second?.3:0);return plan;
 }
public:
 int laneChanges=0,gapPreparations=0,chosenLane=2,rejectedPlans=0;double nearestFront=1e6,closingSpeed=0;std::array<double,3> laneScores{{-1e9,-1e9,-1e9}};
 bool PreparingGap()const{return preparingGap;}
 void Configure(const CarSpec &car,double skill=.9){spec=&car;confidence=std::clamp(skill,.2,1.);}
 void Reset(int lane=2){reaction=boxedTime=0;targetLane=chosenLane=lane;held={};preparingGap=false;laneChanges=gapPreparations=rejectedPlans=0;nearestFront=1e6;closingSpeed=0;laneScores.fill(-1e9);}
 Input Control(const Vehicle &v,const std::vector<SeenCar> &seen,double dt){
  reaction-=dt;if(reaction>0)return held;
  double decisionTime=.10+(1-confidence)*.15;reaction=decisionTime;
  bool settled=std::abs(v.lateral-LaneCentres[targetLane])<.45&&std::abs(v.lateralSpeed)<1.5;
  nearestFront=1e6;double lead=v.speed;
  for(const auto &a:seen)if(a.station>0&&std::abs(a.lateral-v.lateral)<(1.976+a.width)*.5+.18){double gap=a.station-(4.485+a.length)*.5;if(gap<nearestFront){nearestFront=gap;lead=a.speed;}}
  closingSpeed=v.speed-lead;bool blocked=nearestFront<32+std::max(0.,closingSpeed)*1.4||(preparingGap&&nearestFront<1e5);
  Plan best;int bestLane=targetLane;bool adjacentSafe=false;rejectedPlans=0;laneScores.fill(-1e9);
  for(int delta=-1;delta<=1;delta++){
   int first=targetLane+delta;if(first<0||first>3||(!settled&&delta!=0))continue;
   // No unsolicited lane hunting on empty road. Only local obstacles justify a pass.
   if(delta!=0&&!blocked&&!preparingGap)continue;
   for(int next=std::max(0,first-1);next<=std::min(3,first+1);next++)for(int style=0;style<3;style++){
    Plan plan=Predict(v,seen,first,next,style);if(!plan.safe){++rejectedPlans;continue;}
    if(preparingGap&&delta!=0)plan.score+=15;
    laneScores[delta+1]=std::max(laneScores[delta+1],plan.score);adjacentSafe=adjacentSafe||(delta!=0);
    if(plan.score>best.score){best=plan;bestLane=first;}
   }
  }
  bool boxed=settled&&blocked&&(!adjacentSafe||preparingGap);boxedTime=boxed?boxedTime+decisionTime:0;
  bool yield=boxedTime>=.45||(preparingGap&&boxed);
  if(yield&&!preparingGap)++gapPreparations;
  preparingGap=yield;
  held=best.safe?best.input:Input{0,1,Steering(v,targetLane)};
  if(best.safe&&bestLane!=targetLane){targetLane=bestLane;++laneChanges;preparingGap=false;boxedTime=0;}
  chosenLane=targetLane;
  if(preparingGap){double target=std::max(8.,lead-4.5);held.throttle=std::min(held.throttle,std::clamp((target-v.speed)*.65+.25,0.,1.));if(v.speed>target+.3){held.throttle=0;held.brake=std::max(held.brake,std::clamp((v.speed-target)*.16,0.,.28));}}
  return held;
 }
};

enum class RacePhase {Countdown,Racing,Finished};
enum class RaceWinner {None,Player,Opponent,Tie};
struct RaceSession {
 std::uint32_t trafficSeed=12345;
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
