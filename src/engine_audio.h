#pragma once
#include "raylib.h"
#include "engine_sound.h"
#include <atomic>
#include <cstdio>

namespace nz {
class EngineAudio {
 AudioStream stream{};bool device=false,ready=false;
 EngineSound synth;
 std::atomic<std::uint64_t> controls{PackEngineControls({})};
 std::atomic<std::uint64_t> samples{0};
 inline static EngineAudio *owner=nullptr;
 static void Callback(void *buffer,unsigned frames){
  auto &audio=*owner;
  audio.synth.Render((float*)buffer,frames,UnpackEngineControls(audio.controls.load(std::memory_order_relaxed)));
  audio.samples.fetch_add(frames,std::memory_order_relaxed);
 }
public:
 explicit EngineAudio(bool enabled){
  static_assert(std::atomic<std::uint64_t>::is_always_lock_free,"Audio control publication must be lock-free");
  if(!enabled)return;
  InitAudioDevice();device=IsAudioDeviceReady();
  if(!device){std::fprintf(stderr,"WARNING: Audio device unavailable; driving remains available.\n");return;}
  SetAudioStreamBufferSizeDefault(1024);
  stream=LoadAudioStream(AudioRate,32,1);ready=IsAudioStreamValid(stream);
  if(!ready){std::fprintf(stderr,"WARNING: Engine audio stream unavailable.\n");return;}
  owner=this;SetAudioStreamCallback(stream,Callback);PlayAudioStream(stream);
 }
 EngineAudio(const EngineAudio&)=delete;EngineAudio& operator=(const EngineAudio&)=delete;
 ~EngineAudio(){Close();}
 void Update(EngineControls c){controls.store(PackEngineControls(c),std::memory_order_relaxed);}
 bool Ready()const{return ready;}
 std::uint64_t Samples()const{return samples.load(std::memory_order_relaxed);}
 void Close(){
  if(ready){StopAudioStream(stream);UnloadAudioStream(stream);ready=false;owner=nullptr;}
  if(device){CloseAudioDevice();device=false;}
 }
};
}
