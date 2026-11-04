#pragma once
#include "race.h"
#include <filesystem>
#include <fstream>
namespace nz {
// Low-rate diagnostics live in ignored build/qa, never in the audio callback.
class RaceTelemetry {
 std::filesystem::path path;std::ofstream stream;unsigned tick=0,session=1;double lastDistance=-1,lastElapsed=-1;bool finished=false;
public:
 explicit RaceTelemetry(std::filesystem::path file):path(std::move(file)){}
 void Record(const RaceSession &race,const Vehicle &player){
  if(race.elapsed<lastElapsed||(race.phase==RacePhase::Countdown&&player.distance<lastDistance-1)){++session;tick=0;}lastDistance=player.distance;lastElapsed=race.elapsed;
  if(race.phase!=RacePhase::Finished)finished=false;
  if(finished)return;
  if(tick++%30)return;
  if(!stream.is_open()){
   std::error_code error;std::filesystem::create_directories(path.parent_path(),error);stream.open(path);
   if(!stream)return;
   stream<<"session,time,phase,player_kmh,rival_kmh,player_m,rival_m,gap_m,throttle,brake,lateral,target_lane,front_gap_m,closing_mps,left_score,stay_score,right_score,rejected_paths,gap_preparation,contacts\n";
   stream.setf(std::ios::fixed);stream.precision(3);
  }
  finished=race.phase==RacePhase::Finished;
  const auto &d=race.driver;
  stream<<session<<','<<race.elapsed<<','<<(int)race.phase<<','<<player.speed*3.6<<','<<race.opponent.speed*3.6<<','<<race.PlayerProgress(player)<<','<<race.OpponentProgress()<<','<<race.opponent.distance-player.distance<<','<<race.opponentInput.throttle<<','<<race.opponentInput.brake<<','<<race.opponent.lateral<<','<<d.chosenLane<<','<<d.nearestFront<<','<<d.closingSpeed<<','<<d.laneScores[0]<<','<<d.laneScores[1]<<','<<d.laneScores[2]<<','<<d.rejectedPlans<<','<<d.PreparingGap()<<','<<race.opponentContacts<<'\n';
  if(tick%120<30)stream.flush();
 }
};
}
