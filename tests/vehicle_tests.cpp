#include "vehicle.h"
#include "scene.h"
#include <cstdio>
#include <stdexcept>
static void Require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
int main(){try{
 // Wide/tall and odd pixel sizes fill the window without distorting the scene.
 for(auto size:{std::pair<int,int>{1280,720},{2560,1080},{3840,1080},{960,900},{1365,767},{800,450}}){
  auto view=nz::MakeViewport(size.first,size.second);
  Require(view.width>=640&&view.height>=360&&view.scale>=1,"Viewport clips garage controls");
  Require(view.width*view.scale>=size.first&&view.width*view.scale-size.first<view.scale,"Viewport leaves horizontal borders");
  Require(view.height*view.scale>=size.second&&view.height*view.scale-size.second<view.scale,"Viewport leaves vertical borders");
  auto fit=nz::FitDrivingView(view.width,view.height);
  for(float forward:{-nz::VisibleBehind,nz::VisibleAhead})for(float lateral:{-7.2f,7.2f})for(float z:{0.f,1.5f}){
   auto camera=nz::NortheastCamera(forward,lateral,z);float px=fit.x+camera[0]*fit.pixelsPerMetre,py=fit.y-camera[1]*fit.pixelsPerMetre;
   Require(px>=0&&px<view.width&&py>=0&&py<view.height,"Common visible highway range is clipped");
  }
  // Select all street lamps for the actual zoomed/padded viewport across spawn boundaries.
  float lo=1e9f,hi=-1e9f;
  for(float x:{-128.f,(float)view.width+128})for(float y:{-128.f,(float)view.height+128}){
   auto fit=nz::FitDrivingView(view.width,view.height);auto ground=nz::NortheastGround((x-fit.x)/fit.pixelsPerMetre,(fit.y-y)/fit.pixelsPerMetre);lo=std::min(lo,ground.first);hi=std::max(hi,ground.first);
  }
  for(int tick=0;tick<100;tick++){
   int count=3;for(int side:{-1,1}){auto range=nz::StreetRange(tick*.137,lo,hi,side);count+=range.second-range.first+1;}
   Require(count<=32,"Wide viewport exhausts light capacity");
  }
 }
 auto wide=nz::MakeViewport(2560,1080);Require(wide.width>640&&wide.height==360,"Ultrawide viewport dimension invalid");
 auto orbitA=nz::GarageKeyLight(0),orbitB=nz::GarageKeyLight((float)(nz::Pi/.55)),orbitFull=nz::GarageKeyLight((float)(2*nz::Pi/.55));
 Require(orbitA[0]*orbitB[0]<0&&std::abs(orbitA[0]-orbitFull[0])<.001,"Garage light fails to orbit");
 for(int i=0;i<100;i++){auto light=nz::GarageKeyLight(i*.25f);Require(std::abs(light[1]*nz::CameraCos+light[2]*nz::CameraSin-3.2f)<.001,"Garage light height drifts");}
 // Projection must advance northeast while positive steering remains screen-right.
 auto forward=nz::NortheastCamera(1,0,0),right=nz::NortheastCamera(0,1,0);
 Require(forward[0]>0&&forward[1]>0&&right[0]>0&&right[1]<0,"Driving orientation invalid");
 for(float a:{-20.f,0.f,20.f})for(float b:{-7.f,0.f,7.f}){
  auto c=nz::NortheastCamera(a,b,0);auto ground=nz::NortheastGround(c[0],c[1]);
  Require(std::abs(ground.first-a)<.0001&&std::abs(ground.second-b)<.0001,"Projection roundtrip failed");
 }
 // Check every candidate world post across spawn boundaries, including far outside the screen.
 for(int tick=0;tick<1000;tick++){
  double distance=tick*.137;int total=2;
  for(int side:{-1,1}){
   auto [first,last]=nz::StreetRange(distance,-22,36,side);total+=last-first+1;
   for(int k=-10;k<30;k++){
    double s=k*24.+(side>0?12:0)-distance;
    if(s>=-22-25.3&&s<=36+25.3)Require(k>=first&&k<=last,"Contributing offscreen lamp missing");
   }
  }
  Require(total<=24,"Lamp capacity exceeded");
 }
 Require(nz::StreetFalloff(25.3)==0&&nz::StreetFalloff(25.299)<1e-8,"Lamp tail pops at despawn");
 Require(nz::LateralSpeedLimit(30./3.6)>2.5&&nz::LateralSpeedLimit(30./3.6)>nz::LateralSpeedLimit(160./3.6),"Steering speed envelope invalid");
 // Twin beams are split close to the bumper and rotate with the same headlamp basis.
 Require(nz::HeadlampBeam(4,.56f,0)>nz::HeadlampBeam(4,0,0)*3,"Headlight beams merge at the bumper");
 for(float yaw:{-.122f,.122f}){
  auto a=nz::HeadlampPosition(yaw,-1),b=nz::HeadlampPosition(yaw,1);
  Require(std::abs(std::hypot(a.first-b.first,a.second-b.second)-1.12f)<.0001,"Headlamp spacing changes with yaw");
  float forward=5,lateral=.56f;
  float rotatedS=forward*std::cos(yaw)-lateral*std::sin(yaw),rotatedL=forward*std::sin(yaw)+lateral*std::cos(yaw);
  Require(std::abs(nz::HeadlampBeam(forward,lateral,0)-nz::HeadlampBeam(rotatedS,rotatedL,yaw))<.0001,"Beam does not follow vehicle rotation");
 }
 for(double kmh:{5.,15.,30.,60.,100.,160.,180.}){
  double speed=kmh/3.6,previous=std::min(speed*.32,1.7+3./(1+std::pow(speed/21.,2)));
  Require(std::abs(nz::LateralSpeedLimit(speed)-2*previous)<1e-12,"Steering movement speed is not doubled across speed range");
  for(const auto &car:nz::Cars){
   nz::Vehicle turn;turn.speed=speed;turn.throttle=1;nz::Step(turn,car,{1,0,1},1./120.);
   double expected=1-std::exp(-2*car.steerResponse/(1+speed/65.)/120.);
   Require(std::abs(turn.steering-expected)<1e-12,"Steering input response is not doubled");
   nz::Vehicle release;release.speed=speed;release.steering=1;nz::Step(release,car,{},1./120.);
   Require(std::abs(release.steering-std::exp(-48./120.))<1e-12,"Steering release response is not doubled");
  }
 }
 // Evasive manoeuvres: lift-off improves response, braking improves it further.
 for(const auto &car:nz::Cars)for(double kmh:{30.,100.,160.,250.}){
  std::array<double,3> moved{};
  for(int mode=0;mode<3;mode++){
   nz::Vehicle v;v.lateral=0;v.speed=kmh/3.6;v.throttle=mode==0?1:0;
   for(int tick=0;tick<42;tick++){v.speed=kmh/3.6;nz::Step(v,car,{mode==0?1.:0.,mode==2?1.:0.,1},1./120.);}
   moved[mode]=v.lateral;
  }
  Require(moved[1]>moved[0]*1.1,"Lift-off does not improve evasive steering");
  Require(moved[2]>moved[1]*1.1,"Braking does not improve steering more than lift-off");
  std::printf("Evasion %s %.0fkm/h 350ms: throttle %.2fm coast %.2fm brake %.2fm\n",car.id,kmh,moved[0],moved[1],moved[2]);
 }
 for(const auto &car:nz::Cars){
  nz::Vehicle stop;stop.speed=100./3.6;stop.gear=3;stop.rpm=3000;
  for(int tick=0;tick<120*6&&stop.speed>.01;tick++)nz::Step(stop,car,{0,1,0},1./120.);
  Require(stop.speed<.01&&stop.distance>32&&stop.distance<38,"Gameplay braking misses 100-0 target");
  nz::Vehicle full,half;full.speed=half.speed=25;
  for(int t=0;t<60;t++){nz::Step(full,car,{0,1,0},1./120.);nz::Step(half,car,{0,.5,0},1./120.);}
  Require(full.speed<half.speed,"Partial pedal pressure is not progressive");
 }
 for(const auto &car:nz::Cars){
  nz::Vehicle stock,deleted;deleted.speedLimiterDeleted=true;
  for(int tick=0;tick<120*120;tick++){nz::Step(stock,car,{1,0,0},1./120.);nz::Step(deleted,car,{1,0,0},1./120.);}
  if(car.speedLimitKmh>0)Require(deleted.speed*3.6>210&&deleted.speed>stock.speed+8&&!deleted.limiter,"Limiter delete fails to remove the road-speed governor");
  else Require(std::abs(deleted.speed-stock.speed)<1e-9,"Limiter toggle changes an ungoverned car");
  nz::Vehicle first;first.automatic=false;first.speedLimiterDeleted=true;
  for(int tick=0;tick<120*30;tick++)nz::Step(first,car,{1,0,0},1./120.);
  double coupled=first.speed/(2*nz::Pi*car.radius)*60*car.gears[0]*car.finalDrive;
  Require(first.gear==0&&first.rpm<=car.redline&&coupled<car.redline*1.01,"Speed limiter delete also removed engine rev protection");
  std::printf("Limiter delete %s: stock %.1f / deleted %.1f km/h, rev limiter retained\n",car.id,stock.speed*3.6,deleted.speed*3.6);
 }
 // R34 has six usable ratios and AWD traction without changing legacy cars.
 {
  const auto &gtr=nz::Cars[4];Require(gtr.gearCount==6&&gtr.allWheelDrive,"R34 is missing its six-speed/AWD configuration");
  nz::Vehicle manual;manual.automatic=false;manual.gear=4;manual.speed=150/3.6;
  Require(nz::Shift(manual,gtr,1)&&manual.gear==5,"R34 cannot select sixth gear");
  manual.shiftTime=manual.shiftCooldown=0;Require(!nz::Shift(manual,gtr,1),"R34 shifts beyond sixth");
  nz::Vehicle automatic;automatic.gear=4;automatic.speedLimiterDeleted=true;automatic.speed=7900/(gtr.gears[4]*gtr.finalDrive)/60*(2*nz::Pi*gtr.radius);automatic.throttle=1;
  for(int i=0;i<80;i++)nz::Step(automatic,gtr,{1,0,0},1./120.);
  Require(automatic.gear==5,"R34 automatic fails to select sixth");
  auto rearOnly=gtr;rearOnly.allWheelDrive=false;nz::Vehicle awd,rwd;
  for(int i=0;i<240;i++){nz::Step(awd,gtr,{1,0,0},1./120.);nz::Step(rwd,rearOnly,{1,0,0},1./120.);}
  Require(awd.speed>rwd.speed,"R34 AWD does not use both axles for launch traction");
 }
 for(const auto &car:nz::Cars){
  // Request power while still well above the former 2600rpm downshift threshold.
  nz::Vehicle kick;kick.gear=4;kick.throttle=1;kick.rpm=3500;
  double fixedSpeed=3500/(car.gears[4]*car.finalDrive)/60*(2*nz::Pi*car.radius);kick.speed=fixedSpeed;
  nz::Step(kick,car,{1,0,0},1./120.);Require(kick.gear==4,"Automatic kickdown has no decision delay");
  for(int t=0;t<28;t++){kick.speed=fixedSpeed;nz::Step(kick,car,{1,0,0},1./120.);}
  Require(kick.gear<4&&kick.shiftTime>0,"Automatic fails to kick down for useful power above 2600rpm");
  double selectedRpm=fixedSpeed/(2*nz::Pi*car.radius)*60*car.gears[kick.gear]*car.finalDrive;
  Require(selectedRpm<car.shiftRpm*.93,"Automatic kickdown lands too close to an immediate upshift");
  nz::Vehicle steady;steady.gear=kick.gear;steady.speed=fixedSpeed;steady.throttle=1;
  for(int t=0;t<120*4;t++){steady.speed=fixedSpeed;nz::Step(steady,car,{1,0,0},1./120.);}
  Require(steady.shifts==0,"Automatic hunts at a fixed passing speed");
  nz::Vehicle cruise;cruise.gear=4;cruise.speed=fixedSpeed;
  for(int t=0;t<120;t++){cruise.speed=fixedSpeed;nz::Step(cruise,car,{},1./120.);}
  Require(cruise.gear==4,"Lift-off needlessly triggers power-band kickdown");
  nz::Vehicle hand;hand.automatic=false;hand.gear=4;hand.speed=fixedSpeed;
  Require(nz::Shift(hand,car,-1)&&hand.gear==3&&hand.shiftTime<.30,"Manual downshift loses its timing advantage");
  hand.shiftTime=hand.shiftCooldown=0;Require(nz::Shift(hand,car,1)&&hand.shiftTime<.34,"Manual upshift loses its timing advantage");
 }
 for(const auto &car:nz::Cars){
  nz::Vehicle manual;manual.automatic=false;
  for(int i=0;i<120*15;i++)nz::Step(manual,car,{1,0,0},1./120.);
  Require(manual.gear==0&&manual.rpm>car.redline*.95,"Manual gearbox shifted itself or failed to hold revs");
  Require(nz::Shift(manual,car,1)&&manual.gear==1&&manual.shiftTime>0,"Manual upshift failed");
  Require(!nz::Shift(manual,car,1),"Shift cooldown can be bypassed");
  manual.shiftTime=manual.shiftCooldown=0;manual.speed=140./3.6;manual.gear=2;
  Require(!nz::Shift(manual,car,-1)&&manual.gear==2,"Over-rev downshift accepted");
  manual.speed=15./3.6;Require(nz::Shift(manual,car,-1)&&manual.gear==1,"Safe downshift rejected");
  manual.shiftTime=manual.shiftCooldown=0;manual.gear=car.gearCount-1;Require(!nz::Shift(manual,car,1),"Upshift past top gear accepted");
  manual.gear=0;Require(!nz::Shift(manual,car,-1),"Downshift below first accepted");
  nz::Vehicle suspension;
  for(int i=0;i<60;i++)nz::Step(suspension,car,{1,0,0},1./120.);
  Require(suspension.pitch>.75&&suspension.pitch<=3,"Acceleration squat missing");
  suspension.speed=25;
  for(int i=0;i<80;i++)nz::Step(suspension,car,{0,1,0},1./120.);
  Require(suspension.pitch<-.75&&suspension.pitch>=-3,"Braking dive missing");
  suspension.speed=0;
  for(int i=0;i<120*3;i++)nz::Step(suspension,car,{0,1,0},1./120.);
  Require(std::abs(suspension.pitch)<.01,"Suspension remains pitched at rest");
  nz::Vehicle rest;for(int i=0;i<1200;i++)nz::Step(rest,car,{},1./120.);
  Require(rest.speed==0 && rest.lateral==1.75,"Car moved without input");
  nz::Vehicle v;for(int i=0;i<120*70;i++)nz::Step(v,car,{1,0,0},1./120.);
  Require(v.timeTo100>(car.allWheelDrive?4.7:car.speedLimitKmh>0?5.5:4.5) && v.timeTo100<10.5,"Unreasonable 0-100 acceleration");
  double topSpeed=v.speed*3.6;
  if(car.speedLimitKmh>0)Require(topSpeed>178&&topSpeed<181,"Stock limiter failed");else Require(topSpeed>275&&topSpeed<305&&!v.limiter,"Testarossa incorrectly speed-limited or top-speed tuning invalid");
  Require(v.gear>=3 && v.shifts>=3,"Automatic shifting failed");
  v.speed=100./3.6;v.lateralSpeed=0;double start=v.distance,seconds=0;
  while(v.speed>.01 && seconds<10){nz::Step(v,car,{0,1,0},1./120.);seconds+=1./120.;}
  Require(v.speed<.1 && v.distance-start>30 && v.distance-start<50,"100-0 braking invalid");
  double lowRate=0,highRate=0;
  for(double speed:{30./3.6,160./3.6}){
   nz::Vehicle steering;steering.lateral=0;
   for(int i=0;i<120;i++){steering.speed=speed;steering.lateral=0;nz::Step(steering,car,{0,0,1},1./120.);}
   if(speed<10)lowRate=steering.lateralSpeed;else highRate=steering.lateralSpeed;
   double initial=steering.lateralSpeed;
   for(int i=0;i<60;i++){steering.speed=speed;steering.lateral=0;nz::Step(steering,car,{},1./120.);}
   Require(std::abs(steering.lateralSpeed)<.1&&steering.lateralSpeed<initial,"Steering release drifts");
  }
  Require(lowRate>2.5&&lowRate>highRate,"Low speed response is slower than high speed");
  nz::Vehicle stationary;for(int i=0;i<120;i++)nz::Step(stationary,car,{0,0,1},1./120.);
  Require(stationary.lateralSpeed==0&&stationary.lateral==1.75,"Stationary steering slides the car");
  std::printf("Steering %s: 30km/h %.2fm/s, 160km/h %.2fm/s\n",car.id,lowRate,highRate);
  nz::Vehicle a,b;a.speed=b.speed=25;a.lateral=b.lateral=0;
  for(int i=0;i<120;i++){nz::Step(a,car,{.3,0,1},1./120.);nz::Step(b,car,{.3,0,-1},1./120.);}
  Require(a.lateral>1 && std::abs(a.lateral+b.lateral)<1e-6,"Steering symmetry failed");
  for(int i=0;i<120*10;i++)nz::Step(a,car,{1,0,1},1./120.);
  Require(std::abs(a.lateral)<=6.05,"Highway bounds failed");
  nz::Vehicle x,y;for(int i=0;i<1200;i++)nz::Step(x,car,{1,0,0},1./120.);
  for(int i=0;i<2400;i++)nz::Step(y,car,{1,0,0},1./240.);
  Require(std::abs(x.speed-y.speed)<.2,"Physics timestep dependence too high");
  std::printf("PASS %s: 0-100 %.2fs, 100-0 %.1fm in %.2fs, stock max %.1fkm/h\n",car.name,v.timeTo100,v.distance-start,seconds,topSpeed);
 }
 std::puts("PASS: idle, throttle, gears, limiter, braking, continuous steering, road bounds timestep stability, speed-sensitive steering, release, northeast projection and persistent lamps, common lookahead, paired rotating beams, manual shifts and suspension pitch");return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
