#pragma once
#include "../assets/highway/layout.h"

// Props remain separate from the ground and the vehicle. Their depth maps resolve
// each pole, arm and gantry beam against the car, rather than sorting whole sprites.
struct HighwayRenderer {
 struct Prop {std::array<Texture2D,4> textures;};
 struct Instance {int kind;float forward,lateral,depth;};
 std::array<Texture2D,4> road{};std::array<Prop,5> props{};
 explicit HighwayRenderer(const fs::path &assets){
  auto folder=assets/"highway";const char *roads[]={"asphalt.png","asphalt_patched.png","shoulder.png","lane_markings.png"};const char *maps[]={"albedo","normal","depth","emit"};
  for(int i=0;i<4;i++){road[i]=LoadTexture((folder/roads[i]).string().c_str());if(!IsTextureValid(road[i]))throw std::runtime_error("Missing highway surface");GenTextureMipmaps(&road[i]);SetTextureFilter(road[i],TEXTURE_FILTER_TRILINEAR);SetTextureWrap(road[i],TEXTURE_WRAP_REPEAT);}
  for(int i=0;i<5;i++)for(int m=0;m<4;m++){
   auto &tex=props[i].textures[m];tex=LoadTexture((folder/(std::string(highway_assets::Props[i].name)+"_"+maps[m]+".png")).string().c_str());
   if(!IsTextureValid(tex))throw std::runtime_error("Missing highway prop map");
   if(m==0){GenTextureMipmaps(&tex);SetTextureFilter(tex,TEXTURE_FILTER_TRILINEAR);}else SetTextureFilter(tex,TEXTURE_FILTER_POINT);
   SetTextureWrap(tex,TEXTURE_WRAP_CLAMP);
  }
 }
 void Ground(ShaderProgram &shader){shader.Texture("roadSurface",road[0]);shader.Texture("roadPatched",road[1]);shader.Texture("roadShoulder",road[2]);shader.Texture("roadMarkings",road[3]);shader.Float("roadRepeat",highway_assets::RoadRepeat);}
 void Draw(ShaderProgram &shader,const Environment &env,const std::array<Texture2D,5> &car,Vector2 carOrigin,float carDepth,int layerMode=0){
  if(env.garage)return;
  std::vector<Instance> instances;
  auto add=[&](int kind,float s,float l){
   if(s>=nz::VisibleAhead)return;
   auto anchor=Project(env.anchor,s,l);const auto &def=highway_assets::Props[kind];float scale=PixelsPerMetre/26.f;
   Rectangle bounds{anchor.x-def.anchorX*scale,anchor.y-def.anchorY*scale,def.canvas*scale,def.canvas*scale};
   if(bounds.x+bounds.width<0||bounds.y+bounds.height<0||bounds.x>Width||bounds.y>Height)return;
   instances.push_back({kind,s,l,WorldToCamera(s,l,0).z});
  };
  int a=(int)std::floor((env.distance+env.minS-16)/24),b=(int)std::ceil((env.distance+env.maxS+16)/24);
  for(int k=a;k<=b;k++)for(int side:{-1,1})add(side>0?0:1,k*24.f+(side>0?12.f:0.f)-env.distance,side*highway_assets::MountLateral);
  a=(int)std::floor((env.distance+env.minS-4)/4);b=(int)std::ceil((env.distance+env.maxS+4)/4);
  for(int k=a;k<=b;k++)for(int side:{-1,1})add(side>0?2:3,k*4.f-env.distance,side*8.f);
  a=(int)std::floor((env.distance+env.minS-32)/180);b=(int)std::ceil((env.distance+env.maxS+32)/180);
  for(int k=a;k<=b;k++)add(4,k*180.f+32-env.distance,0);
  std::sort(instances.begin(),instances.end(),[](const Instance&a,const Instance&b){return a.depth<b.depth;});
  shader.Lights(env);shader.Int("layerMode",layerMode);shader.Vec2("resolution",{(float)Width,(float)Height});shader.Vec2("carOrigin",carOrigin);shader.Float("carDepth",carDepth);
  rlDrawRenderBatchActive();rlEnableDepthTest();
  for(const auto &instance:instances){
   const auto &def=highway_assets::Props[instance.kind];const auto &maps=props[instance.kind].textures;
   auto anchor=Project(env.anchor,instance.forward,instance.lateral);float scale=PixelsPerMetre/26.f;
   Vector2 origin{anchor.x-def.anchorX*scale,anchor.y-def.anchorY*scale};
   shader.Float("propDepth",instance.depth);shader.Float("depthRange",def.depthRange);shader.Float("sourcePixels",(float)def.canvas);shader.Float("station",instance.forward);shader.Float("gantry",instance.kind==4?1.f:0.f);
   BeginShaderMode(shader.value);shader.Texture("normalMap",maps[1]);shader.Texture("depthMap",maps[2]);shader.Texture("emissionMap",maps[3]);shader.Texture("carAlbedo",car[0]);shader.TextureSlot("carFront",car[2],5);
   DrawTexturePro(maps[0],{0,0,(float)def.canvas,(float)def.canvas},{origin.x,origin.y,def.canvas*scale,def.canvas*scale},{0,0},0,WHITE);EndShaderMode();
  }
  rlDrawRenderBatchActive();ShaderProgram::ClearSlot(5);rlDisableDepthTest();
  // Small optical halos are separate from solid geometry and cannot make the pole paint over the car.
  for(const auto &instance:instances)if(layerMode!=2&&instance.kind<2){
   auto head=instance.kind==0?highway_assets::RightHead:highway_assets::LeftHead;
   auto light=Project(env.anchor,instance.forward+head.forward,instance.lateral+head.lateral,head.height);
   float fade=std::clamp((nz::VisibleAhead-instance.forward)/8.f,0.f,1.f);
   DrawCircleGradient((int)light.x,(int)light.y,5,RGBA(255,226,177,(int)(28*fade)),BLANK);
  }
 }
 void ProbeOcclusion(ShaderProgram &shader,Environment env){
  const auto &def=highway_assets::Props[0];const auto &maps=props[0].textures;int n=def.canvas;
  auto probe=LoadRenderTexture(n,n);Image image=GenImageColor(128,128,WHITE);auto car=LoadTextureFromImage(image);UnloadImage(image);
  image=GenImageColor(128,128,RGBA(128,0,0));auto front=LoadTextureFromImage(image);UnloadImage(image);
  env.anchor={def.anchorX,def.anchorY};env.garage=false;shader.Lights(env);shader.Int("layerMode",3);shader.Float("spritePixels",(float)n);shader.Vec2("resolution",{(float)n,(float)n});shader.Vec2("carOrigin",{0,0});
  shader.Float("propDepth",0);shader.Float("depthRange",def.depthRange);shader.Float("sourcePixels",(float)n);shader.Float("station",0);shader.Float("gantry",0);
  auto count=[&](float carW){
   shader.Float("carDepth",carW);BeginTextureMode(probe);ClearBackground(BLANK);rlEnableDepthTest();BeginShaderMode(shader.value);
   shader.Texture("normalMap",maps[1]);shader.Texture("depthMap",maps[2]);shader.Texture("emissionMap",maps[3]);shader.Texture("carAlbedo",car);shader.TextureSlot("carFront",front,5);
   DrawTexture(maps[0],0,0,WHITE);EndShaderMode();rlDrawRenderBatchActive();ShaderProgram::ClearSlot(5);rlDisableDepthTest();EndTextureMode();
   Image capture=LoadImageFromTexture(probe.texture);ImageFormat(&capture,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);const Color *data=(Color*)capture.data;int visible=0;
   for(int i=0;i<n*n;i++)visible+=data[i].r+data[i].g+data[i].b>0;
   UnloadImage(capture);return visible;
  };
  int all=count(-100),none=count(100),split=count(1.2f);
  std::vector<Color> encoded(128*128,RGBA(255,255,0));UpdateTexture(front,encoded.data());int encodedNear=count(0);
  std::fill(encoded.begin(),encoded.end(),RGBA(0,0,0));UpdateTexture(front,encoded.data());int encodedFar=count(0);
  UnloadTexture(car);UnloadTexture(front);UnloadRenderTexture(probe);
  if(all<100||none!=0||split<=0||split>=all||encodedFar!=all||encodedNear>=encodedFar)throw std::runtime_error("Prop/car per-pixel depth occlusion or sampler binding failed");
  std::printf("SCENERY DEPTH PROBE: foreground %d, behind car %d, split pole/arm %d; encoded near/far %d/%d pixels\n",all,none,split,encodedNear,encodedFar);
 }
 void Close(){for(auto tex:road)UnloadTexture(tex);for(const auto &prop:props)for(auto tex:prop.textures)UnloadTexture(tex);}
};
