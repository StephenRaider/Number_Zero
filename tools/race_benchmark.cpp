#include "race.h"
#include <cstdio>
#include <filesystem>
int main(int argc,char **argv){
 std::filesystem::create_directories("build/qa");const char *tag=argc>1?argv[1]:"current";
 char path[180];std::snprintf(path,sizeof(path),"build/qa/rival_%s.csv",tag);FILE *log=std::fopen(path,"w");if(!log)return 1;
 std::fprintf(log,"seed,time,player_kmh,rival_kmh,player_m,rival_m,rival_throttle,rival_brake,rival_lateral,rival_lateral_speed,near_front_m,closing_mps,lane_changes,gap_preparations,contacts,chosen_lane,rejected_plans,left_score,stay_score,right_score\n");
 bool stalled=false;
 for(unsigned seed:{11u,42u,197u,7u,12345u,901u}){
  nz::Vehicle p;nz::RaceSession r;r.Start(p,nz::Cars[1],true,false);nz::TrafficWorld traffic;auto o=nz::RaceSession::Observer(r.opponent);traffic.Reset(nz::RaceSession::Observer(p),seed,&o);
  nz::RivalDriver civic;civic.Configure(nz::Cars[1]);civic.Reset(1);double brake=0,slow=0,boxed=0;
  for(int tick=0;tick<120*450&&r.phase!=nz::RacePhase::Finished;tick++){
   auto input=civic.Control(p,nz::ObserveRaceTraffic(p,traffic.cars,nz::RaceSession::Observer(r.opponent)),1./120.);
   r.Step(p,nz::Cars[1],input,traffic,1./120.);
   if(r.phase==nz::RacePhase::Racing){brake+=(r.opponentInput.brake>.05)/120.;slow+=(r.opponent.speed<100/3.6)/120.;boxed+=r.driver.PreparingGap()/120.;}
   if(tick%30==0){double nearest=1e6,closing=0;for(const auto &a:nz::ObserveRaceTraffic(r.opponent,traffic.cars,nz::RaceSession::Observer(p)))if(a.station>0&&std::abs(a.lateral-r.opponent.lateral)<(1.976+a.width)*.5+.18&&a.station<nearest){nearest=a.station;closing=r.opponent.speed-a.speed;}
    std::fprintf(log,"%u,%.3f,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f\n",seed,r.elapsed,p.speed*3.6,r.opponent.speed*3.6,r.PlayerProgress(p),r.OpponentProgress(),r.opponentInput.throttle,r.opponentInput.brake,r.opponent.lateral,r.opponent.lateralSpeed,nearest,closing,r.driver.laneChanges,r.driver.gapPreparations,r.opponentContacts,r.driver.chosenLane,r.driver.rejectedPlans,r.driver.laneScores[0],r.driver.laneScores[1],r.driver.laneScores[2]);
   }
  }
  stalled=stalled||r.phase!=nz::RacePhase::Finished;
  std::printf("seed %u winner %s time %.1f civic %.0fm rival %.0fm brake %.1fs sub100 %.1fs yielding %.1fs contacts %d\n",seed,r.winner==nz::RaceWinner::Player?"Civic":"Ferrari",r.elapsed,r.PlayerProgress(p),r.OpponentProgress(),brake,slow,boxed,r.opponentContacts);
 }
 std::fclose(log);return stalled?1:0;
}
