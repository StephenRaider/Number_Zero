#pragma once
struct EffectSettings {bool trails=true,motionBlur=true;};
struct EffectLayers {
 RenderTexture2D world{},props{},depth{},hud{};
 void Resize(int width,int height){
  Close();for(auto *target:{&world,&props,&depth,&hud}){*target=LoadRenderTexture(width,height);SetTextureFilter(target->texture,TEXTURE_FILTER_POINT);SetTextureWrap(target->texture,TEXTURE_WRAP_CLAMP);}
 }
 void Close(){for(auto *target:{&world,&props,&depth,&hud})if(target->id){UnloadRenderTexture(*target);*target={};}}
};
static void DrawTrails(const nz::TailHistory &history,const Environment &env,const nz::Vehicle &v){
 if(history.samples.size()<2)return;
 BeginBlendMode(BLEND_ADDITIVE);
 for(size_t i=1;i<history.samples.size();i++){
  if(!history.samples[i].connected)continue;
  float age=(float)(v.elapsed-history.samples[i-1].time),fade=std::clamp(1.f-age/.14f,0.f,1.f);fade*=fade;
  float strength=std::min(history.samples[i-1].strength,history.samples[i].strength);
  for(int side=0;side<2;side++){
   auto a=history.samples[i-1].point[side],b=history.samples[i].point[side];
   auto first=Project(env.anchor,a.x-(float)v.distance,a.y,a.z),last=Project(env.anchor,b.x-(float)v.distance,b.y,b.z);
   DrawLineEx(first,last,6.f,RGBA(255,22,5,(int)(fade*strength*28)));
   DrawLineEx(first,last,3.4f,RGBA(255,35,10,(int)(fade*strength*64)));
   DrawLineEx(first,last,2.f,RGBA(255,52,20,(int)(fade*strength*185)));
  }
 }
 EndBlendMode();
}
static Vector2 MotionPixels(const nz::Vehicle &v,bool enabled){
 float length=enabled?std::clamp(((float)v.speed-4.f)*PixelsPerMetre*.008f,0.f,7.f):0.f;
 return {-length*.89442719f,length*.44721360f};
}
static void Layer(Texture2D texture){DrawTexturePro(texture,{0,0,(float)Width,-(float)Height},{0,0,(float)Width,(float)Height},{0,0},0,WHITE);}
