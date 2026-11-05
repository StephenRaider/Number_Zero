#pragma once
#include "race.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
namespace nz {
// Low-rate diagnostics live in ignored build/qa, never in the audio callback.
class RaceTelemetry {
 std::filesystem::path path;std::ofstream stream,events;unsigned tick=0,session=1;double lastDistance=-1,lastElapsed=-1;bool finished=false,hasSeed=false;std::uint32_t lastSeed=0;double lastEventElapsed=-1;
 static bool OpenLog(const std::filesystem::path &file,std::ofstream &out){
  std::error_code error;std::filesystem::create_directories(file.parent_path(),error);if(error)return false;
  bool exists=std::filesystem::exists(file,error);if(error)return false;
  auto bytes=exists?std::filesystem::file_size(file,error):0;if(error)return false;
  if(exists&&bytes>0){
   auto history=file.parent_path()/"race_history";std::filesystem::create_directories(history,error);if(error)return false;
   auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
   auto archived=history/(file.stem().string()+"_"+std::to_string(stamp)+file.extension().string());
   for(int suffix=1;std::filesystem::exists(archived);++suffix)archived=history/(file.stem().string()+"_"+std::to_string(stamp)+"_"+std::to_string(suffix)+file.extension().string());
   std::filesystem::copy_file(file,archived,error);
   if(error){std::fprintf(stderr,"Could not archive race log; preserving %s\n",file.string().c_str());return false;}
  }
  out.open(file);if(!out)return false;out.setf(std::ios::fixed);out.precision(3);return true;
 }
 void RecordContacts(const RaceSession &race,int car){
  if(lastEventElapsed==race.elapsed)return;
  lastEventElapsed=race.elapsed;
  if(!events.is_open()){
   if(!OpenLog(path.parent_path()/(path.stem().string()+"_contacts.csv"),events))return;
   events<<"session,seed,time,kind,counted,player_car,player_kmh_before,player_kmh_after,rival_kmh_before,rival_kmh_after,player_station_before,player_station_after,rival_station_before,rival_station_after,player_lateral_before,player_lateral_after,rival_lateral_before,rival_lateral_after,player_lateral_mps_before,rival_lateral_mps_before,gap_m_before,gap_m_after,relative_lateral_m_before,player_traffic_contacts,rival_traffic_contacts,player_rival_contacts\n";
  }
  for(const auto &e:race.contactEvents){
   const char *kind=e.kind==RaceContactKind::PlayerTraffic?"player_traffic":e.kind==RaceContactKind::RivalTraffic?"rival_traffic":"player_rival";
   events<<session<<','<<race.trafficSeed<<','<<e.time<<','<<kind<<','<<e.counted<<','<<(car>=0&&car<(int)Cars.size()?Cars[car].id:"unknown")
    <<','<<e.playerBefore.speed*3.6<<','<<e.playerAfter.speed*3.6<<','<<e.rivalBefore.speed*3.6<<','<<e.rivalAfter.speed*3.6
    <<','<<e.playerBefore.station<<','<<e.playerAfter.station<<','<<e.rivalBefore.station<<','<<e.rivalAfter.station
    <<','<<e.playerBefore.lateral<<','<<e.playerAfter.lateral<<','<<e.rivalBefore.lateral<<','<<e.rivalAfter.lateral
    <<','<<e.playerBefore.lateralSpeed<<','<<e.rivalBefore.lateralSpeed<<','<<e.rivalBefore.station-e.playerBefore.station<<','<<e.rivalAfter.station-e.playerAfter.station
    <<','<<e.rivalBefore.lateral-e.playerBefore.lateral<<','<<race.playerTrafficContacts<<','<<race.opponentContacts<<','<<race.playerRivalContacts<<'\n';
  }
  if(!race.contactEvents.empty())events.flush();
 }
public:
 explicit RaceTelemetry(std::filesystem::path file):path(std::move(file)){}
 void Record(const RaceSession &race,const Vehicle &player,int car=-1){
  if((hasSeed&&race.trafficSeed!=lastSeed)||race.elapsed<lastElapsed||(race.phase==RacePhase::Countdown&&player.distance<lastDistance-1)){++session;tick=0;lastEventElapsed=-1;}lastDistance=player.distance;lastElapsed=race.elapsed;lastSeed=race.trafficSeed;hasSeed=true;
  RecordContacts(race,car);
  if(race.phase!=RacePhase::Finished)finished=false;
  if(finished)return;
  if(tick++%30&&race.phase!=RacePhase::Finished)return;
  if(!stream.is_open()){
   if(!OpenLog(path,stream))return;
   stream<<"session,seed,time,phase,player_kmh,rival_kmh,player_m,rival_m,gap_m,throttle,brake,lateral,target_lane,front_gap_m,closing_mps,left_score,stay_score,right_score,rejected_paths,gap_preparation,contacts,player_car,player_slip,rival_slip,player_tcs,rival_tcs,player_clutch,player_wheel_kmh,player_throttle,player_assist,player_assist_active,player_tcs_active,player_esc_active,player_traffic_contacts,rival_traffic_contacts,player_rival_contacts,player_traffic_contact_s,rival_traffic_contact_s,player_rival_contact_s,player_lateral,player_lateral_mps,player_steer,player_brake,rival_lateral_mps,rival_decision,rival_side_contest,rival_forward_blocked\n";
   stream.setf(std::ios::fixed);stream.precision(3);
  }
  finished=race.phase==RacePhase::Finished;
  const auto &d=race.driver;
  stream<<session<<','<<race.trafficSeed<<','<<race.elapsed<<','<<(int)race.phase<<','<<player.speed*3.6<<','<<race.opponent.speed*3.6<<','<<race.PlayerProgress(player)<<','<<race.OpponentProgress()<<','<<race.opponent.distance-player.distance<<','<<race.opponentInput.throttle<<','<<race.opponentInput.brake<<','<<race.opponent.lateral<<','<<d.chosenLane<<','<<d.nearestFront<<','<<d.closingSpeed<<','<<d.laneScores[0]<<','<<d.laneScores[1]<<','<<d.laneScores[2]<<','<<d.rejectedPlans<<','<<d.PreparingGap()<<','<<race.opponentContacts<<','<<(car>=0&&car<(int)Cars.size()?Cars[car].id:"unknown")<<','<<player.slipRatio<<','<<race.opponent.slipRatio<<','<<player.tcsEnabled<<','<<race.opponent.tcsEnabled<<','<<player.clutch<<','<<DrivenWheelSpeed(player)*3.6<<','<<player.throttle<<','<<player.tractionAssist<<','<<player.assistActive<<','<<player.tcsActive<<','<<player.escActive<<','<<race.playerTrafficContacts<<','<<race.opponentContacts<<','<<race.playerRivalContacts<<','<<race.playerTrafficContactSeconds<<','<<race.rivalTrafficContactSeconds<<','<<race.playerRivalContactSeconds<<','<<player.lateral<<','<<player.lateralSpeed<<','<<player.steering<<','<<player.brake<<','<<race.opponent.lateralSpeed<<','<<d.decisionReason<<','<<d.sideContest<<','<<d.forwardBlocked<<'\n';
  if(tick%120<30)stream.flush();
 }
};
}
