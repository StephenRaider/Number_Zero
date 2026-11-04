#pragma once
#include "vehicle.h"
#include "../third_party/car_engine_synth/car_engine_synth.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace nz {
constexpr int AudioRate=44100;
struct EngineControls {float rpm=850,throttle=0;int car=0;bool active=false,muted=false;};
// Publish one coherent snapshot; the audio thread never reads the mutable vehicle.
inline std::uint64_t PackEngineControls(EngineControls c){
 c.rpm=std::clamp(c.rpm,0.f,12000.f);c.throttle=std::clamp(c.throttle,0.f,1.f);
 std::uint32_t rpmBits;std::memcpy(&rpmBits,&c.rpm,sizeof(rpmBits));
 return rpmBits | (std::uint64_t(std::lround(c.throttle*65535.f))<<32)
  | (std::uint64_t(std::clamp(c.car,0,2))<<48) | (std::uint64_t(c.active)<<50) | (std::uint64_t(c.muted)<<51);
}
inline EngineControls UnpackEngineControls(std::uint64_t bits){
 EngineControls c;std::uint32_t rpmBits=(std::uint32_t)bits;std::memcpy(&c.rpm,&rpmBits,sizeof(c.rpm));
 c.throttle=float((bits>>32)&65535)/65535.f;c.car=int((bits>>48)&3);c.active=(bits>>50)&1;c.muted=(bits>>51)&1;return c;
}
inline EngineControls DrivingSound(const Vehicle &v,int car,bool driving,bool paused,bool muted){
 // A shift/factory speed cut removes engine load even if the pedal remains held.
 return {(float)v.rpm,(float)((v.shiftTime>0||v.limiter)?0.:v.throttle),car,driving&&!paused,muted};
}
inline carsynth::Profile TestarossaProfile(){
 auto p=carsynth::profileEK9();p.name="Ferrari Testarossa F113 flat-12";p.cylinders=12;p.idleRpm=1000;p.limiterRpm=6800;p.vtec=false;p.turbo=false;
 p.pulseMs=1.55f;p.jitter=.045f;p.idleLump=.008f;p.bodyGain=2.1f;p.bankBodyGain=.70f;
 p.lowpassClosed=580;p.lowpassOpen=1800;p.muffleHz=1750;p.intakeHz=520;p.intakeQ=.8f;p.intakeGain=.025f;p.mechGain=.001f;p.outGain=.90f;
 const float hz[]={90,180,430,1100},q[]={.95f,1.15f,1.35f,.9f},g[]={1.6f,1.4f,.45f,.08f},accent[]={1.f,.97f,1.03f,.99f};
 for(int i=0;i<4;i++){p.firingGain[i]=accent[i];p.formantHz[i]=hz[i];p.formantQ[i]=q[i];p.formantGain[i]=g[i];}return p;
}
class EngineSound {
 std::array<carsynth::Engine,3> engines{carsynth::Engine(carsynth::profileS13(),AudioRate),carsynth::Engine(carsynth::profileEK9(),AudioRate),carsynth::Engine(TestarossaProfile(),AudioRate)};
 int currentCar=0;float gain=0;
public:
 // Single audio-thread owner. No allocation, locks, filesystem or Raylib calls here.
 void Render(float *out,unsigned frames,EngineControls controls){
  for(unsigned i=0;i<frames;i++){
   bool switching=controls.car!=currentCar;
   float target=controls.active&&!controls.muted&&!switching?.70f:0.f;
   gain+=(target-gain)*.0045f; // ~5 ms edge fade, independent of render-frame stalls.
   if(switching&&gain<.0001f){currentCar=controls.car;gain=0;}
   float sample=0;
   if(controls.active||gain>.00001f){
    bool selected=currentCar==controls.car;
    float rpm=selected?controls.rpm:engines[currentCar].profile().idleRpm;
    float throttle=selected?controls.throttle:0.f;
    sample=engines[currentCar].process(rpm,throttle)*gain;
   }
   out[i]=sample;
  }
 }
};
}
