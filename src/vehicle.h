#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace nz {
constexpr double Pi=3.14159265358979323846;
struct TorquePoint { double rpm,nm; };
struct CarSpec {
    const char *id,*name,*engine,*drive;
    double mass,peakPS,peakNm,wheelbase,radius,finalDrive,efficiency,cdArea;
    double idle,launchRpm,shiftRpm,redline,frontWeight,grip,brakeG,steerResponse;
    bool frontDrive,turbo;
    std::array<double,5> gears;
    std::vector<TorquePoint> torque;
    double speedLimitKmh=180.;bool popupHeadlights=false;
};
inline const std::array<CarSpec,3> Cars={{
    {"nissan_silvia_s13","NISSAN SILVIA S13 K'S","SR20DET  /  2.0 TURBO","RWD",
     1150,205,275,2.475,.3135,4.083,.85,.64,850,3400,7050,7500,.53,.95,.93,9.5,false,true,
     {3.321,1.902,1.308,1.000,.759},
     {{800,90},{1500,150},{2500,210},{3500,265},{4000,275},{5000,265},{6000,240},{6500,220},{7000,195},{7500,150}}},
    {"honda_civic_ek9","HONDA CIVIC TYPE R EK9","B16B  /  1.6 VTEC","FWD",
     1070,185,160,2.620,.29775,4.400,.87,.62,1000,5800,8450,8600,.62,1.00,.98,9.0,true,false,
     {3.230,2.105,1.458,1.107,.848},
     {{1000,80},{2000,104},{3000,118},{4500,128},{5500,136},{6000,145},{7000,155},{7500,160},{8000,159},{8200,158.4},{8500,150},{8600,140}}}
 ,
    {"ferrari_testarossa","FERRARI TESTAROSSA","F113  /  4.9 FLAT-12","RWD",
     1630,390,490,2.550,.328,3.210,.88,.72,1000,3200,6500,6800,.40,.98,.95,9.3,false,false,
     {3.139,2.104,1.526,1.167,.875},
     {{1000,230},{2000,330},{3000,420},{4000,475},{4500,490},{5000,478},{5500,465},{6000,450},{6300,435},{6500,410},{6800,370}},0.,true}
}};
inline double TorqueAt(const CarSpec &c,double rpm){
    if(rpm<=c.torque.front().rpm)return c.torque.front().nm;
    for(size_t i=1;i<c.torque.size();++i)if(rpm<=c.torque[i].rpm){
        auto a=c.torque[i-1],b=c.torque[i];return a.nm+(b.nm-a.nm)*(rpm-a.rpm)/(b.rpm-a.rpm);
    }
    return c.torque.back().nm;
}
struct Input { double throttle=0,brake=0,steer=0; };
struct Vehicle {
    double speed=0,distance=0,lateral=1.75,lateralSpeed=0,rpm=900;
    double throttle=0,brake=0,steering=0,boost=0,shiftTime=0,shiftCooldown=0,acceleration=0;
    double pitch=0,pitchVelocity=0;bool automatic=true,speedLimiterDeleted=false;
    double yaw=0,elapsed=0,timeTo100=-1;
    int gear=0,shifts=0,autoShiftTarget=-1;double autoShiftIntent=0;bool limiter=false;
};
inline double Approach(double value,double target,double rate,double dt){
    return value+(target-value)*(1-std::exp(-rate*dt));
}
inline bool Shift(Vehicle &v,const CarSpec &c,int direction){
    if(direction==0||v.shiftTime>0||v.shiftCooldown>0)return false;
    int next=v.gear+(direction>0?1:-1);
    if(next<0||next>4)return false;
    double targetRpm=v.speed/(2*Pi*c.radius)*60*c.gears[next]*c.finalDrive;
    if(direction<0&&targetRpm>c.redline*.985)return false;
    v.gear=next;++v.shifts;v.shiftTime=direction>0?.28:.22;v.shiftCooldown=.38;return true;
}
// Automatic kickdown compares usable wheel torque, rather than waiting for a
// universal low-RPM threshold. A conservative margin leaves manual timing useful.
inline int AutomaticGear(const Vehicle &v,const CarSpec &c,double wheelRpm){
    double rpm=wheelRpm*c.gears[v.gear]*c.finalDrive;
    if(rpm>c.shiftRpm&&v.gear<4&&v.throttle>.2)return v.gear+1;
    if(v.gear==0)return 0;
    if(v.throttle>.45&&v.brake<.05&&!(c.speedLimitKmh>0&&!v.speedLimiterDeleted&&v.speed*3.6>c.speedLimitKmh-1.)){
        int best=v.gear;double torque=TorqueAt(c,std::max(c.idle,rpm))*c.gears[v.gear],bestTorque=torque;
        for(int gear=v.gear-1;gear>=0;--gear){
            double candidate=wheelRpm*c.gears[gear]*c.finalDrive;
            if(candidate>c.shiftRpm*.92)continue;
            double pull=TorqueAt(c,std::max(c.idle,candidate))*c.gears[gear];
            if(pull>bestTorque){bestTorque=pull;best=gear;}
        }
        if(bestTorque>torque*1.14)return best;
    }
    double floor=c.shiftRpm*(v.brake>.1?.43:v.throttle>.15?.37:.28);
    double nextRpm=wheelRpm*c.gears[v.gear-1]*c.finalDrive;
    if(rpm<floor&&nextRpm<c.shiftRpm*.78)return v.gear-1;
    return v.gear;
}
// Highway positioning envelope: quick at urban speeds, progressively calmer at speed.
// Steering response is a game tuning parameter, separate from tyre grip and driven axle.
constexpr double SteeringResponsiveness=2.;
// Deliberate highway-game assists, separate from catalogue brake/tyre values.
constexpr double GameBrakeBoost=1.20;
inline double SteeringAssist(double throttle,double brake){
    brake=std::clamp(brake,0.,1.);throttle=std::clamp(throttle,0.,1.);
    return 1.+.30*(1.-throttle)*(1.-brake)+.80*brake;
}
inline double LateralSpeedLimit(double speed){
    return SteeringResponsiveness*std::min(speed*.32,1.7+3.0/(1+std::pow(speed/21.,2)));
}
inline void Step(Vehicle &v,const CarSpec &c,Input in,double dt){
    in.throttle=std::clamp(in.throttle,0.0,1.0);in.brake=std::clamp(in.brake,0.0,1.0);in.steer=std::clamp(in.steer,-1.0,1.0);
    const double mass=c.mass+75.; // One driver; catalogue curb mass already includes normal fluids.
    v.elapsed+=dt;v.shiftTime=std::max(0.,v.shiftTime-dt);v.shiftCooldown=std::max(0.,v.shiftCooldown-dt);
    v.throttle=Approach(v.throttle,in.brake>.05?0.:in.throttle,12.,dt);
    v.brake=Approach(v.brake,in.brake,28.,dt);
    const double assist=SteeringAssist(v.throttle,v.brake);
    v.steering=Approach(v.steering,in.steer,SteeringResponsiveness*(std::abs(in.steer)<.01?24.:assist*c.steerResponse/(1+v.speed/65.)),dt);
    const double wheelRpm=v.speed/(2*Pi*c.radius)*60;
    double coupledRpm=wheelRpm*c.gears[v.gear]*c.finalDrive;
    if(v.automatic&&v.shiftTime<=0&&v.shiftCooldown<=0){
        int target=AutomaticGear(v,c,wheelRpm);
        if(target==v.gear){v.autoShiftTarget=-1;v.autoShiftIntent=0;}
        else{
            if(target!=v.autoShiftTarget){v.autoShiftTarget=target;v.autoShiftIntent=0;}
            v.autoShiftIntent+=dt;
            bool up=target>v.gear;
            // A skilled manual shift starts immediately and completes faster.
            if(v.autoShiftIntent>=(up?.12:.20)){
                int skipped=std::max(0,std::abs(target-v.gear)-1);
                v.gear=target;++v.shifts;v.shiftTime=up?.34:.30+.04*skipped;v.shiftCooldown=up?.58:.65;
                v.autoShiftTarget=-1;v.autoShiftIntent=0;
            }
        }
    }else{v.autoShiftTarget=-1;v.autoShiftIntent=0;}
    coupledRpm=wheelRpm*c.gears[v.gear]*c.finalDrive;
    double engineRpm=std::max(c.idle,coupledRpm);
    if(v.gear==0 && v.throttle>.05)engineRpm=std::max(engineRpm,c.idle+(c.launchRpm-c.idle)*v.throttle);
    v.rpm=Approach(v.rpm,std::min(engineRpm,c.redline),20.,dt);
    const double boostTarget=v.throttle*std::clamp((v.rpm-1600)/1700,0.,1.);
    v.boost=Approach(v.boost,boostTarget,boostTarget>v.boost?4.:9.,dt);
    double force=TorqueAt(c,v.rpm)*c.gears[v.gear]*c.finalDrive*c.efficiency/c.radius*v.throttle;
    if(c.turbo)force*=.70+.30*v.boost;
    if(v.shiftTime>0)force=0.;
    if(coupledRpm>c.redline)force=0.;
    const bool governed=c.speedLimitKmh>0&&!v.speedLimiterDeleted;
    v.limiter=governed&&v.speed*3.6>c.speedLimitKmh-.5;
    if(governed)force*=std::clamp((c.speedLimitKmh+.5-v.speed*3.6)/1.0,0.,1.0);
    const double transfer=std::clamp(v.acceleration*.50/(9.81*c.wheelbase),-.18,.18);
    const double drivenWeight=c.frontDrive?c.frontWeight-transfer:1-c.frontWeight+transfer;
    force=std::min(force,c.grip*mass*9.81*std::clamp(drivenWeight,.25,.80));
    const double aero=.5*1.225*c.cdArea*v.speed*v.speed;
    const double rolling=v.speed>.005?.012*mass*9.81:0.;
    const double engineBrake=(1-v.throttle)*std::min(v.speed/6.,1.)*(170.+v.rpm*.035);
    const double braking=v.brake*c.brakeG*GameBrakeBoost*mass*9.81;
    const double oldSpeed=v.speed;
    v.acceleration=(force-aero-rolling-engineBrake-braking)/(mass*1.04); // Equivalent rotating wheel/driveline inertia.
    v.speed=std::max(0.,v.speed+v.acceleration*dt);
    // A damped suspension response: positive pitch lifts the nose, negative pitches into braking.
    // Suppress the nominal braking force once stationary, so a held brake does not leave a parked car diving.
    double actualAcceleration=(v.speed-oldSpeed)/dt;
    double targetPitch=std::clamp(actualAcceleration*.58,-3.,3.);
    v.pitchVelocity+=(100.*(targetPitch-v.pitch)-17.*v.pitchVelocity)*dt;
    v.pitch=std::clamp(v.pitch+v.pitchVelocity*dt,-3.,3.);
    v.distance+=(oldSpeed+v.speed)*.5*dt;
    if(v.timeTo100<0 && oldSpeed<100./3.6 && v.speed>=100./3.6)v.timeTo100=v.elapsed;
    const double targetLateral=v.steering*LateralSpeedLimit(v.speed)*(1.+.12*(1.-v.throttle)*(1.-v.brake)+.28*v.brake);
    const double physicalG=std::sqrt(std::max(.12,c.grip*c.grip-std::pow(std::clamp(v.acceleration/9.81,-c.grip,c.grip),2)));
    // Preserve evasive steering under braking rather than exhausting the lateral budget.
    const double availableG=std::max(physicalG,c.grip*.85*std::clamp((assist-1.)/.30,0.,1.));
    const double maxLateralAcceleration=SteeringResponsiveness*assist*availableG*9.81;
    const double response=SteeringResponsiveness*(std::abs(in.steer)<.01?22./(1+v.speed/100.):assist*std::max(5.,c.steerResponse*1.35/(1+v.speed/75.)));
    const double desiredAcceleration=(targetLateral-v.lateralSpeed)*response;
    v.lateralSpeed+=std::clamp(desiredAcceleration,-maxLateralAcceleration,maxLateralAcceleration)*dt;
    if(v.speed<.1)v.lateralSpeed=0;
    v.lateral+=v.lateralSpeed*dt;
    if(std::abs(v.lateral)>6.05){v.lateral=std::clamp(v.lateral,-6.05,6.05);v.lateralSpeed=0;v.speed=std::max(0.,v.speed-1.5*dt);}
    v.yaw=std::atan2(v.lateralSpeed,std::max(4.,v.speed));
}
}
