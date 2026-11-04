#include "raylib.h"
#include "rlgl.h"
#include "vehicle.h"
#include "trail_history.h"
#include "traffic.h"
#include "race.h"
#include "race_telemetry.h"
#include "scene.h"
#include "decal_projection.h"
#include "engine_audio.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;
static int Width=640,Height=360;
constexpr int SpriteSize=128,PixelCount=128*128;
constexpr float K=.70710678118f,ElevationSin=.5f,ElevationCos=.86602540378f;
static float PixelsPerMetre=26.f;
const Color Ink{10,15,23,255},Panel{19,27,39,246},Muted{133,155,173,255},Paper{231,236,233,255},Mint{120,224,196,255},Amber{244,189,110,255};
const char *MapNames[]={"albedo","normal","pack","pack2","emit","mask"};
const char *PoseNames[]={"left","straight","right"};
static std::array<bool,512> framePress{};
static bool Pressed(int key){return key>=0&&key<(int)framePress.size()&&framePress[key];}
struct Maps { std::array<std::vector<Color>,6> pixels; };
struct PoseArt { Maps body,closed;std::array<Maps,4> pitch,closedPitch;std::array<Maps,9> wheels; };
struct CarArt { std::array<std::array<PoseArt,3>,2> views; };
struct PaintSettings {
 Color paint;bool twoTone,blackHood;float hue,saturation,value;
 bool operator==(const PaintSettings &b)const{return ColorToInt(paint)==ColorToInt(b.paint)&&twoTone==b.twoTone&&blackHood==b.blackHood&&hue==b.hue&&saturation==b.saturation&&value==b.value;}
};
struct Selection {
 std::array<bool,nz::Cars.size()> limiterDeleted{};
 std::array<std::array<nz::DecalPlacement,3>,nz::Cars.size()> decals{};
 std::array<PaintSettings,nz::Cars.size()> finishes{{{{229,226,207,255},true,false,35,.12f,.90f},{{225,230,222,255},false,false,95,.035f,.90f},{{204,28,34,255},false,false,358,.86f,.80f},{{174,181,184,255},false,false,198,.054f,.72f}}};
 int car=0;Color paint{229,226,207,255};bool twoTone=true,blackHood=false,manual=false;float hue=35.f,saturation=.12f,value=.90f;
 void CapturePaint(){finishes[car]={paint,twoTone,blackHood,hue,saturation,value};}
 void RestorePaint(){const auto &p=finishes[car];paint=p.paint;twoTone=p.twoTone;blackHood=p.blackHood;hue=p.hue;saturation=p.saturation;value=p.value;}
 void SelectCar(int next){if(next==car)return;CapturePaint();car=next;RestorePaint();}
};
struct Light { Vector3 position;Vector3 colour;float power,radius; };
struct Environment { Vector2 anchor;std::vector<Light> lights;int streetCount=0;bool garage=false,studio=false;float distance=0,lateral=0,minS=0,maxS=0;bool heads=true,tails=true,brake=false;float yaw=0,indicatorLeft=0,indicatorRight=0; };
static Color RGBA(int r,int g,int b,int a=255){return Color{(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a};}
static Vector3 WorldToCamera(float s,float l,float z){auto p=nz::NortheastCamera(s,l,z);return {p[0],p[1],p[2]};}
static Vector2 Project(Vector2 anchor,float s,float l,float z=0){auto p=WorldToCamera(s,l,z);return {anchor.x+p.x*PixelsPerMetre,anchor.y-p.y*PixelsPerMetre};}
static std::vector<Color> ReadPixels(const fs::path &path){
 Image image=LoadImage(path.string().c_str());
 if(!IsImageValid(image)||image.width!=128||image.height!=128)throw std::runtime_error("Missing or invalid car texture: "+path.string());
 ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
 std::vector<Color> result(PixelCount);std::memcpy(result.data(),image.data,PixelCount*sizeof(Color));UnloadImage(image);return result;
}
static Maps LoadMaps(const fs::path &folder,const std::string &prefix){Maps m;for(int i=0;i<6;i++)m.pixels[i]=ReadPixels(folder/(prefix+"_"+MapNames[i]+".png"));return m;}
static CarArt LoadCar(const fs::path &assets,const nz::CarSpec &spec){
 CarArt art;const auto folder=assets/"cars"/spec.id/"sprites";
 for(int view=0;view<2;view++)for(int pose=0;pose<3;pose++){
  auto label=std::string(view==0?"front_":"back_")+PoseNames[pose];
  art.views[view][pose].body=LoadMaps(folder,label+"_body");
  const char *samples[]={"m2","m1","p1","p2"};for(int i=0;i<4;i++)art.views[view][pose].pitch[i]=LoadMaps(folder,label+"_body_pitch_"+samples[i]);
  for(int f=0;f<9;f++)art.views[view][pose].wheels[f]=LoadMaps(folder,label+"_wheels_f"+std::to_string(f));
  if(spec.popupHeadlights){art.views[view][pose].closed=LoadMaps(folder,label+"_body_closed");for(int i=0;i<4;i++)art.views[view][pose].closedPitch[i]=LoadMaps(folder,label+"_body_closed_pitch_"+samples[i]);}
 }
 return art;
}
#include "decals.h"
struct ShaderProgram {
 Shader value{};std::map<std::string,int> locations;
 explicit ShaderProgram(const fs::path &path){value=LoadShader(nullptr,path.string().c_str());if(!IsShaderValid(value)||value.id==rlGetShaderIdDefault())throw std::runtime_error("Shader compilation failed: "+path.string());}

 int At(const char *name){auto it=locations.find(name);if(it!=locations.end())return it->second;int loc=GetShaderLocation(value,name);locations[name]=loc;return loc;}
 void Float(const char *name,float v){int loc=At(name);if(loc>=0)SetShaderValue(value,loc,&v,SHADER_UNIFORM_FLOAT);}
 void Int(const char *name,int v){int loc=At(name);if(loc>=0)SetShaderValue(value,loc,&v,SHADER_UNIFORM_INT);}
 void Vec2(const char *name,Vector2 v){int loc=At(name);if(loc>=0)SetShaderValue(value,loc,&v,SHADER_UNIFORM_VEC2);}
 void Vec4(const char *name,Vector4 v){int loc=At(name);if(loc>=0)SetShaderValue(value,loc,&v,SHADER_UNIFORM_VEC4);}
 void Texture(const char *name,Texture2D tex){int loc=At(name);if(loc>=0)SetShaderValueTexture(value,loc,tex);}
 // Raylib's batch tracks four auxiliary samplers. Complex passes explicitly bind
 // additional units above that range, then release them after the batch is flushed.
 void TextureSlot(const char *name,Texture2D tex,int slot){Int(name,slot);rlActiveTextureSlot(slot);rlEnableTexture(tex.id);rlActiveTextureSlot(0);}
 static void ClearSlot(int slot){rlActiveTextureSlot(slot);rlDisableTexture();rlActiveTextureSlot(0);}
 void Lights(const Environment &e){
  std::array<float,96> positions{},colours{};std::array<float,32> powers{},radii{};
  if(e.lights.size()>32)throw std::runtime_error("Light capacity exceeded");
  int n=(int)e.lights.size();
  for(int i=0;i<n;i++){const auto &l=e.lights[i];positions[i*3]=l.position.x;positions[i*3+1]=l.position.y;positions[i*3+2]=l.position.z;colours[i*3]=l.colour.x;colours[i*3+1]=l.colour.y;colours[i*3+2]=l.colour.z;powers[i]=l.power;radii[i]=l.radius;}
  Float("studioLight",e.studio?1.f:0.f);Int("lightCount",n);Int("streetCount",e.streetCount);
  for(auto pair:{std::pair<const char*,const float*>{"lightPosition[0]",positions.data()},{"lightColour[0]",colours.data()}}){int loc=At(pair.first);if(loc>=0)SetShaderValueV(value,loc,pair.second,SHADER_UNIFORM_VEC3,n);}
  int loc=At("lightPower[0]");if(loc>=0)SetShaderValueV(value,loc,powers.data(),SHADER_UNIFORM_FLOAT,n);
  loc=At("lightRadius[0]");if(loc>=0)SetShaderValueV(value,loc,radii.data(),SHADER_UNIFORM_FLOAT,n);
  Vec2("screenAnchor",e.anchor);Float("pixelsPerMetre",PixelsPerMetre);Float("spritePixels",128.f*PixelsPerMetre/26.f);Float("carYaw",e.yaw);Float("indicatorLeft",e.indicatorLeft);Float("indicatorRight",e.indicatorRight);Float("visibleAhead",nz::VisibleAhead);Float("beamReach",nz::BeamReach);Float("beamBase",nz::BeamBase);Float("beamSpread",nz::BeamSpread);Float("headlights",e.heads?1.f:0.f);Float("taillights",e.tails?1.f:0.f);Float("braking",e.brake?1.f:0.f);
 }
};
#include "highway.h"
struct CarRenderer {
 Vector4 shadowBounds{0,0,1,1};
 std::array<std::vector<Color>,5> output;std::array<Texture2D,5> textures{};RenderTexture2D shaded{};
 std::array<nz::DecalPlacement,3> lastDecals{};int decalPixels=0;
 int lastCar=-1,lastView=-1,lastPose=-1,lastFrame=-1,lastPitch=99;Color lastPaint{};bool lastHood=false,lastTwoTone=false,lastRaised=true;
 CarRenderer(){shaded=LoadRenderTexture(128,128);SetTextureFilter(shaded.texture,TEXTURE_FILTER_POINT);Image im=GenImageColor(128,128,BLANK);for(int i=0;i<5;i++){output[i].resize(PixelCount);textures[i]=LoadTextureFromImage(im);SetTextureFilter(textures[i],TEXTURE_FILTER_POINT);SetTextureWrap(textures[i],TEXTURE_WRAP_CLAMP);}UnloadImage(im);}
 void Compose(const CarArt &art,const Selection &choice,int view,int pose,int frame,int pitch,const DecalLibrary &decals,bool raised=true){
  if(lastCar==choice.car&&lastView==view&&lastPose==pose&&lastFrame==frame&&lastPitch==pitch&&ColorToInt(lastPaint)==ColorToInt(choice.paint)&&lastHood==choice.blackHood&&lastTwoTone==choice.twoTone&&lastDecals==choice.decals[choice.car]&&lastRaised==raised)return;
  lastCar=choice.car;lastView=view;lastPose=pose;lastFrame=frame;lastPitch=pitch;lastPaint=choice.paint;lastHood=choice.blackHood;lastTwoTone=choice.twoTone;lastDecals=choice.decals[choice.car];lastRaised=raised;decalPixels=0;
  const auto &part=art.views[view][pose];bool closed=nz::Cars[choice.car].popupHeadlights&&!raised;const auto &body=pitch==0?(closed?part.closed:part.body):(closed?part.closedPitch:part.pitch)[pitch<0?pitch+2:pitch+1];const auto &wheels=art.views[view][pose].wheels[frame];
  const float original[4][3]={{255,234,186},{219,229,237},{219,229,237},{219,229,237}};
  for(int i=0;i<PixelCount;i++){
   bool b=body.pixels[0][i].a>=128,w=wheels.pixels[0][i].a>=128;
   unsigned bd=body.pixels[2][i].r*256u+body.pixels[2][i].g,wd=wheels.pixels[2][i].r*256u+wheels.pixels[2][i].g;
   const Maps &src=w&&(!b||wd>bd)?wheels:body;
   for(int m=0;m<5;m++)output[m][i]=src.pixels[m][i];
   // Pack alpha is spare: coverage/occlusion uses albedo alpha. Keep finish identity categorical.
   output[2][i].a=0;
   if(&src==&body&&b){
    // Preserve vehicle-left/right in the blue indicator channel, including mirrored driving sprites.
    float cx=(i%128+.5f-64)/26.f,cy=(79-i/128-.5f)/26.f,cz=((bd/65535.f)-.5f)*5.2f;
    float localX=(view==0?1.f:-1.f)*(K*cx+K*.5f*cy-K*ElevationCos*cz);
    output[4][i].a=localX>0?255:0;
    Color mask=body.pixels[5][i],paint=choice.paint;int channel=mask.b>127?2:mask.g>127?1:mask.r>127?0:-1;
    output[2][i].a=channel>=0?128:src.pixels[2][i].b>=215?255:0;
    if(channel<0)continue;
    if(channel==1&&choice.twoTone&&choice.car==0)paint=RGBA(174,174,174);
    if(channel==2&&choice.blackHood)paint=RGBA(29,32,36);
    const float *base=original[choice.car];float lower[3]={174,174,174};if(channel==1&&choice.car==0)base=lower;
    Color source=output[0][i];output[0][i]=RGBA(std::clamp((int)std::lround(paint.r*source.r/base[0]),0,255),std::clamp((int)std::lround(paint.g*source.g/base[1]),0,255),std::clamp((int)std::lround(paint.b*source.b/base[2]),0,255),source.a);
    // Project into the actual side surface BEFORE lighting and rear mirroring.
    // Paint mask + side normals reject glazing, bonnet, bumper and opposite side.
    if(channel!=2){
     float nx=(src.pixels[1][i].r*256.f+src.pixels[1][i].g)/65535.f*2-1;
     float ny=(src.pixels[1][i].b*256.f+src.pixels[3][i].r)/65535.f*2-1,nz=std::sqrt(std::max(0.f,1-nx*nx-ny*ny));
     auto surface=nz::RecoverDecalSurface(cx,cy,cz,nx,ny,nz,view,pose,pitch);
     if(nz::DecalSide(surface)){
      bool covered=false;
      for(const auto &placement:choice.decals[choice.car])if(placement.design>=0&&placement.design<(int)decals.entries.size()){
       const auto &decal=decals.entries[placement.design];std::array<float,4> ink{};
       // Four colour samples per native pixel; packed geometry itself remains point sampled.
       for(float dx:{-.25f,.25f})for(float dy:{-.25f,.25f}){
        auto q=nz::RecoverDecalSurface(cx+dx/26.f,cy+dy/26.f,cz,nx,ny,nz,view,pose,pitch);
        auto uv=nz::DecalUV(q,placement,decal.width/(float)decal.height);auto tap=decals.Sample(decal,uv.first,uv.second,placement.tint);
        for(int k=0;k<4;k++)ink[k]+=tap[k]*.25f;
       }
       Color &pixel=output[0][i];float alpha=ink[3];covered|=alpha>.05f;
       pixel=RGBA((int)std::lround(pixel.r*(1-alpha)+ink[0]),(int)std::lround(pixel.g*(1-alpha)+ink[1]),(int)std::lround(pixel.b*(1-alpha)+ink[2]),pixel.a);
      }
      if(covered)decalPixels++;
     }
    }
   }
  }
  if(view==1){
   // A mirrored sprite also needs mirrored camera-space normals, not just flipped colour.
   for(int y=0;y<128;y++)for(int x=0;x<64;x++)for(int m=0;m<5;m++)std::swap(output[m][y*128+x],output[m][y*128+127-x]);
   for(int i=0;i<PixelCount;i++){unsigned nx=65535u-(output[1][i].r*256u+output[1][i].g);output[1][i].r=(unsigned char)(nx/256);output[1][i].g=(unsigned char)(nx%256);}
  }
  int x0=128,y0=128,x1=0,y1=0;for(int i=0;i<PixelCount;i++)if(output[0][i].a>127){x0=std::min(x0,i%128);y0=std::min(y0,i/128);x1=std::max(x1,i%128+1);y1=std::max(y1,i/128+1);}
  shadowBounds={(float)x0/128,(float)y0/128,(float)x1/128,(float)y1/128};
  for(int i=0;i<5;i++)UpdateTexture(textures[i],output[i].data());
 }
 std::array<nz::TrailPoint,2> TailSources(Vector2 origin,Vector2 anchor)const{
  std::array<nz::TrailPoint,2> result{{{-1.9f,-.58f,.55f},{-1.9f,.58f,.55f}}};
  std::array<Vector3,2> sum{};std::array<float,2> weight{};
  for(int i=0;i<PixelCount;i++)if(output[0][i].a>127&&output[4][i].g>=48){
   float w=output[4][i].g;
   float x=(i%128+.5f-64)/26.f,y=(79-i/128-.5f)/26.f,z=((output[2][i].r*256.f+output[2][i].g)/65535.f-.5f)*5.2f;
   // Classify on the unposed body, not the indicator alpha or a centre brake lamp.
   auto local=nz::RecoverDecalSurface(lastView==1?-x:x,y,z,0,0,1,lastView,lastPose,lastPitch);
   if(!nz::OuterRearLamp(-local.y,local.x))continue;
   int side=local.x<0?1:0; // vehicle lateral = -model X in the mirrored driving view
   float scale=PixelsPerMetre/26.f;
   x+=(origin.x-(anchor.x-64*scale))/PixelsPerMetre;
   y-=(origin.y-(anchor.y-79*scale))/PixelsPerMetre;
   sum[side].x+=(K*x+K*ElevationSin*y-K*ElevationCos*z)*w;
   sum[side].y+=(K*x-K*ElevationSin*y+K*ElevationCos*z)*w;
   sum[side].z+=(ElevationCos*y+ElevationSin*z)*w;weight[side]+=w;
  }
  for(int side=0;side<2;side++)if(weight[side]>0)result[side]={sum[side].x/weight[side],sum[side].y/weight[side],std::max(.15f,sum[side].z/weight[side])};
  return result;
 }
 void Shade(ShaderProgram &shader,const Environment &env,Vector2 origin,float baseDepth){
  shader.Lights(env);shader.Float("sourcePixels",128);shader.Float("depthRange",5.2f);shader.Vec2("carOrigin",origin);shader.Float("carDepth",baseDepth);
  // Shade the original categorical/16-bit maps at native resolution first.
  BeginTextureMode(shaded);ClearBackground(BLANK);BeginShaderMode(shader.value);shader.Texture("normalMap",textures[1]);shader.Texture("depthMap",textures[2]);shader.Texture("auxiliaryMap",textures[3]);shader.Texture("emissionMap",textures[4]);DrawTexture(textures[0],0,0,WHITE);EndShaderMode();EndTextureMode();
 }
 void Draw(ShaderProgram &resolve,ShaderProgram &glow,const Environment &env,Vector2 origin,Texture2D propColour,Texture2D propDepth,Vector2 blur,float baseDepth){
  float size=128.f*PixelsPerMetre/26.f;resolve.Float("spritePixels",size);resolve.Vec2("resolution",{(float)Width,(float)Height});resolve.Vec2("blurPixels",blur);resolve.Float("carDepth",baseDepth);
  BeginShaderMode(resolve.value);resolve.Texture("propColour",propColour);resolve.Texture("propDepth",propDepth);resolve.Texture("carFront",textures[2]);DrawTexturePro(shaded.texture,{0,0,128,-128},{origin.x,origin.y,size,size},{0,0},0,WHITE);EndShaderMode();
  // A separate lamp-only halo can spill over both bodywork and the road, without blooming the HUD/specular paint.
  glow.Lights(env);BeginBlendMode(BLEND_ADDITIVE);BeginShaderMode(glow.value);
  DrawTexturePro(textures[4],{0,0,128,128},{origin.x-6,origin.y-6,size+12,size+12},{0,0},0,WHITE);
  EndShaderMode();EndBlendMode();
 }
 void ProbeComposite(ShaderProgram &shader){
  Image image=GenImageColor(128,128,RGBA(0,255,0));auto vehicle=LoadTextureFromImage(image);UnloadImage(image);
  image=GenImageColor(128,128,RGBA(128,0,0));auto front=LoadTextureFromImage(image);UnloadImage(image);
  image=GenImageColor(64,64,RGBA(255,0,0));auto foreground=LoadTextureFromImage(image);UnloadImage(image);
  image=GenImageColor(64,64,BLANK);auto field=LoadTextureFromImage(image);UnloadImage(image);auto probe=LoadRenderTexture(64,64);
  shader.Float("spritePixels",64);shader.Float("carDepth",0);shader.Vec2("resolution",{64,64});
  std::vector<Color> encoded(64*64);
  auto sample=[&](float w,float blur,bool partial){
   unsigned value=(unsigned)std::lround((w/256.f+.5f)*65535);std::fill(encoded.begin(),encoded.end(),RGBA(value/256,value%256,0));UpdateTexture(field,encoded.data());
   std::fill(encoded.begin(),encoded.end(),partial?RGBA(128,0,0,128):RGBA(255,0,0));UpdateTexture(foreground,encoded.data());shader.Vec2("blurPixels",{blur,blur*.5f});
   BeginTextureMode(probe);ClearBackground(BLANK);BeginShaderMode(shader.value);shader.Texture("propColour",foreground);shader.Texture("propDepth",field);shader.Texture("carFront",front);
   DrawTexturePro(vehicle,{0,0,128,128},{0,0,64,64},{0,0},0,WHITE);EndShaderMode();EndTextureMode();
   Image capture=LoadImageFromTexture(probe.texture);Color pixel=GetImageColor(capture,32,32);UnloadImage(capture);return pixel;
  };
  bool ok=true;
  for(float blur:{0.f,5.f}){auto behind=sample(-10,blur,false),above=sample(10,blur,false),edge=sample(10,blur,true);
   ok&=behind.g>245&&behind.r<5&&above.r>245&&above.g<5&&edge.r>120&&edge.r<136&&edge.g>120&&edge.g<136;}
  UnloadTexture(vehicle);UnloadTexture(front);UnloadTexture(foreground);UnloadTexture(field);UnloadRenderTexture(probe);
  if(!ok)throw std::runtime_error("Sharp car / blurred foreground compositor failed");
  std::puts("COMPOSITOR GPU PROBE: behind/front/partial-coverage cases pass with blur off/on");
 }
 void ProbeGlow(ShaderProgram &shader,Environment env){
  float size=128.f*PixelsPerMetre/26.f;int extent=(int)std::ceil(size+12);RenderTexture2D probe=LoadRenderTexture(extent,extent);
  env.indicatorLeft=env.indicatorRight=0;
  auto measure=[&](bool head,bool tail,bool brake){
   env.heads=head;env.tails=tail;env.brake=brake;shader.Lights(env);
   BeginTextureMode(probe);ClearBackground(BLANK);BeginBlendMode(BLEND_ADDITIVE);BeginShaderMode(shader.value);
   DrawTexturePro(textures[4],{0,0,128,128},{0,0,size+12,size+12},{0,0},0,WHITE);
   EndShaderMode();EndBlendMode();EndTextureMode();
   Image capture=LoadImageFromTexture(probe.texture);ImageFormat(&capture,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
   std::array<unsigned long long,3> sum{};const Color *pixels=(Color*)capture.data;
   for(int i=0;i<capture.width*capture.height;i++){sum[0]+=pixels[i].r;sum[1]+=pixels[i].g;sum[2]+=pixels[i].b;}UnloadImage(capture);return sum;
  };
  auto off=measure(false,false,false),head=measure(true,false,false),tail=measure(false,true,false),brake=measure(false,false,true);
  bool hasHead=false,hasTail=false;for(auto e:output[4]){hasHead|=e.r>2;hasTail|=e.g>2;}
  UnloadRenderTexture(probe);
  if(off[0]+off[1]+off[2]!=0)throw std::runtime_error("Disabled lamps still produce bloom");
  if(hasHead&&head[0]==0)throw std::runtime_error("Visible headlights have no bloom");
  if(hasTail&&(tail[0]==0||tail[0]<=tail[1]||brake[0]<=tail[0]))throw std::runtime_error("Tail/brake bloom intensity or colour invalid");
  std::printf("GLOW PROBE %s: head %llu, tail %llu, brake %llu, all-off %llu\n",env.garage?"garage":"driving",head[0],tail[0],brake[0],off[0]);
 }
 void Close(){for(auto tex:textures)UnloadTexture(tex);UnloadRenderTexture(shaded);}
};
#include "visual_effects.h"
#include "traffic_renderer.h"
static Environment MakeEnvironment(bool garage,const nz::Vehicle &v,bool heads,bool tails,bool brake,float clock,int indicator){
 auto view=nz::FitDrivingView(Width,Height);PixelsPerMetre=garage?26.f:view.pixelsPerMetre;
 Environment e;e.garage=garage;e.anchor=garage?Vector2{186+(Width-640)*.5f,214+(Height-360)*.5f}:Vector2{view.x,view.y};e.distance=(float)v.distance;e.lateral=garage?0.f:(float)v.lateral;e.heads=heads;e.tails=tails;e.brake=brake;e.yaw=garage?0.f:nz::SpriteYaw(v.yaw);
 float flash=std::fmod(clock,1.2f)<.56f?1.f:0.f;e.indicatorLeft=(indicator==-1||indicator==2)?flash:0;e.indicatorRight=(indicator==1||indicator==2)?flash:0;
 if(garage){
  auto key=nz::GarageKeyLight(clock);Vector3 orbit{key[0],key[1],key[2]};
  e.lights={{orbit,{1,.90f,.76f},2.1f,11},{{2.6f,2.6f,2.2f},{.52f,.75f,1},.45f,10},{{0,5,1.0f},{1,.96f,.85f},.35f,12}};e.streetCount=3;
 }
 else{
  // Keep fixed world-space posts alive until their full light tail clears every visible receiver.
  float minS=1e9f,maxS=-1e9f;
  for(float x:{-128.f,(float)Width+128})for(float y:{-128.f,(float)Height+128}){
   float cx=(x-e.anchor.x)/PixelsPerMetre,cy=(e.anchor.y-y)/PixelsPerMetre;
   float s=nz::NortheastGround(cx,cy).first;minS=std::min(minS,s);maxS=std::max(maxS,s);
  }
  e.minS=minS;e.maxS=maxS;
  for(int side:{-1,1}){
   float phase=side>0?12.f:0.f;
   auto [first,last]=nz::StreetRange(v.distance,minS,maxS,side);
   for(int k=first;k<=last;k++){
    float s=(float)(k*24+phase-v.distance);e.lights.push_back({WorldToCamera(s+(side>0?highway_assets::RightHead.forward:highway_assets::LeftHead.forward),side*highway_assets::MountLateral+(side>0?highway_assets::RightHead.lateral:highway_assets::LeftHead.lateral),(side>0?highway_assets::RightHead.height:highway_assets::LeftHead.height)),{1.f,.85f,.66f},3.4f,11.5f});
   }
  }
  e.streetCount=(int)e.lights.size();

 }
 // Lamp positions and floor cones share the same yaw/spacing. Garage uses the front-view basis.
 auto lampCamera=[&](float forward,float lateral,float height){return garage?Vector3{(forward+lateral)*K,height*ElevationCos+(lateral-forward)*K*ElevationSin,(forward-lateral)*K*ElevationCos+height*ElevationSin}:WorldToCamera(forward,lateral,height);};
 if(heads)for(int side:{-1,1}){auto point=nz::HeadlampPosition(e.yaw,side);e.lights.push_back({lampCamera(point.first,point.second+e.lateral,.5f),{1,.88f,.65f},.42f,3.2f});}
 if(tails||brake)e.lights.push_back({lampCamera(-2,e.lateral,.45f),{1,.08f,.04f},brake?1.35f:.45f,3.5f});
 return e;
}
static void Road(ShaderProgram &shader,CarRenderer &car,HighwayRenderer &highway,const Environment &env,Vector2 origin,float depth,Texture2D trafficGeometry){
 shader.Lights(env);shader.Vec2("resolution",{(float)Width,(float)Height});shader.Float("travelled",env.distance);shader.Float("garage",env.garage?1.f:0.f);shader.Float("playerLateral",env.lateral);shader.Vec4("carBounds",car.shadowBounds);shader.Vec2("carOrigin",origin);shader.Float("carDepth",depth);
 BeginShaderMode(shader.value);highway.Ground(shader);shader.TextureSlot("carAlbedo",car.textures[0],5);shader.TextureSlot("carFront",car.textures[2],6);shader.TextureSlot("carBack",car.textures[3],7);shader.TextureSlot("trafficGeometry",trafficGeometry,8);DrawRectangle(0,0,Width,Height,WHITE);EndShaderMode();ShaderProgram::ClearSlot(8);ShaderProgram::ClearSlot(5);ShaderProgram::ClearSlot(6);ShaderProgram::ClearSlot(7);
}
static void Label(const char *text,int x,int y,int size,Color color=Paper){DrawText(text,x,y,size,color);}
static void Box(Rectangle r,Color color=Panel){DrawRectangleRec(r,color);DrawRectangleLinesEx(r,1,RGBA(49,65,79));}
static bool Button(Rectangle r,const char *text,Vector2 mouse,bool accent=false){
 bool hover=CheckCollisionPointRec(mouse,r);DrawRectangleRec(r,accent?(hover?RGBA(162,245,216):Mint):(hover?RGBA(43,59,73):RGBA(28,39,52)));DrawRectangleLinesEx(r,1,accent?Mint:RGBA(56,75,91));
 int size=10;Label(text,(int)(r.x+(r.width-MeasureText(text,size))/2),(int)(r.y+(r.height-size)/2),size,accent?Ink:Paper);
 return hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static bool Toggle(Rectangle r,const char *text,bool &value,Vector2 mouse){
 DrawRectangle((int)r.x,(int)r.y,9,9,value?Mint:RGBA(37,50,63));if(value)Label("+",(int)r.x+1,(int)r.y-1,10,Ink);Label(text,(int)r.x+15,(int)r.y,10,Muted);
 if(CheckCollisionPointRec(mouse,r)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){value=!value;return true;}return false;
}
#include "decal_editor.h"
static void Garage(Selection &s,Vector2 mouse,bool &start,bool &quit,bool &heads,int &indicator,bool &decalEditor,bool &raceMode){
 DrawRectangle(0,0,640,57,RGBA(9,15,23,235));Label("NUMBER ZERO",24,16,23);Label(raceMode?"ROLLING RACE  /  10 KM":"NIGHT DRIVE  /  PROTOTYPE 01",25,42,9,Mint);
 Label("CHOOSE YOUR CAR",381,73,10,Muted);
 for(int i=0;i<(int)nz::Cars.size();i++){
  Rectangle r{378.f,91.f+i*26,240,24};Box(r,s.car==i?RGBA(36,58,65):Panel);
  Label(nz::Cars[i].name,389,(int)r.y+3,11,s.car==i?Mint:Paper);
  Label(TextFormat("%.0f PS   %s   %.0f kg",nz::Cars[i].peakPS,nz::Cars[i].drive,nz::Cars[i].mass),389,(int)r.y+15,8,Muted);
  if(CheckCollisionPointRec(mouse,r)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){s.SelectCar(i);}
 }
 Label("BODY PAINT",381,200,10,Muted);
 const Color colours[]={RGBA(229,226,207),RGBA(188,39,46),RGBA(36,79,139),RGBA(37,87,76),RGBA(28,31,36),RGBA(174,181,184),RGBA(223,177,41),RGBA(129,88,140)};
 for(int i=0;i<8;i++){Rectangle r{381.f+i*29,216,24,18};DrawRectangleRec(r,colours[i]);if(ColorToInt(colours[i])==ColorToInt(s.paint))DrawRectangleLinesEx({r.x-2,r.y-2,r.width+4,r.height+4},1,Mint);if(CheckCollisionPointRec(mouse,r)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))s.paint=colours[i];}
 Rectangle square{381,242,196,44},hue{584,242,26,44};Color full=ColorFromHSV(s.hue,1,1);DrawRectangleGradientEx(square,WHITE,BLACK,full,BLACK);
 for(int y=0;y<44;y++)DrawRectangle(584,242+y,26,1,ColorFromHSV(y/44.f*360,1,1));
 if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,hue)){s.hue=std::clamp((mouse.y-hue.y)/hue.height,0.f,1.f)*360;s.paint=ColorFromHSV(s.hue,s.saturation,s.value);}
 if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,square)){s.saturation=std::clamp((mouse.x-square.x)/square.width,0.f,1.f);s.value=1-std::clamp((mouse.y-square.y)/square.height,0.f,1.f);s.paint=ColorFromHSV(s.hue,s.saturation,s.value);}
 DrawRectangleLinesEx(square,1,RGBA(62,78,89));Label(TextFormat("#%02X%02X%02X",s.paint.r,s.paint.g,s.paint.b),381,291,9,Muted);
 if(s.car==0)Toggle({443,291,102,13},"Two-tone",s.twoTone,mouse);
 Toggle({537,291,80,13},"Black hood",s.blackHood,mouse);
 if(Button({378,316,240,27},raceMode?"START 10 KM RACE   [ENTER]":"START NIGHT DRIVE   [ENTER]",mouse,true)||Pressed(KEY_ENTER))start=true;
 Label("ORBITING STUDIO LIGHT",25,75,9,Muted);
 if(Button({25,94,108,20},"DECALS / GRAPHICS",mouse))decalEditor=true;
 if(Button({25,121,103,19},"DRIVE",mouse,!raceMode))raceMode=false;
 if(Button({135,121,103,19},"RACE / 10 KM",mouse,raceMode))raceMode=true;
 if(Button({25,252,77,18},heads?"H LIGHTS ON":"H LIGHTS OFF",mouse,heads))heads=!heads;
 if(Button({106,252,64,18},"Z LEFT",mouse,indicator==-1))indicator=indicator==-1?0:-1;
 if(Button({174,252,64,18},"X HAZARD",mouse,indicator==2))indicator=indicator==2?0:2;
 if(Button({242,252,70,18},"C RIGHT",mouse,indicator==1))indicator=indicator==1?0:1;
 Label(nz::Cars[s.car].engine,25,279,13,Paper);if(Button({25,297,145,18},s.manual?"MANUAL  /  5 SPEED":"AUTO  /  5 SPEED",mouse,s.manual))s.manual=!s.manual;if(nz::Cars[s.car].speedLimitKmh>0){if(Button({182,297,164,18},s.limiterDeleted[s.car]?"SPEED LIMITER: DELETED":"SPEED LIMITER: STOCK",mouse,s.limiterDeleted[s.car]))s.limiterDeleted[s.car]=!s.limiterDeleted[s.car];}else Label("NO FACTORY SPEED LIMITER",182,302,8,Muted);Label("Q DOWN / E UP",268,322,8,s.manual?Mint:Muted);Label("W / UP  THROTTLE     S / DOWN  BRAKE",25,322,9,Muted);Label("A / D  STEER     H  HEADLIGHTS     T  TAIL LIGHTS",25,338,8,Muted);
 if(Pressed(KEY_ESCAPE))quit=true;
}
static Vector2 DialPoint(Vector2 centre,float radius,float rpm,float maximum){
 float angle=(135.f+270.f*rpm/maximum)*(float)nz::Pi/180.f;
 return {centre.x+std::cos(angle)*radius,centre.y+std::sin(angle)*radius};
}
static void Hud(const Selection &s,const nz::Vehicle &v,bool heads,bool tails,bool audioReady,bool muted,int indicator,float clock){
 const auto &spec=nz::Cars[s.car];const float left=12;
 Vector2 centre{left+63,(float)Height-52};float maximum=(float)std::ceil(spec.redline/1000.)*1000;
 const bool overrev=v.rpm>=spec.shiftRpm;
 const Color copper=overrev?Amber:RGBA(151,89,67,205),dim=RGBA(92,121,116,190),cool=overrev?Amber:RGBA(180,231,214);
 auto centred=[&](const char *text,int y,int size,Color colour){Label(text,(int)centre.x-MeasureText(text,size)/2,y,size,colour);};
 // Fully transparent instrument face: only the rim, ticks and lettering are drawn.
 DrawRing(centre,33,34,0,360,96,copper);DrawRing(centre,30,31,135,405,72,overrev?RGBA(244,189,110,120):RGBA(48,76,72,190));
 float fraction=(float)std::clamp(v.rpm/maximum,0.,1.);
 DrawRing(centre,30,31,135,135+270*fraction,64,v.rpm>=spec.shiftRpm?Amber:RGBA(102,196,171,210));
 DrawRing(centre,32,34,135+270*(float)(spec.shiftRpm/maximum),405,20,overrev?Amber:RGBA(207,90,66,210));
 for(int tick=0;tick<=(int)(maximum/250);tick++){
  float rpm=tick*250.f;bool major=tick%4==0;
  auto outer=DialPoint(centre,32,rpm,maximum),inner=DialPoint(centre,major?27.f:30.f,rpm,maximum);
  Color colour=overrev?Amber:rpm>=spec.shiftRpm?RGBA(218,122,94):rpm<=v.rpm?cool:dim;
  DrawLineEx(inner,outer,1,colour);
  if(major){auto pos=DialPoint(centre,23,rpm,maximum);const char *number=TextFormat("%d",tick/4);Label(number,(int)pos.x-MeasureText(number,7)/2,(int)pos.y-3,7,colour);}
 }
 auto marker=DialPoint(centre,35,(float)std::clamp(v.rpm,0.,(double)maximum),maximum);DrawCircleV(marker,1.3f,overrev?Amber:Mint);
 centred(TextFormat("%d",(int)std::lround(v.speed*3.6)),(int)centre.y-10,18,cool);
 centred("km/h",(int)centre.y+9,7,overrev?Amber:Muted);centred("x1000",(int)centre.y+24,6,overrev?Amber:dim);
 centred(TextFormat("%.2f km",v.distance/1000),(int)centre.y+39,7,Muted);
 if(spec.turbo){
  Vector2 boost{centre.x+55,centre.y};float fraction=(float)std::clamp(v.boost,0.,1.);
  DrawRing(boost,15,16,0,360,48,copper);DrawRing(boost,12,13,135,405,40,dim);
  DrawRing(boost,12,13,135,135+270*fraction,32,overrev?Amber:Mint);
  for(int i=0;i<=4;i++)DrawLineEx(DialPoint(boost,13,i/4.f,1),DialPoint(boost,15,i/4.f,1),1,overrev?Amber:dim);
  const char *value=TextFormat("%.0f",fraction*100);Label(value,(int)boost.x-MeasureText(value,8)/2,(int)boost.y-3,8,cool);
  Label("%",(int)boost.x-3,(int)boost.y+6,6,overrev?Amber:dim);Label("BOOST",(int)boost.x-14,(int)boost.y+21,7,overrev?Amber:Muted);
 }
 // Auxiliary strip mirrors the reference: gear and tiny pedal/status indicators beside the dial.
 for(int i=0;i<2;i++){
  int x=(int)left+2+i*8,y=(int)centre.y-31;DrawRectangle(x,y,2,13,RGBA(48,68,66));
  int h=(int)(13*(i==0?v.throttle:v.brake));DrawRectangle(x,y+13-h,2,h,i==0?Mint:RGBA(220,115,88));
 }
 Label(TextFormat("%d",v.gear+1),(int)left+2,(int)centre.y-12,20,cool);
 Label(v.automatic?"A":"M",(int)left+4,(int)centre.y+11,7,Muted);
 Label("H",(int)left,(int)centre.y+24,7,heads?Mint:RGBA(54,71,70));
 Label("T",(int)left+11,(int)centre.y+24,7,tails?Mint:RGBA(54,71,70));
 Label("M",(int)left+4,(int)centre.y+36,7,!audioReady||muted?RGBA(218,122,94):dim);
 bool flash=std::fmod(clock,1.2f)<.56f;
 if(flash&&(indicator==-1||indicator==2))Label("<",(int)centre.x-17,(int)centre.y-43,8,Amber);
 if(flash&&(indicator==1||indicator==2))Label(">",(int)centre.x+10,(int)centre.y-43,8,Amber);
 if(v.limiter)Label("LIMIT",(int)left,(int)centre.y-45,7,Amber);
 else if(!v.automatic&&v.rpm>spec.shiftRpm)Label("SHIFT",(int)left,(int)centre.y-45,7,Amber);
 else if(std::abs(v.lateral)>5.9)Label("EDGE",(int)left,(int)centre.y-45,7,Amber);
}
static void RaceHud(const nz::RaceSession &race,const nz::Vehicle &player,Vector2 mouse,bool &retry,bool &garage){
 float x=Width*.5f-130;double progress=race.PlayerProgress(player),other=race.OpponentProgress();
 DrawRectangle((int)x-8,8,276,47,RGBA(9,16,24,215));
 Label("10 KM  /  WHITE TESTAROSSA",(int)x,13,9,Paper);
 Label(TextFormat("YOU %.2f    RIVAL %.2f km",progress/1000,other/1000),(int)x,27,9,Mint);
 double gap=race.opponent.distance-player.distance;
 Label(TextFormat("%s %.0f m  |  %.1f s",gap>=0?"RIVAL AHEAD":"YOU LEAD",std::abs(gap),race.elapsed),(int)x,40,8,Muted);
 if(race.phase==nz::RacePhase::Countdown){
  Label(TextFormat("%d",(int)std::ceil(race.countdown)),Width/2-9,Height/2-28,36,Amber);Label("ROLLING START  /  50 KM/H",Width/2-89,Height/2+14,10,Paper);
 }else if(race.elapsed<.85){Label("GO",Width/2-19,Height/2-24,30,Mint);}
 if(race.phase==nz::RacePhase::Finished){
  float bx=Width*.5f-145,by=Height*.5f-69;Box({bx,by,290,139},RGBA(11,20,29,248));
  const char *result=race.winner==nz::RaceWinner::Player?"YOU WIN":race.winner==nz::RaceWinner::Tie?"PHOTO FINISH / TIE":"TESTAROSSA WINS";
  Label(result,(int)bx+18,(int)by+15,21,race.winner==nz::RaceWinner::Player?Mint:Amber);
  double time=race.winner==nz::RaceWinner::Player?race.playerFinish:race.opponentFinish;
  Label(TextFormat("10.00 KM FINISH  /  %.2f SEC",time),(int)bx+18,(int)by+46,10,Paper);
  Label("R REMATCH   /   TAB GARAGE",(int)bx+18,(int)by+67,9,Muted);
  retry=Button({bx+18,by+93,118,26},"REMATCH",mouse,true);garage=Button({bx+147,by+93,124,26},"GARAGE",mouse);
 }
}
static void PauseMenu(EffectSettings &effects,Vector2 mouse,bool &paused,bool &driving,double &accumulator){
 float x=(float)Width-258,y=16;
 DrawRectangle(0,0,Width,Height,RGBA(3,9,17,22));Box({x,y,242,170},RGBA(12,21,30,238));
 Label("PAUSED",(int)x+14,(int)y+12,18,Mint);Label("COMPARE VISUAL EFFECTS",(int)x+14,(int)y+37,8,Muted);
 if(Pressed(KEY_ONE))effects.trails=!effects.trails;
 if(Pressed(KEY_TWO))effects.motionBlur=!effects.motionBlur;
 Toggle({x+14,y+58,215,15},"[1] Tail-light trails",effects.trails,mouse);
 Label("Short red trails behind the car",(int)x+29,(int)y+77,8,Muted);
 Toggle({x+14,y+98,215,15},"[2] Background motion blur",effects.motionBlur,mouse);
 Label("Car and instruments stay sharp",(int)x+29,(int)y+117,8,Muted);
 if(Button({x+14,y+140,104,19},"RESUME  ESC",mouse,true))paused=false;
 if(Button({x+125,y+140,103,19},"GARAGE  TAB",mouse)){driving=false;paused=false;accumulator=0;}
}
static void ProbeDecalViews(CarRenderer &renderer,const CarArt &art,const Selection &selected,const DecalLibrary &lib){
 int minimum=PixelCount,maximum=0;
 for(int view=0;view<2;view++)for(int pose=0;pose<3;pose++)for(int pitch=-2;pitch<=2;pitch++){
  renderer.Compose(art,selected,view,pose,0,pitch,lib);auto decorated=renderer.output;
  minimum=std::min(minimum,renderer.decalPixels);maximum=std::max(maximum,renderer.decalPixels);
  if(renderer.decalPixels<8)throw std::runtime_error("Decal disappeared in a steering/pitch view");
  Selection plain=selected;for(auto &d:plain.decals[plain.car])d.design=-1;
  renderer.Compose(art,plain,view,pose,0,pitch,lib);
  const auto &body=pitch==0?art.views[view][pose].body:art.views[view][pose].pitch[pitch<0?pitch+2:pitch+1];
  for(int i=0;i<PixelCount;i++){
   for(int m=1;m<5;m++)if(ColorToInt(decorated[m][i])!=ColorToInt(renderer.output[m][i]))throw std::runtime_error("Decal modified geometry, finish or lamp maps");
   if(decorated[0][i].a!=renderer.output[0][i].a)throw std::runtime_error("Decal changed car silhouette");
   if(ColorToInt(decorated[0][i])==ColorToInt(renderer.output[0][i]))continue;
   int source=view?i/128*128+127-i%128:i;Color mask=body.pixels[5][source];
   if(body.pixels[0][source].a<128||mask.b>127||(mask.r<128&&mask.g<128))throw std::runtime_error("Decal escaped side paint mask");
  }
 }
 renderer.Compose(art,selected,0,1,0,0,lib);
 std::printf("DECAL MAP PROBE: all 30 poses, %d..%d ink pixels; masks, lamps, geometry and silhouette preserved\n",minimum,maximum);
}
static fs::path LocateAssets(){
 std::vector<fs::path> choices={fs::current_path()/"assets",fs::path(GetApplicationDirectory())/"../assets",fs::path(GetApplicationDirectory())/"assets"};
 for(const auto &path:choices)if(fs::exists(path/"cars/catalog.json"))return fs::weakly_canonical(path);
 throw std::runtime_error("Cannot find assets/cars/catalog.json. Run from the Number_Zero folder.");
}
int main(int argc,char **argv){
 bool raceMode=false,raceSmoke=false,limiterDeleteSmoke=false,trafficSmoke=false,decalSmoke=false,smoke=false,audioSmoke=false,resizeSmoke=false,manualSmoke=false,featureSmoke=false,glowSmoke=false,garageSmoke=false,blackPaint=false,scenerySmoke=false,effectsSmoke=false;int smokeCar=0,windowWidth=1280,windowHeight=720;for(int i=1;i<argc;i++){if(std::string(argv[i])=="--race")raceMode=true;if(std::string(argv[i])=="--race-smoke-test"){smoke=true;trafficSmoke=true;raceSmoke=true;raceMode=true;}if(std::string(argv[i])=="--smoke-test")smoke=true;if(std::string(argv[i])=="--traffic-smoke-test"){smoke=true;trafficSmoke=true;}if(std::string(argv[i])=="--decal-smoke-test"){smoke=true;decalSmoke=true;}if(std::string(argv[i])=="--resize-smoke-test"){smoke=true;resizeSmoke=true;}if(std::string(argv[i])=="--audio-smoke-test"){smoke=true;audioSmoke=true;}if(std::string(argv[i])=="--effects-smoke-test"){smoke=true;effectsSmoke=true;}if(std::string(argv[i])=="--scenery-smoke-test"){smoke=true;scenerySmoke=true;}if(std::string(argv[i])=="--black")blackPaint=true;if(std::string(argv[i])=="--garage-smoke-test"){smoke=true;garageSmoke=true;}if(std::string(argv[i])=="--glow-smoke-test"){smoke=true;glowSmoke=true;}if(std::string(argv[i])=="--manual")manualSmoke=true;if(std::string(argv[i])=="--limiter-delete")limiterDeleteSmoke=true;if(std::string(argv[i])=="--feature-smoke-test"){smoke=true;featureSmoke=true;}if(std::string(argv[i])=="--civic")smokeCar=1;if(std::string(argv[i])=="--testarossa")smokeCar=2;if(std::string(argv[i])=="--supra")smokeCar=3;if(std::string(argv[i])=="--ultrawide"){windowWidth=2560;windowHeight=1080;}if(std::string(argv[i])=="--superwide"){windowWidth=3840;windowHeight=1080;}if(std::string(argv[i])=="--tall"){windowWidth=960;windowHeight=900;}}
 SetConfigFlags(FLAG_WINDOW_RESIZABLE|FLAG_VSYNC_HINT|(smoke?FLAG_WINDOW_HIDDEN:0));InitWindow(windowWidth,windowHeight,"Number Zero | Highway Prototype");SetWindowMinSize(800,450);SetTargetFPS(60);SetExitKey(KEY_NULL);
 SetTraceLogLevel(LOG_WARNING);
 try{
 const auto assets=LocateAssets();auto root=assets.parent_path();fs::create_directories(root/"build/qa");
 nz::EngineAudio audio(!smoke||audioSmoke);
 if(audioSmoke&&!audio.Ready())throw std::runtime_error("Audio smoke test needs an available output device");
 ShaderProgram carShader(assets/"shaders/car.fs"),roadShader(assets/"shaders/road.fs"),resolveShader(assets/"shaders/car_resolve.fs"),glowShader(assets/"shaders/lamp_glow.fs"),propShader(assets/"shaders/highway_prop.fs"),motionShader(assets/"shaders/motion_blur.fs"),hudGlowShader(assets/"shaders/hud_glow.fs");
 HighwayRenderer highway(assets);DecalLibrary decals(assets);TrafficRenderer trafficRenderer(assets);nz::TrafficWorld traffic;bool trafficActive=false;
 std::array<CarArt,nz::Cars.size()> art;for(size_t i=0;i<art.size();i++)art[i]=LoadCar(assets,nz::Cars[i]);CarRenderer car,opponentCar;
 auto initialView=nz::MakeViewport(GetScreenWidth(),GetScreenHeight());Width=initialView.width;Height=initialView.height;
 RenderTexture2D target=LoadRenderTexture(Width,Height);SetTextureFilter(target.texture,TEXTURE_FILTER_POINT);
 EffectLayers layers;layers.Resize(Width,Height);EffectSettings effects;nz::TailHistory trails;
 Selection selected;selected.manual=manualSmoke;selected.car=smokeCar;selected.RestorePaint();if(smokeCar==1)selected.paint=RGBA(42,89,149);if(blackPaint){selected.paint=RGBA(28,31,36);selected.twoTone=false;}nz::Vehicle vehicle;bool driving=false,paused=false,quit=false,heads=true,tails=true,muted=false;
 nz::RaceSession race;nz::RaceTelemetry raceTelemetry(root/"build/qa/race_telemetry.csv");Selection opponentChoice;opponentChoice.car=2;opponentChoice.paint=WHITE;opponentChoice.twoTone=false;opponentChoice.blackHood=false;
 bool decalEditor=decalSmoke;int decalView=0,decalLayer=0;
 if(limiterDeleteSmoke)selected.limiterDeleted[selected.car]=true;
 const auto decalSave=root/"build/customisation.cfg";if(!smoke){LoadDecalSettings(decalSave,selected,decals);if(smokeCar)selected.SelectCar(smokeCar);if(manualSmoke)selected.manual=true;if(limiterDeleteSmoke)selected.limiterDeleted[selected.car]=true;}
 if(decalSmoke){selected.paint=RGBA(42,89,149);selected.twoTone=false;selected.decals[selected.car][0].design=decals.Find("17_flowing_checkerboard");}
 if(decalSmoke){
  ProbeDecalViews(car,art[selected.car],selected,decals);
  if(nz::Cars[selected.car].popupHeadlights){
   int changed=0;
   for(int view=0;view<2;view++)for(int pose=0;pose<3;pose++)for(int pitch=-2;pitch<=2;pitch++){
    car.Compose(art[selected.car],selected,view,pose,0,pitch,decals,true);auto raised=car.output[0];
    car.Compose(art[selected.car],selected,view,pose,0,pitch,decals,false);int difference=0;
    for(int i=0;i<PixelCount;i++)difference+=ColorToInt(raised[i])!=ColorToInt(car.output[0][i]);
    if(!difference)throw std::runtime_error("Popup headlights did not retract in a body pose");
    changed+=difference;
   }
   std::printf("POPUP PROBE: all 30 poses change raised/closed geometry, %d pixels total\n",changed);
  }
  {
   Selection previous;LoadDecalSettings(root/"build/customisation.cfg",previous,decals);previous.CapturePaint();
   SaveDecalSettings(root/"build/qa/garage-upgrade-test.cfg",previous,decals);Selection upgraded;LoadDecalSettings(root/"build/qa/garage-upgrade-test.cfg",upgraded,decals);
   if(previous.limiterDeleted!=upgraded.limiterDeleted||previous.finishes!=upgraded.finishes||previous.decals!=upgraded.decals||previous.car!=upgraded.car)throw std::runtime_error("Existing garage save upgrade lost customisations");
  }

  Selection stored=selected;stored.limiterDeleted[0]=true;stored.limiterDeleted[1]=false;auto &d=stored.decals[(selected.car+1)%nz::Cars.size()][2];d.design=decals.Find("15_minimal_triple_stripe");d.forward=-.231234567f;d.height=.81234567f;d.width=3.11234567f;d.angle=-17.5f;d.tint=0xf45577ff;
  auto path=root/"build/qa/decal-save-test.cfg";SaveDecalSettings(path,stored,decals);Selection restored;LoadDecalSettings(path,restored,decals);
  if(restored.limiterDeleted!=stored.limiterDeleted)throw std::runtime_error("Per-car limiter delete save/reload failed");
  if(restored.decals!=stored.decals)throw std::runtime_error("Decal placement save/reload failed");
  stored.CapturePaint();if(restored.finishes!=stored.finishes||restored.car!=stored.car||restored.manual!=stored.manual)throw std::runtime_error("Garage paint save/reload failed");
  Selection colours=selected;colours.paint=RGBA(17,73,129);colours.twoTone=false;colours.blackHood=true;colours.hue=211.234567f;colours.saturation=.81234567f;colours.value=.61234567f;
  colours.SelectCar((colours.car+1)%nz::Cars.size());colours.paint=RGBA(191,37,66);colours.twoTone=true;colours.blackHood=false;colours.hue=7.7654321f;colours.saturation=.41234567f;colours.value=.91234567f;colours.manual=true;colours.CapturePaint();
  auto paintPath=root/"build/qa/garage-paint-test.cfg";SaveDecalSettings(paintPath,colours,decals);Selection loaded;LoadDecalSettings(paintPath,loaded,decals);
  if(loaded.finishes!=colours.finishes||loaded.car!=colours.car||!loaded.manual||ColorToInt(loaded.paint)!=ColorToInt(colours.paint))throw std::runtime_error("Two-car paint restart roundtrip failed");
  loaded.SelectCar((loaded.car+(int)nz::Cars.size()-1)%nz::Cars.size());if(ColorToInt(loaded.paint)!=ColorToInt(colours.finishes[loaded.car].paint)||!loaded.blackHood)throw std::runtime_error("Switching cars lost saved paint");
  auto legacyPath=root/"build/qa/decal-legacy-test.cfg";{std::ofstream legacy(legacyPath);legacy.precision(std::numeric_limits<float>::max_digits10);legacy<<"NZ_DECALS_1\n";for(int c=0;c<(int)nz::Cars.size();c++)for(int l=0;l<3;l++){const auto &d=stored.decals[c][l];legacy<<c<<' '<<l<<' '<<(d.design>=0?decals.entries[d.design].id:"none")<<' '<<d.forward<<' '<<d.height<<' '<<d.width<<' '<<d.angle<<' '<<d.tint<<'\n';}}
  Selection legacy;legacy.limiterDeleted.fill(true);LoadDecalSettings(legacyPath,legacy,decals);if(std::any_of(legacy.limiterDeleted.begin(),legacy.limiterDeleted.end(),[](bool b){return b;}))throw std::runtime_error("Legacy save should retain stock limiters");
  if(legacy.decals!=stored.decals)throw std::runtime_error("Legacy decals were lost during migration");
  std::puts("GARAGE SAVE PROBE: both paint colours, hood/two-tone/HSV, car switching, gearbox, decals and legacy migration pass");
 }
 int indicator=0;float sceneClock=0;double accumulator=0;int frame=0;float bestSpeed=0;const char *qaName=smokeCar==3?"supra":smokeCar==2?"testarossa":smokeCar==1?"civic":"silvia";
 while(!WindowShouldClose()&&!quit){
  frame++;framePress.fill(false);for(int key=GetKeyPressed();key>0;key=GetKeyPressed())if(key<(int)framePress.size())framePress[key]=true;float dt=std::min(GetFrameTime(),.1f);if(smoke)dt=1.f/60;
  if(resizeSmoke){if(frame==140)SetWindowSize(2560,1080);if(frame==300)SetWindowSize(960,900);if(frame==450)SetWindowSize(1280,720);}
  if(Pressed(KEY_F11))ToggleBorderlessWindowed();
  auto viewport=nz::MakeViewport(GetScreenWidth(),GetScreenHeight());
  if(viewport.width!=Width||viewport.height!=Height){
   Width=viewport.width;Height=viewport.height;UnloadRenderTexture(target);target=LoadRenderTexture(Width,Height);SetTextureFilter(target.texture,TEXTURE_FILTER_POINT);layers.Resize(Width,Height);
  }
  float scale=(float)viewport.scale;sceneClock+=dt;
  Rectangle destination{(GetScreenWidth()-Width*scale)*.5f,(GetScreenHeight()-Height*scale)*.5f,Width*scale,Height*scale};Vector2 nativeMouse=GetMousePosition(),mouse{(nativeMouse.x-destination.x)/scale,(nativeMouse.y-destination.y)/scale};
  if(smoke&&!garageSmoke&&!decalSmoke&&frame==30){driving=true;vehicle=nz::Vehicle{};if(scenerySmoke)vehicle.lateral=5.25;vehicle.automatic=!selected.manual;vehicle.speedLimiterDeleted=selected.limiterDeleted[selected.car];vehicle.rpm=nz::Cars[selected.car].idle;if(raceMode)race.Start(vehicle,nz::Cars[selected.car],!selected.manual,selected.limiterDeleted[selected.car]);}
  if(Pressed(KEY_H))heads=!heads;
  if(Pressed(KEY_T))tails=!tails;
  if(Pressed(KEY_Z))indicator=indicator==-1?0:-1;
  if(Pressed(KEY_C))indicator=indicator==1?0:1;
  if(Pressed(KEY_X))indicator=indicator==2?0:2;
  if(featureSmoke){indicator=frame<16?2:frame<300?-1:frame<500?1:2;heads=!(frame>=16&&frame<30)&&!(frame>=640&&frame<690);}
  if(Pressed(KEY_M))muted=!muted;
  if(effectsSmoke&&frame>=580&&frame<=584){paused=true;effects.trails=frame==581||frame>=583;effects.motionBlur=frame>=582;}
  if(effectsSmoke&&frame==585)paused=false;
  nz::Input input;
  if(driving){
   if(!smoke&&Pressed(KEY_ESCAPE))paused=!paused;
   if(!smoke&&!IsWindowFocused())paused=true;
   if(Pressed(KEY_TAB)){driving=false;paused=false;accumulator=0;}
   if(Pressed(KEY_R)){trails.Clear();trafficActive=false;vehicle=nz::Vehicle{};vehicle.automatic=!selected.manual;vehicle.speedLimiterDeleted=selected.limiterDeleted[selected.car];vehicle.rpm=nz::Cars[selected.car].idle;if(raceMode)race.Start(vehicle,nz::Cars[selected.car],!selected.manual,selected.limiterDeleted[selected.car]);}
   if(smoke){double t=(frame-30)/60.;double cutoff=selected.car==2?8.:10.;input.throttle=t<cutoff?1:0;input.brake=t>=cutoff?1:0;input.steer=t>3&&t<4?.65:t>6&&t<7?-.65:0;}
   else{input.throttle=(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))?1:0;input.brake=(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_SPACE))?1:0;input.steer=((IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))?1:0)-((IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))?1:0);}
   if(!paused&&!vehicle.automatic&&(!raceMode||race.phase==nz::RacePhase::Racing)){
    if(Pressed(KEY_Q))nz::Shift(vehicle,nz::Cars[selected.car],-1);
    if(Pressed(KEY_E)||(smoke&&vehicle.rpm>nz::Cars[selected.car].shiftRpm-100))nz::Shift(vehicle,nz::Cars[selected.car],1);
   }
   if(!trafficActive){
    auto other=nz::RaceSession::Observer(race.opponent);traffic.Reset({vehicle.distance,vehicle.lateral,vehicle.speed},12345,raceMode?&other:nullptr);trafficActive=true;
    if(trafficSmoke&&!raceSmoke){vehicle.speed=28;for(size_t i=0;i<traffic.cars.size();i++){auto &a=traffic.cars[i];a.type=(int)i%5;a.station=vehicle.distance+10+(i%4)*8+(i/4)*30;a.cruise=a.speed=(78+(i%5)*4)/3.6;}}
   }
   if(raceSmoke&&frame==120&&(std::abs(vehicle.speed-nz::RollingSpeed)>.001||std::abs(race.opponent.speed-nz::RollingSpeed)>.001))throw std::runtime_error("Race rolling start did not hold both cars at 50km/h");
   if(raceSmoke&&frame==820&&race.phase==nz::RacePhase::Racing){vehicle.distance=race.startLine+nz::RaceDistance-.1;vehicle.speed=30;race.opponent.distance=vehicle.distance-20;race.opponent.speed=20;}
   if(!paused){accumulator+=dt;while(accumulator>=1./120.){
    if(raceMode){race.Step(vehicle,nz::Cars[selected.car],input,traffic,1./120.);raceTelemetry.Record(race,vehicle);}
    else{nz::Step(vehicle,nz::Cars[selected.car],input,1./120.);if(!smoke||trafficSmoke){traffic.Step({vehicle.distance,vehicle.lateral,vehicle.speed},1./120.);if(traffic.ResolveContact(vehicle))trails.Clear();}}
    accumulator-=1./120.;
   }}
  }
  if(decalSmoke){decalView=frame<45?0:1;if(frame==90){decalEditor=false;driving=true;vehicle.automatic=!selected.manual;vehicle.speedLimiterDeleted=selected.limiterDeleted[selected.car];}if(frame==120)vehicle.pitch=1.5;}
  audio.Update(nz::DrivingSound(vehicle,selected.car,driving,paused,muted));
  bool brake=driving&&vehicle.brake>.1;auto env=MakeEnvironment(!driving,vehicle,heads,tails,brake,sceneClock,indicator);
  if(!driving&&decalEditor){env.studio=true;env.heads=env.tails=false;env.indicatorLeft=env.indicatorRight=0;env.lights.clear();env.streetCount=0;PixelsPerMetre=52;env.anchor={186+(Width-640)*.5f,228+(Height-360)*.5f};}
  int pose=1;float spriteYaw=driving?nz::SpriteYaw(vehicle.yaw):0;if(spriteYaw!=0)pose=spriteYaw>0?2:0;
  int wheel=driving?(vehicle.speed*3.6>75?8:(int)std::floor(vehicle.distance/(2*nz::Pi*nz::Cars[selected.car].radius)*8)%8):0;
  car.Compose(art[selected.car],selected,driving?1:decalEditor?decalView:0,pose,wheel,driving?std::clamp((int)std::lround(vehicle.pitch/1.5),-2,2):0,decals,heads);
  if(decalSmoke&&(frame==12||frame==60||frame==180)){if(car.decalPixels<8)throw std::runtime_error("Projected decal disappeared from a car view");std::printf("DECAL PROBE %s view %d: %d painted-side pixels\n",qaName,driving?1:decalView,car.decalPixels);}
  Vector2 carAnchor=driving?Project(env.anchor,0,(float)vehicle.lateral):env.anchor;
  float spriteScale=PixelsPerMetre/26.f;
  Vector2 origin{std::round(carAnchor.x-64*spriteScale),std::round(carAnchor.y-79*spriteScale)};float depth=driving?WorldToCamera(0,(float)vehicle.lateral,0).z:0;
  car.Shade(carShader,env,origin,depth);
  if(effectsSmoke&&frame==12)car.ProbeComposite(resolveShader);
  if(trafficSmoke&&frame==12){trafficRenderer.ProbeDepth();trafficRenderer.ProbeLighting(roadShader);}
  if(scenerySmoke&&frame==12)highway.ProbeOcclusion(propShader,env);
  if(glowSmoke&&(frame==12||frame==580))car.ProbeGlow(glowShader,env);
  if(!driving){trails.Clear();trafficActive=false;}else if(!paused)trails.Update(vehicle,selected.car,tails,brake,car.TailSources(origin,carAnchor));
  bool showTraffic=driving&&(!smoke||trafficSmoke);if(showTraffic)trafficRenderer.Prepare(traffic,vehicle,carShader,env);
  Environment rivalLight=env;Vector2 rivalOrigin{};float rivalDepth=0,rivalStation=(float)(race.opponent.distance-vehicle.distance);bool showRival=driving&&raceMode;
  if(showRival){
   rivalLight.heads=rivalLight.tails=true;rivalLight.brake=race.opponent.brake>.1;rivalLight.indicatorLeft=rivalLight.indicatorRight=0;rivalLight.yaw=nz::SpriteYaw(race.opponent.yaw);
   auto local=[&](float forward,float side,Vector3 colour,float power,float radius){if(rivalLight.lights.size()<32){float yaw=rivalLight.yaw;rivalLight.lights.push_back({WorldToCamera(rivalStation+forward*std::cos(yaw)-side*std::sin(yaw),(float)race.opponent.lateral+forward*std::sin(yaw)+side*std::cos(yaw),.5f),colour,power,radius});}};
   for(int side:{-1,1})local(2,side*.56f,{1,.88f,.65f},.42f,3.2f);
   local(-2,0,{1,.08f,.04f},rivalLight.brake?1.35f:.45f,3.5f);
   auto a=Project(env.anchor,rivalStation,(float)race.opponent.lateral);rivalOrigin={std::round(a.x-64*spriteScale),std::round(a.y-79*spriteScale)};rivalDepth=WorldToCamera(rivalStation,(float)race.opponent.lateral,0).z;
   if(rivalStation> -28&&rivalStation<nz::VisibleAhead+35){int rp=rivalLight.yaw>0?2:rivalLight.yaw<0?0:1;int rw=race.opponent.speed*3.6>75?8:(int)std::floor(race.opponent.distance/(2*nz::Pi*nz::Cars[2].radius)*8)%8;
    opponentCar.Compose(art[2],opponentChoice,1,rp,rw,std::clamp((int)std::lround(race.opponent.pitch/1.5),-2,2),decals,true);opponentCar.Shade(carShader,rivalLight,rivalOrigin,rivalDepth);
   }
  }
  trafficRenderer.Opponent(opponentCar,rivalLight,rivalOrigin,rivalDepth,rivalStation,(float)race.opponent.lateral,rivalLight.yaw,showRival);
  roadShader.Int("raceMode",driving&&raceMode&&race.phase!=nz::RacePhase::Countdown?1:0);roadShader.Float("raceFinishStation",(float)(race.startLine+nz::RaceDistance-vehicle.distance));
  trafficRenderer.RoadLights(roadShader,showTraffic?traffic:nz::TrafficWorld{},vehicle,showTraffic);
  Vector2 blur=MotionPixels(vehicle,driving&&effects.motionBlur);
  BeginTextureMode(layers.world);ClearBackground(Ink);if(env.studio)StudioBackdrop();else Road(roadShader,car,highway,env,origin,depth,trafficRenderer.shadowAtlas.texture);
  if(driving&&effects.trails)DrawTrails(trails,env,vehicle);
  EndTextureMode();
  BeginTextureMode(layers.props);ClearBackground(BLANK);
  rlSetBlendFactorsSeparate(RL_SRC_ALPHA,RL_ONE_MINUS_SRC_ALPHA,RL_ONE,RL_ONE_MINUS_SRC_ALPHA,RL_FUNC_ADD,RL_FUNC_ADD);BeginBlendMode(BLEND_CUSTOM_SEPARATE);
  highway.Draw(propShader,env,car.textures,origin,depth,0);if(showTraffic)trafficRenderer.Draw(0);EndBlendMode();if(showTraffic)trafficRenderer.Glow(glowShader,traffic,env);EndTextureMode();
  BeginTextureMode(layers.depth);ClearBackground(BLANK);rlSetBlendFactors(RL_ONE,RL_ZERO,RL_FUNC_ADD);BeginBlendMode(BLEND_CUSTOM);
  highway.Draw(propShader,env,car.textures,origin,depth,2);if(showTraffic)trafficRenderer.Draw(2);EndBlendMode();EndTextureMode();
  BeginTextureMode(layers.world);BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);Layer(layers.props.texture);EndBlendMode();EndTextureMode();
  nz::Vehicle instruments=vehicle;if(effectsSmoke&&frame==584)instruments.rpm=nz::Cars[selected.car].redline;
  BeginTextureMode(layers.hud);ClearBackground(BLANK);if(driving)Hud(selected,instruments,heads,tails,audio.Ready(),muted,indicator,sceneClock);EndTextureMode();
  BeginTextureMode(target);ClearBackground(Ink);
  motionShader.Vec2("resolution",{(float)Width,(float)Height});motionShader.Vec2("blurPixels",blur);motionShader.Vec2("screenAnchor",env.anchor);motionShader.Float("pixelsPerMetre",PixelsPerMetre);motionShader.Float("visibleAhead",nz::VisibleAhead);motionShader.Float("driving",driving?1.f:0.f);
  BeginShaderMode(motionShader.value);Layer(layers.world.texture);EndShaderMode();
  car.Draw(resolveShader,glowShader,env,origin,layers.props.texture,layers.depth.texture,blur,depth);
  if(driving){
   hudGlowShader.Vec2("resolution",{(float)Width,(float)Height});BeginBlendMode(BLEND_ADDITIVE);BeginShaderMode(hudGlowShader.value);Layer(layers.hud.texture);EndShaderMode();EndBlendMode();
   Hud(selected,instruments,heads,tails,audio.Ready(),muted,indicator,sceneClock);
   if(showTraffic)trafficRenderer.Warnings(traffic,vehicle,env);
   if(raceMode){bool retry=false,garage=false;RaceHud(race,vehicle,mouse,retry,garage);
    if(retry){race.Start(vehicle,nz::Cars[selected.car],!selected.manual,selected.limiterDeleted[selected.car]);trafficActive=false;trails.Clear();accumulator=0;}
    if(garage){driving=false;paused=false;trafficActive=false;accumulator=0;}
   }
   if(paused)PauseMenu(effects,mouse,paused,driving,accumulator);
  }
  else{bool start=false;Vector2 uiOffset{(Width-640)*.5f,(Height-360)*.5f};rlPushMatrix();rlTranslatef(uiOffset.x,uiOffset.y,0);if(decalEditor){bool done=DecalEditor(selected,decals,decalView,decalLayer,{mouse.x-uiOffset.x,mouse.y-uiOffset.y},dt);if(done){decalEditor=false;if(!smoke)SaveDecalSettings(decalSave,selected,decals);}}else Garage(selected,{mouse.x-uiOffset.x,mouse.y-uiOffset.y},start,quit,heads,indicator,decalEditor,raceMode);rlPopMatrix();if(start){if(!smoke)SaveDecalSettings(decalSave,selected,decals);driving=true;paused=false;accumulator=0;vehicle=nz::Vehicle{};vehicle.automatic=!selected.manual;vehicle.speedLimiterDeleted=selected.limiterDeleted[selected.car];vehicle.rpm=nz::Cars[selected.car].idle;if(raceMode)race.Start(vehicle,nz::Cars[selected.car],!selected.manual,selected.limiterDeleted[selected.car]);}}
  EndTextureMode();BeginDrawing();ClearBackground(RGBA(5,9,14));DrawTexturePro(target.texture,{0,0,(float)Width,-(float)Height},destination,{0,0},0,WHITE);EndDrawing();
  bestSpeed=std::max(bestSpeed,(float)(vehicle.speed*3.6));
  if(smoke&&(frame==12||(trafficSmoke&&(frame==180||frame==450||frame==800))||(decalSmoke&&(frame==60||frame==180))||frame==580||frame==865||(effectsSmoke&&frame>=580&&frame<=584)||(scenerySmoke&&(frame==120||frame==170||frame==280||frame==350||frame==480||frame==720))||(garageSmoke&&(frame==90||frame==150||frame==210||frame==300||frame==450||frame==600))||(featureSmoke&&(frame==18||frame==230||frame==440||frame==650))||(resizeSmoke&&(frame==200||frame==360)))){std::string name=std::string(qaName)+(blackPaint?"_black":"")+"_"+std::to_string(Width)+"x"+std::to_string(Height)+(garageSmoke?"_orbit"+std::to_string(frame)+".png":frame==12?"_garage.png":frame==580&&!effectsSmoke?"_driving.png":frame==865?"_braking.png":"_frame"+std::to_string(frame)+".png");Image capture=LoadImageFromTexture(target.texture);ImageFlipVertical(&capture);bool saved=ExportImage(capture,(root/"build/qa"/name).string().c_str());UnloadImage(capture);if(!saved)throw std::runtime_error("Could not save graphics test frame");}
  if(decalSmoke&&frame>=180){std::printf("DECAL EDITOR SMOKE %s complete\n",qaName);quit=true;}
  if(garageSmoke&&frame>=720){std::printf("GARAGE FINISH SMOKE %s complete\n",qaName);quit=true;}
  if(raceSmoke&&frame>=900){if(trafficRenderer.rivalDrawn<50||race.phase!=nz::RacePhase::Finished||race.winner!=nz::RaceWinner::Player)throw std::runtime_error("Race renderer/result smoke failed");std::printf("RACE SMOKE: rolling start, white rival (%d passes), traffic and 10km result pass\n",trafficRenderer.rivalDrawn);quit=true;}
  if(trafficSmoke&&!raceSmoke&&frame>=900){if(trafficRenderer.totalWarnings<1)throw std::runtime_error("Traffic entry warnings were never drawn");if(trafficRenderer.totalDrawn<50)throw std::runtime_error("Traffic was not rendered");std::printf("TRAFFIC GRAPHICS: %d actor passes, %d lane changes, %d contacts, %d warnings\n",trafficRenderer.totalDrawn,traffic.laneChanges,traffic.contacts,trafficRenderer.totalWarnings);quit=true;}
  if(smoke&&!garageSmoke&&!decalSmoke&&!trafficSmoke&&frame>=900){if(audioSmoke){std::printf("AUDIO SMOKE: %llu samples generated on the device callback\n",(unsigned long long)audio.Samples());if(audio.Samples()<nz::AudioRate*5)throw std::runtime_error("Audio callback did not run continuously");}std::printf("GRAPHICS SMOKE %s: max %.1fkm/h, stopped %.2fkm/h, 0-100 %.2fs\n",qaName,bestSpeed,vehicle.speed*3.6,vehicle.timeTo100);if(bestSpeed<100||vehicle.speed>1)throw std::runtime_error("Graphics driving smoke test failed");quit=true;}
 }
 if(!smoke)SaveDecalSettings(decalSave,selected,decals);
 audio.Close();opponentCar.Close();trafficRenderer.Close();decals.Close();car.Close();highway.Close();layers.Close();UnloadRenderTexture(target);UnloadShader(carShader.value);UnloadShader(roadShader.value);UnloadShader(resolveShader.value);UnloadShader(glowShader.value);UnloadShader(propShader.value);UnloadShader(motionShader.value);UnloadShader(hudGlowShader.value);CloseWindow();return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"ERROR: %s\n",e.what());CloseWindow();return 1;}
}
