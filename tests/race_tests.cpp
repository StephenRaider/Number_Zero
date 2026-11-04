#include "race.h"
#include <cstdio>
#include <stdexcept>
static void Require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(const auto &spec:nz::Cars){
  nz::Vehicle player;nz::RaceSession race;race.Start(player,spec,false,true);nz::TrafficWorld traffic;auto other=nz::RaceSession::Observer(race.opponent);traffic.Reset(nz::RaceSession::Observer(player),42,&other);
  Require(player.speed==nz::RollingSpeed&&race.opponent.speed==nz::RollingSpeed&&player.distance==race.opponent.distance,"Unequal rolling start");
  for(int tick=0;tick<360;tick++){race.Step(player,spec,{1,1,1},traffic,1./120.);if(race.phase==nz::RacePhase::Countdown)Require(race.PlayerProgress(player)==0&&race.OpponentProgress()==0,"Countdown progress is not zero");}
  Require(race.phase==nz::RacePhase::Racing&&race.elapsed==0&&race.PlayerProgress(player)==0&&race.OpponentProgress()==0,"Countdown distance counts toward the 10km race");
  Require(std::abs(player.speed-nz::RollingSpeed)<1e-9&&std::abs(race.opponent.speed-nz::RollingSpeed)<1e-9&&player.lateral==-1.75,"Countdown inputs alter rolling start");
  Require(!player.automatic&&player.speedLimiterDeleted,"Rolling start discarded garage options");
 }
 {
  nz::Vehicle v;v.speed=50;v.lateral=1.75;std::vector<nz::TrafficCar> traffic(1);traffic[0].station=30;traffic[0].lateral=1.75;traffic[0].speed=25;
  nz::TrafficPlayer player{-100,-1.75,20};auto sensed=nz::ObserveRaceTraffic(v,traffic,player);traffic.resize(30);for(size_t i=1;i<traffic.size();i++){traffic[i].station=100+i*20;traffic[i].lateral=nz::LaneCentres[i%4];traffic[i].speed=0;}
  auto more=nz::ObserveRaceTraffic(v,traffic,player);Require(more.size()==sensed.size(),"Opponent sees future/offscreen traffic");
  nz::RivalDriver a,b;a.Reset();b.Reset();auto x=a.Control(v,sensed,.12),y=b.Control(v,more,.12);Require(x.throttle==y.throttle&&x.brake==y.brake&&x.steer==y.steer,"Distant lane density influences opponent");
  Require(a.laneChanges==1,"Opponent does not react to a visible obstruction");
  nz::RivalDriver empty;empty.Reset();for(int tick=0;tick<100;tick++)empty.Control(v,{},.12);Require(empty.laneChanges==0,"Opponent changes lanes to hunt empty patterns without an obstruction");
 }
 // A boxed driver must deliberately drop behind the alongside cars, then
 // use the opening instead of sitting at the lead car's speed indefinitely.
 {
  nz::Vehicle v;nz::RaceSession::Rolling(v,nz::Cars[2],1.75,true);v.speed=27;
  nz::RivalDriver driver;driver.Reset();double minimum=v.speed;bool merged=false,resumed=false;
  for(int tick=0;tick<120*12;tick++){
   double t=tick/120.;std::vector<nz::SeenCar> seen;
   for(int lane:{1,2,3}){double relative=27*t+(lane==2?25:0)-v.distance;if(relative<=nz::VisibleAhead&&relative>=-25)seen.push_back({relative,nz::LaneCentres[lane],27,0,4.5,1.9});}
   auto input=driver.Control(v,seen,1./120.);minimum=std::min(minimum,v.speed);
   merged=merged||(driver.laneChanges>0&&std::abs(v.lateral-1.75)>2.8);
   resumed=resumed||(merged&&input.throttle>.99&&input.brake==0);
   nz::Step(v,nz::Cars[2],input,1./120.);
   for(const auto &a:seen)Require(std::abs(a.station)>=4.5||std::abs(a.lateral-v.lateral)>=1.94,"Box escape steers into an alongside car");
  }
  Require(driver.gapPreparations>0&&minimum<25,"Boxed rival does not slow down to create space");
  Require(merged&&resumed,"Boxed rival does not use the opened gap and resume acceleration");
  std::printf("PASS boxed escape: min %.1fkm/h, %d gap preparations, %d lane changes\n",minimum*3.6,driver.gapPreparations,driver.laneChanges);
 }
 {
  for(double speed:{20.,50.,65.,73.}){nz::Vehicle v;v.speed=speed;v.lateral=1.75;nz::RivalDriver driver;driver.Reset();auto input=driver.Control(v,{},.125);Require(input.throttle==1&&input.brake==0,"Clear-road rival unnecessarily lifts or brakes");}
  nz::Vehicle v;v.speed=27;v.lateral=1.75;nz::RivalDriver driver;driver.Reset();
  std::vector<nz::SeenCar> queue={{20,1.75,27,0,4.5,1.9},{20,-1.75,27,0,4.5,1.9},{20,5.25,27,0,4.5,1.9}};
  driver.Control(v,queue,.125);Require(driver.laneChanges==0,"Rival hops immediately into an equally blocked queue");
  driver.Reset();Require(!driver.PreparingGap()&&driver.gapPreparations==0,"Restart retains gap preparation state");
 }
 for(int winner=0;winner<3;winner++){
  nz::RaceSession r;nz::Vehicle p;r.phase=nz::RacePhase::Racing;r.startLine=40;r.elapsed=100;double goal=40+nz::RaceDistance;
  double pb=goal-1,ob=goal-1;p.distance=goal+(winner==0?3:1);r.opponent.distance=goal+(winner==1?3:1);r.FinishStep(pb,ob,p,.1);
  Require(r.phase==nz::RacePhase::Finished,"10km finish not detected");
  Require(r.winner==(winner==0?nz::RaceWinner::Player:winner==1?nz::RaceWinner::Opponent:nz::RaceWinner::Tie),"Same-step finish order is incorrect");
 }
 int totalContacts=0;
 for(unsigned seed:{11u,42u,197u,7u,12345u,901u}){
  nz::Vehicle player;nz::RaceSession race;race.Start(player,nz::Cars[0],true,false);nz::TrafficWorld traffic;auto other=nz::RaceSession::Observer(race.opponent);traffic.Reset(nz::RaceSession::Observer(player),seed,&other);
  for(int tick=0;tick<120*420&&race.phase!=nz::RacePhase::Finished;tick++){
   race.Step(player,nz::Cars[0],{1,0,0},traffic,1./120.);Require(traffic.cars.size()<=56,"Two-racer traffic population is unbounded");Require(race.opponent.speed>=0&&std::abs(race.opponent.lateral)<=6.05,"Opponent leaves the road or reverses");
  }
  Require(race.phase==nz::RacePhase::Finished&&race.OpponentProgress()==nz::RaceDistance,"Opponent fails to complete 10km");Require(race.driver.laneChanges>=5,"Opponent does not weave through traffic");Require(race.opponentContacts<=6,"Opponent repeatedly crashes into traffic");totalContacts+=race.opponentContacts;
  std::printf("PASS 10km seed %u: %.1fs, %d lane changes, %d contacts\n",seed,race.elapsed,race.driver.laneChanges,race.opponentContacts);
 }
 Require(totalContacts<=20,"Opponent contact rate is excessive across races");
 {
  nz::Vehicle p;nz::RaceSession r;r.Start(p,nz::Cars[2],true,false);nz::TrafficWorld world;auto o=nz::RaceSession::Observer(r.opponent);world.Reset(nz::RaceSession::Observer(p),42,&o);r.opponent.distance=2500;
  for(int i=0;i<120;i++){o=nz::RaceSession::Observer(r.opponent);world.Step(nz::RaceSession::Observer(p),1./120.,&o);}
  int nearPlayer=0,nearRival=0;for(const auto &a:world.cars){nearPlayer+=std::abs(a.station-p.distance)<450;nearRival+=std::abs(a.station-r.opponent.distance)<450;}
  Require(nearPlayer>4&&nearRival>4,"Leading opponent drives into an empty unsimulated road");
  r.Start(p,nz::Cars[1],true,false);Require(r.countdown==3&&r.elapsed==0&&r.winner==nz::RaceWinner::None&&r.opponentContacts==0,"Rematch retains race state");
 }
 std::puts("PASS race: equal rolling starts, local-only perception, safe weaving, dual traffic coverage, 10km finish ordering and rematch");return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL race: %s\n",e.what());return 1;}}
