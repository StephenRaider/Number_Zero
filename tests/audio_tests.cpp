#include "engine_sound.h"
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>
static void Require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static double Energy(const std::vector<float> &samples){double e=0;for(float s:samples)e+=s*s;return e/samples.size();}
static void Valid(const std::vector<float> &samples){for(float s:samples)Require(std::isfinite(s)&&std::abs(s)<=.85f,"Invalid or clipped synth sample");}
static double SteadyRms(int car,float rpm,float throttle){
 nz::EngineSound sound;std::vector<float> block(441);double energy=0;
 for(int chunk=0;chunk<300;chunk++){
  sound.Render(block.data(),(unsigned)block.size(),{rpm,throttle,car,true,false});Valid(block);
  if(chunk>=200)energy+=Energy(block);
 }
 return std::sqrt(energy/100);
}
static double BassFraction(float rpm){
 nz::EngineSound sound;std::vector<float> block(441);double total=0,bass=0;float a=1-std::exp(-2*(float)nz::Pi*300/nz::AudioRate),dc=1-std::exp(-2*(float)nz::Pi*25/nz::AudioRate),first=0,second=0,low=0;
 for(int chunk=0;chunk<200;chunk++){
  sound.Render(block.data(),(unsigned)block.size(),{rpm,1,2,true,false});
  for(float sample:block){first+=(sample-first)*a;second+=(first-second)*a;low+=(second-low)*dc;if(chunk>=100){bass+=(second-low)*(second-low);total+=sample*sample;}}
 }
 return bass/total;
}
// Proxy for lower exhaust orders; tonal authenticity still needs listening.
static double ExhaustBodyFraction(int car,float rpm){
 nz::EngineSound sound;std::vector<float> block(441);double total=0,body=0;
 float a=1-std::exp(-2*(float)nz::Pi*250/nz::AudioRate),dc=1-std::exp(-2*(float)nz::Pi*25/nz::AudioRate),first=0,second=0,low=0;
 for(int chunk=0;chunk<200;chunk++){
  sound.Render(block.data(),(unsigned)block.size(),{rpm,1,car,true,false});
  for(float v:block){first+=(v-first)*a;second+=(first-second)*a;low+=(second-low)*dc;if(chunk>=100){body+=(second-low)*(second-low);total+=v*v;}}
 }
 return body/total;
}
static double LowRpmVariation(carsynth::Profile profile,float rpm){
 carsynth::Engine engine(profile);float smallest=1e9f,largest=0;
 for(int i=0;i<6*nz::AudioRate;i++){
  float sample=engine.process(rpm,.4f);Require(std::isfinite(sample)&&std::abs(sample)<=.85f,"Low-RPM texture is invalid");
  if(i>=nz::AudioRate){smallest=std::min(smallest,engine.effectiveRpm());largest=std::max(largest,engine.effectiveRpm());}
 }
 return largest-smallest;
}
int main(){try{
 std::filesystem::create_directories("build/qa");
 for(int car=0;car<(int)nz::Cars.size();car++){
  double idle=SteadyRms(car,(float)nz::Cars[car].idle,0);
  for(float rpm:{3000.f,5000.f,7000.f}){
   double loaded=SteadyRms(car,rpm,1),coast=SteadyRms(car,rpm,0),ratio=coast/loaded;
   std::printf("Lift-off %s at %.0frpm: loaded %.3f, coast %.3f, ratio %.2f\n",nz::Cars[car].id,rpm,loaded,coast,ratio);
   Require(ratio>.25&&ratio<.95,"Lift-off volume collapses or loses load contrast");
   Require(coast>idle*1.25,"High RPM coast sounds like quiet idle");
  }
  nz::Vehicle v;v.rpm=car?8000:6000;v.throttle=.8;v.shiftTime=.2;
  auto c=nz::DrivingSound(v,car,true,false,false);
  Require(c.throttle==0,"Shift did not lift audio throttle");
  v.shiftTime=0;v.limiter=true;Require(nz::DrivingSound(v,car,true,false,false).throttle==0,"Speed cut did not lift audio throttle");
  c=nz::UnpackEngineControls(nz::PackEngineControls({8123.5f,.73f,car,true,true}));
  Require(c.rpm==8123.5f&&std::abs(c.throttle-.73f)<.00002&&c.car==car&&c.active&&c.muted,"Audio snapshot corrupted");
  nz::EngineSound sound;
  std::vector<float> buffer(1024);
  sound.Render(buffer.data(),(unsigned)buffer.size(),{900,0,car,false,false});Require(Energy(buffer)==0,"Garage is audible");
  v=nz::Vehicle{};v.rpm=nz::Cars[car].idle;
  std::vector<float> drive;drive.reserve(18*nz::AudioRate);double rms=0;float peak=0;
  for(int tick=0;tick<18*120;tick++){
   nz::Step(v,nz::Cars[car],{tick<1200?1.:0.,tick>=1200?1.:0.,0},1./120.);
   unsigned count=(unsigned)((tick+1)*nz::AudioRate/120-tick*nz::AudioRate/120);
   buffer.resize(count);sound.Render(buffer.data(),count,nz::DrivingSound(v,car,true,false,false));
   Valid(buffer);for(float s:buffer){rms+=s*s;peak=std::max(peak,std::abs(s));}drive.insert(drive.end(),buffer.begin(),buffer.end());
  }
  Require(peak>.02f&&rms/drive.size()>.000001,"Drive audio is silent");
  Require(carsynth::writeWavMono(car==3?"build/qa/supra_engine_drive.wav":car==2?"build/qa/testarossa_engine_drive.wav":car==1?"build/qa/civic_engine_drive.wav":"build/qa/silvia_engine_drive.wav",drive,nz::AudioRate),"Diagnostic WAV export failed");
  buffer.resize(nz::AudioRate/4);
  sound.Render(buffer.data(),(unsigned)buffer.size(),{5000,1,car,true,true});Valid(buffer);
  Require(std::abs(buffer.back())<1e-10,"Mute does not settle to silence");
  sound.Render(buffer.data(),(unsigned)buffer.size(),{5000,1,car,true,false});Require(Energy(buffer)>.000001,"Unmute remains silent");
  sound.Render(buffer.data(),(unsigned)buffer.size(),{5000,1,car,false,false});Require(std::abs(buffer.back())<1e-10,"Pause does not settle to silence");
  sound.Render(buffer.data(),(unsigned)buffer.size(),{5000,1,(car+1)%(int)nz::Cars.size(),true,false});Valid(buffer);Require(Energy(buffer)>.000001,"Switching cars is silent");
  std::printf("PASS %s: 18s drivetrain audio, peak %.3f, RMS %.3f; mute/pause/resume/car switch\n",nz::Cars[car].id,peak,std::sqrt(rms/drive.size()));
 }
 for(float rpm:{3000.f,5000.f,6500.f}){
  double body=ExhaustBodyFraction(3,rpm);
  std::printf("PASS Supra exhaust body at %.0frpm: %.2f\n",rpm,body);
  Require(body>.28,"Supra loses its lower exhaust orders and turns into a thin firing-frequency whine");
 }
 for(float rpm:{3000.f,5000.f,6500.f}){
  double fraction=BassFraction(rpm);Require(fraction>.30,"Ferrari lost its low exhaust body at high RPM");
  std::printf("PASS Ferrari low-body energy at %.0frpm: %.2f\n",rpm,fraction);
 }
 for(auto profile:{carsynth::profileS13(),carsynth::profileEK9(),nz::TestarossaProfile(),nz::SupraProfile()}){
  for(float rpm:{1500.f,2500.f,3000.f}){double variation=LowRpmVariation(profile,rpm);Require(variation>10&&variation<180,"Low-RPM combustion remains sterile or hunts excessively");std::printf("PASS low-RPM variation %s %.0frpm: %.1frpm range\n",profile.name,rpm,variation);}
  Require(LowRpmVariation(profile,6000)<10,"Low-RPM hunting leaks into high-RPM running");
 }
 // Observable cam state: loaded high RPM engages; lift disengages with smoothing.
 carsynth::Engine civic(carsynth::profileEK9());
 for(int i=0;i<nz::AudioRate;i++)civic.process(7000,1);
 Require(civic.vtecActive(),"VTEC did not engage");
 for(int i=0;i<nz::AudioRate/2;i++)civic.process(7000,0);
 Require(!civic.vtecActive(),"VTEC did not disengage on lift");
 // Compare audible output against an otherwise identical profile with lift layers disabled.
 auto base=carsynth::profileS13(),noBov=base;noBov.bovGain=0;noBov.flutterGain=0;
 carsynth::Engine turbo(base),plain(noBov);double liftDifference=0;
 for(int i=0;i<3*nz::AudioRate;i++){
  float throttle=i<2*nz::AudioRate?1.f:0.f;
  float a=turbo.process(6000,throttle),b=plain.process(6000,throttle);
  if(i>=2*nz::AudioRate)liftDifference+=std::abs(a-b);
 }
 Require(turbo.boost()>0&&liftDifference>.01,"S13 lift-off layers never triggered");
 std::printf("PASS VTEC engagement/release and boosted S13 lift-off (output difference %.3f)\n",liftDifference);
 return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
