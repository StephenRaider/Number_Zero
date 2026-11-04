#include "race.h"
#include "race_telemetry.h"
#include <sstream>
#include <cstdio>
#include <stdexcept>
static void Require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 {
  nz::RaceSeedSequence seeds(42),replay(42);std::uint32_t previous=0;bool differentLayout=false;
  nz::TrafficWorld original;auto first=seeds.Next();Require(first==replay.Next(),"Seed sequence cannot be replayed");original.Reset({0,0,0},first);previous=first;
  for(int i=0;i<100;i++){
   auto next=seeds.Next();Require(next!=previous&&next==replay.Next(),"Rematch repeats its seed or deterministic replay differs");previous=next;
   nz::TrafficWorld fresh,copy;fresh.Reset({0,0,0},next);copy.Reset({0,0,0},next);
   Require(fresh.cars.size()==copy.cars.size(),"Recorded seed cannot reproduce traffic");
   if(fresh.cars.size()!=original.cars.size())differentLayout=true;
   for(size_t j=0;j<fresh.cars.size();j++){Require(fresh.cars[j].station==copy.cars[j].station&&fresh.cars[j].type==copy.cars[j].type&&fresh.cars[j].paint==copy.cars[j].paint,"Recorded seed changes layout");if(j<original.cars.size()&&fresh.cars[j].station!=original.cars[j].station)differentLayout=true;}
  }
  Require(differentLayout,"Fresh race seeds leave the same traffic layout");std::puts("PASS fresh race/rematch seeds and recorded-seed replay");
 }
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
 // The adjacent lane has a farther car: use it briefly, pass, and return
 // instead of rejecting the entire lane because it is not empty forever.
 {
  nz::Vehicle v;nz::RaceSession::Rolling(v,nz::Cars[2],1.75,true);v.speed=40;
  nz::RivalDriver driver;driver.Reset();bool right=false,returned=false;double closest=1e6;
  for(int tick=0;tick<120*8;tick++){
   double t=tick/120.;std::vector<nz::SeenCar> seen;
   for(auto a:std::vector<nz::SeenCar>{{18,1.75,25,0,4.5,1.9},{42,5.25,25,0,4.5,1.9},{0,-1.75,40,0,4.5,1.9}}){a.station+=a.speed*t-v.distance;if(a.station>=-25&&a.station<=nz::VisibleAhead)seen.push_back(a);}
   auto input=driver.Control(v,seen,1./120.);nz::Step(v,nz::Cars[2],input,1./120.);
   right=right||v.lateral>4.5;returned=returned||(right&&v.lateral<2.2&&v.distance>25*t+26);
   for(const auto &a:seen)if(std::abs(a.lateral-v.lateral)<1.94)closest=std::min(closest,std::abs(a.station));
  }
  Require(right&&returned,"Rival refuses the temporary right-lane overtake and return");Require(closest>4.5,"Predicted overtake contacts a car");
  std::puts("PASS temporary-gap overtake and return with farther car in passing lane");
 }
 {
  nz::Vehicle v;v.speed=0;v.lateral=1.75;nz::RivalDriver driver;driver.Reset();
  auto control=driver.Control(v,{{-4.55,1.75,0,0,4.485,1.976}},.125);
  Require(control.throttle==1&&control.brake==0,"Preferred buffer deadlocks a car after rear contact");
 }
 {
  nz::Vehicle v;nz::RaceSession::Rolling(v,nz::Cars[2],1.75,true);v.speed=40;
  auto weak=nz::Cars[2];weak.steerResponse=2;weak.grip=.4;
  nz::RivalDriver capable,limited;capable.Configure(nz::Cars[2]);limited.Configure(weak);capable.Reset();limited.Reset();
  std::vector<nz::SeenCar> seen={{14,1.75,25,0,4.5,1.9},{42,5.25,25,0,4.5,1.9},{0,-1.75,40,0,4.5,1.9}};
  auto fast=capable.Control(v,seen,.125),slow=limited.Control(v,seen,.125);
  Require(fast.steer>.9&&fast.brake==0&&slow.brake>.5,"Lane-change prediction ignores grip and steering capability");
  std::puts("PASS handling-aware gap: capable car passes under power; low-grip/slower steering requires braking");
 }
 for(int winner=0;winner<3;winner++){
  nz::RaceSession r;nz::Vehicle p;r.phase=nz::RacePhase::Racing;r.startLine=40;r.elapsed=100;double goal=40+nz::RaceDistance;
  double pb=goal-1,ob=goal-1;p.distance=goal+(winner==0?3:1);r.opponent.distance=goal+(winner==1?3:1);r.FinishStep(pb,ob,p,.1);
  Require(r.phase==nz::RacePhase::Finished,"10km finish not detected");
  Require(r.winner==(winner==0?nz::RaceWinner::Player:winner==1?nz::RaceWinner::Opponent:nz::RaceWinner::Tie),"Same-step finish order is incorrect");
 }
 {
  const double dt=1./120.;
  auto directory=std::filesystem::path("build/qa")/("contact-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(directory);auto log=directory/"race.csv";
  {std::ofstream previous(log);previous<<"previous race evidence\n";}
  {
   nz::RaceTelemetry telemetry(log);
   for(int kind=0;kind<3;kind++){
    nz::Vehicle p;nz::RaceSession r;r.Start(p,nz::Cars[4],true,false);r.phase=nz::RacePhase::Racing;r.trafficSeed=100+kind;
    p.speed=35;r.opponent.speed=20;r.opponent.distance=100;
    nz::TrafficWorld world;auto other=nz::RaceSession::Observer(r.opponent);world.Reset(nz::RaceSession::Observer(p),r.trafficSeed,&other);world.cars.clear();
    if(kind<2){
     nz::TrafficCar a;a.id=100000;a.lane=a.targetLane=kind==0?1:2;a.lateral=kind==0?p.lateral:r.opponent.lateral;a.station=(kind==0?p.distance:r.opponent.distance)+3.5;a.speed=a.cruise=15;a.decision=10;world.cars.push_back(a);
    }else{r.opponent.distance=p.distance+3.5;r.opponent.lateral=p.lateral;}
    telemetry.Record(r,p,4);r.Step(p,nz::Cars[4],{1,0,0},world,dt);
    Require(r.contactEvents.size()==1,"Contact category missing or duplicated");const auto e=r.contactEvents.front();
    Require((int)e.kind==kind&&e.counted,"Contact kind or count flag incorrect");
    Require(kind==0?r.playerTrafficContacts==1&&r.opponentContacts==0&&r.playerRivalContacts==0:kind==1?r.playerTrafficContacts==0&&r.opponentContacts==1&&r.playerRivalContacts==0:r.playerTrafficContacts==0&&r.opponentContacts==0&&r.playerRivalContacts==1,"Contact categories contaminate each other's counters");
    Require(kind==1?e.rivalAfter.speed<e.rivalBefore.speed:e.playerAfter.speed<e.playerBefore.speed,"Contact snapshots miss speed loss");
    Require(e.time==r.elapsed,"Contact timestamp is not the physics tick");
    telemetry.Record(r,p,4);telemetry.Record(r,p,4); // Same event must not be duplicated.
    if(kind==2){
     r.opponent.distance=p.distance+3.5;r.opponent.lateral=p.lateral;
     r.Step(p,nz::Cars[4],{1,0,0},world,dt);Require(r.playerRivalContacts==1&&std::abs(r.playerRivalContactSeconds-2*dt)<1e-9&&!r.contactEvents.back().counted,"Sustained push inflates counter or loses contact duration");
     telemetry.Record(r,p,4);r.phase=nz::RacePhase::Finished;telemetry.Record(r,p,4);
     r.Step(p,nz::Cars[4],{},world,dt);Require(r.contactEvents.empty(),"Finished race replays contact events");
    }
    r.Start(p,nz::Cars[4],true,false);Require(r.playerTrafficContacts==0&&r.playerRivalContacts==0&&r.opponentContacts==0&&r.playerRivalContactSeconds==0&&r.contactEvents.empty(),"Rematch retains contact history");
   }
  }
  std::ifstream events(directory/"race_contacts.csv");std::string line;int count=0,fields=0;
  while(std::getline(events,line)){int n=1+(int)std::count(line.begin(),line.end(),',');if(!count)fields=n;Require(n==fields,"Contact CSV header/row mismatch");++count;}
  Require(count==5,"Exact contact rows missing or duplicate recording bypasses tick guard");
  std::ifstream summary(log);count=0;while(std::getline(summary,line)){int n=1+(int)std::count(line.begin(),line.end(),',');if(!count)fields=n;Require(n==fields,"Summary CSV header/row mismatch");++count;}
  Require(count>=5,"Contact/terminal telemetry replaces or loses summary rows");
  bool preserved=false;for(const auto &file:std::filesystem::directory_iterator(directory/"race_history")){std::ifstream old(file.path());std::getline(old,line);preserved=preserved||line=="previous race evidence";}
  Require(preserved,"Opening a new telemetry log destroys prior race evidence");
  std::puts("PASS contact telemetry: three categories, speed/position snapshots, tick events, push duration, cooldown, rematch, terminal row, CSV schema and archived evidence");
 }
 int totalContacts=0;
 for(unsigned seed:{11u,42u,197u,7u,12345u,901u}){
  nz::Vehicle player;nz::RaceSession race;race.Start(player,nz::Cars[0],true,false);nz::TrafficWorld traffic;auto other=nz::RaceSession::Observer(race.opponent);traffic.Reset(nz::RaceSession::Observer(player),seed,&other);
  for(int tick=0;tick<120*420&&race.phase!=nz::RacePhase::Finished;tick++){
   race.Step(player,nz::Cars[0],{1,0,0},traffic,1./120.);auto rival=nz::RaceSession::Observer(race.opponent);Require(traffic.cars.size()<=nz::TrafficWorld::PopulationLimit(nz::RaceSession::Observer(player),&rival),"Two-racer traffic population is unbounded");Require(race.opponent.speed>=0&&std::abs(race.opponent.lateral)<=6.05,"Opponent leaves the road or reverses");
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
