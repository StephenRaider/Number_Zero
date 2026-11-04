// Standstill and 50km/h rolling comparisons using the actual fixed-step model.
// No traffic, steering or reaction delay; governor deleted only to measure 200.
#include "vehicle.h"
#include <cstdio>
int main(){
 std::puts("car,start_kmh,to30_s,to50_s,to60mph_s,to100_s,to160_s,to200_s");
 for(const auto &car:nz::Cars)for(double start:{0.,50.}){
  nz::Vehicle v;v.speed=start/3.6;v.speedLimiterDeleted=true;
  if(start>0){v.gear=1;v.rpm=v.speed/(2*nz::Pi*car.radius)*60*car.gears[1]*car.finalDrive;}
  double times[6]={};const double marks[]={30,50,60*1.609344,100,160,200};
  for(int tick=0;tick<120*60;tick++){
   double before=v.speed*3.6;nz::Step(v,car,{1,0,0},1./120.);
   for(int j=0;j<6;j++)if(marks[j]>start&&times[j]==0&&v.speed*3.6>=marks[j])times[j]=(tick+(marks[j]-before)/(v.speed*3.6-before))/120.;
  }
  std::printf("%s,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",car.id,start,times[0],times[1],times[2],times[3],times[4],times[5]);
 }
}
