#include "engine_sound.h"
#include <filesystem>
#include <cstdio>
int main(int argc,char **argv){std::filesystem::create_directories("build/qa");const char *tag=argc>1?argv[1]:"current";std::vector<float> samples; nz::EngineSound sound;
 for(float rpm:{5000.f,6500.f,7500.f,8300.f}){std::vector<float> block(3*nz::AudioRate);sound.Render(block.data(),(unsigned)block.size(),{rpm,1,1,true,false});double e=0;for(int i=nz::AudioRate;i<(int)block.size();i++)e+=block[i]*block[i];std::printf("Civic %.0frpm RMS %.4f\n",rpm,std::sqrt(e/(2*nz::AudioRate)));samples.insert(samples.end(),block.begin(),block.end());}
 char path[180];std::snprintf(path,sizeof(path),"build/qa/civic_top_end_%s.wav",tag);return carsynth::writeWavMono(path,samples,nz::AudioRate)?0:1;}
