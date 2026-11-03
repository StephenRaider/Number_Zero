#include "raylib.h"
#include "rlgl.h"
#include "vehicle.h"
#include "scene.h"
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
constexpr int Width=640,Height=360,SpriteSize=128,PixelCount=128*128;
constexpr float K=.70710678118f,ElevationSin=.5f,ElevationCos=.86602540378f,PixelsPerMetre=26.f;
const Color Ink{10,15,23,255},Panel{19,27,39,246},Muted{133,155,173,255},Paper{231,236,233,255},Mint{120,224,196,255},Amber{244,189,110,255};
const char *MapNames[]={"albedo","normal","pack","pack2","emit","mask"};
const char *PoseNames[]={"left","straight","right"};
static std::array<bool,512> framePress{};
static bool Pressed(int key){return key>=0&&key<(int)framePress.size()&&framePress[key];}
struct Maps { std::array<std::vector<Color>,6> pixels; };
struct PoseArt { Maps body;std::array<Maps,9> wheels; };
struct CarArt { std::array<std::array<PoseArt,3>,2> views; };
struct Selection { int car=0;Color paint{229,226,207,255};bool twoTone=true,blackHood=false;float hue=35.f,saturation=.12f,value=.90f; };
struct Light { Vector3 position;Vector3 colour;float power,radius; };
struct Environment { Vector2 anchor;std::vector<Light> lights;int streetCount=0;bool garage=false;float distance=0,lateral=0;bool heads=true,tails=true,brake=false; };
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
  for(int f=0;f<9;f++)art.views[view][pose].wheels[f]=LoadMaps(folder,label+"_wheels_f"+std::to_string(f));
 }
 return art;
}
struct ShaderProgram {
 Shader value{};std::map<std::string,int> locations;
 explicit ShaderProgram(const fs::path &path){value=LoadShader(nullptr,path.string().c_str());if(!IsShaderValid(value)||value.id==rlGetShaderIdDefault())throw std::runtime_error("Shader compilation failed: "+path.string());}

 int At(const char *name){auto it=locations.find(name);if(it!=locations.end())return it->second;int loc=GetShaderLocation(value,name);locations[name]=loc;return loc;}
 void Float(const char *name,float v){int loc=At(name);if(loc>=0)SetShaderValue(value,loc,&v,SHADER_UNIFORM_FLOAT);}
 void Int(const char *name,int v){int loc=At(name);if(loc>=0)SetShaderValue(value,loc,&v,SHADER_UNIFORM_INT);}
 void Vec2(const char *name,Vector2 v){int loc=At(name);if(loc>=0)SetShaderValue(value,loc,&v,SHADER_UNIFORM_VEC2);}
 void Texture(const char *name,Texture2D tex){int loc=At(name);if(loc>=0)SetShaderValueTexture(value,loc,tex);}
 void Lights(const Environment &e){
  std::array<float,72> positions{},colours{};std::array<float,24> powers{},radii{};
  if(e.lights.size()>24)throw std::runtime_error("Light capacity exceeded");
  int n=(int)e.lights.size();
  for(int i=0;i<n;i++){const auto &l=e.lights[i];positions[i*3]=l.position.x;positions[i*3+1]=l.position.y;positions[i*3+2]=l.position.z;colours[i*3]=l.colour.x;colours[i*3+1]=l.colour.y;colours[i*3+2]=l.colour.z;powers[i]=l.power;radii[i]=l.radius;}
  Int("lightCount",n);Int("streetCount",e.streetCount);
  for(auto pair:{std::pair<const char*,const float*>{"lightPosition[0]",positions.data()},{"lightColour[0]",colours.data()}}){int loc=At(pair.first);if(loc>=0)SetShaderValueV(value,loc,pair.second,SHADER_UNIFORM_VEC3,n);}
  int loc=At("lightPower[0]");if(loc>=0)SetShaderValueV(value,loc,powers.data(),SHADER_UNIFORM_FLOAT,n);
  loc=At("lightRadius[0]");if(loc>=0)SetShaderValueV(value,loc,radii.data(),SHADER_UNIFORM_FLOAT,n);
  Vec2("screenAnchor",e.anchor);Float("headlights",e.heads?1.f:0.f);Float("taillights",e.tails?1.f:0.f);Float("braking",e.brake?1.f:0.f);
 }
};
struct CarRenderer {
 std::array<std::vector<Color>,5> output;std::array<Texture2D,5> textures{};
 int lastCar=-1,lastView=-1,lastPose=-1,lastFrame=-1;Color lastPaint{};bool lastHood=false,lastTwoTone=false;
 CarRenderer(){Image im=GenImageColor(128,128,BLANK);for(int i=0;i<5;i++){output[i].resize(PixelCount);textures[i]=LoadTextureFromImage(im);SetTextureFilter(textures[i],TEXTURE_FILTER_POINT);SetTextureWrap(textures[i],TEXTURE_WRAP_CLAMP);}UnloadImage(im);}
 void Compose(const CarArt &art,const Selection &choice,int view,int pose,int frame){
  if(lastCar==choice.car&&lastView==view&&lastPose==pose&&lastFrame==frame&&ColorToInt(lastPaint)==ColorToInt(choice.paint)&&lastHood==choice.blackHood&&lastTwoTone==choice.twoTone)return;
  lastCar=choice.car;lastView=view;lastPose=pose;lastFrame=frame;lastPaint=choice.paint;lastHood=choice.blackHood;lastTwoTone=choice.twoTone;
  const auto &body=art.views[view][pose].body,&wheels=art.views[view][pose].wheels[frame];
  const float original[2][3]={{255,234,186},{219,229,237}};
  for(int i=0;i<PixelCount;i++){
   bool b=body.pixels[0][i].a>=128,w=wheels.pixels[0][i].a>=128;
   unsigned bd=body.pixels[2][i].r*256u+body.pixels[2][i].g,wd=wheels.pixels[2][i].r*256u+wheels.pixels[2][i].g;
   const Maps &src=w&&(!b||wd>bd)?wheels:body;
   for(int m=0;m<5;m++)output[m][i]=src.pixels[m][i];
   if(&src==&body&&b){
    Color mask=body.pixels[5][i],paint=choice.paint;int channel=mask.b>127?2:mask.g>127?1:mask.r>127?0:-1;
    if(channel<0)continue;
    if(channel==1&&choice.twoTone&&choice.car==0)paint=RGBA(174,174,174);
    if(channel==2&&choice.blackHood)paint=RGBA(29,32,36);
    const float *base=original[choice.car];float lower[3]={174,174,174};if(channel==1&&choice.car==0)base=lower;
    Color source=output[0][i];output[0][i]=RGBA(std::clamp((int)std::lround(paint.r*source.r/base[0]),0,255),std::clamp((int)std::lround(paint.g*source.g/base[1]),0,255),std::clamp((int)std::lround(paint.b*source.b/base[2]),0,255),source.a);
   }
  }
  if(view==1){
   // A mirrored sprite also needs mirrored camera-space normals, not just flipped colour.
   for(int y=0;y<128;y++)for(int x=0;x<64;x++)for(int m=0;m<5;m++)std::swap(output[m][y*128+x],output[m][y*128+127-x]);
   for(int i=0;i<PixelCount;i++){unsigned nx=65535u-(output[1][i].r*256u+output[1][i].g);output[1][i].r=(unsigned char)(nx/256);output[1][i].g=(unsigned char)(nx%256);}
  }
  for(int i=0;i<5;i++)UpdateTexture(textures[i],output[i].data());
 }
 void Draw(ShaderProgram &shader,const Environment &env,Vector2 origin,float baseDepth){
  shader.Lights(env);shader.Vec2("carOrigin",origin);shader.Float("carDepth",baseDepth);
  BeginShaderMode(shader.value);shader.Texture("normalMap",textures[1]);shader.Texture("depthMap",textures[2]);shader.Texture("auxiliaryMap",textures[3]);shader.Texture("emissionMap",textures[4]);DrawTexture(textures[0],(int)std::round(origin.x),(int)std::round(origin.y),WHITE);EndShaderMode();
 }
 void Close(){for(auto tex:textures)UnloadTexture(tex);}
};
static Environment MakeEnvironment(bool garage,const nz::Vehicle &v,bool heads,bool tails,bool brake){
 Environment e;e.garage=garage;e.anchor=garage?Vector2{186,214}:Vector2{250,234};e.distance=(float)v.distance;e.lateral=garage?0.f:(float)v.lateral;e.heads=heads;e.tails=tails;e.brake=brake;
 if(garage){e.lights={{{-2,3.2f,3.4f},{1,.83f,.61f},1.7f,11},{{2.6f,2.6f,2.2f},{.52f,.75f,1},1.1f,10},{{0,5,1.0f},{1,.96f,.85f},.7f,12}};e.streetCount=3;}
 else{
  // Keep fixed world-space posts alive until their full light tail clears every visible receiver.
  float minS=1e9f,maxS=-1e9f;
  for(float x:{-128.f,(float)Width+128})for(float y:{-128.f,(float)Height+128}){
   float cx=(x-e.anchor.x)/PixelsPerMetre,cy=(e.anchor.y-y)/PixelsPerMetre;
   float s=nz::NortheastGround(cx,cy).first;minS=std::min(minS,s);maxS=std::max(maxS,s);
  }
  for(int side:{-1,1}){
   float phase=side>0?12.f:0.f;
   auto [first,last]=nz::StreetRange(v.distance,minS,maxS,side);
   for(int k=first;k<=last;k++){
    float s=(float)(k*24+phase-v.distance);e.lights.push_back({WorldToCamera(s,side*4.2f,5.4f),{1.f,.85f,.66f},2.4f,11.5f});
   }
  }
  e.streetCount=(int)e.lights.size();
  if(heads)e.lights.push_back({WorldToCamera(2.5f,(float)v.lateral,.5f),{1,.88f,.65f},.85f,5});
  if(tails||brake)e.lights.push_back({WorldToCamera(-2.f,(float)v.lateral,.45f),{1,.08f,.04f},brake?1.35f:.45f,3.5f});
 }
 return e;
}
static void Road(ShaderProgram &shader,CarRenderer &car,const Environment &env,Vector2 origin,float depth){
 shader.Lights(env);shader.Vec2("resolution",{(float)Width,(float)Height});shader.Float("travelled",env.distance);shader.Float("garage",env.garage?1.f:0.f);shader.Float("playerLateral",env.lateral);shader.Vec2("carOrigin",origin);shader.Float("carDepth",depth);
 BeginShaderMode(shader.value);shader.Texture("carAlbedo",car.textures[0]);shader.Texture("carFront",car.textures[2]);shader.Texture("carBack",car.textures[3]);DrawRectangle(0,0,Width,Height,WHITE);EndShaderMode();
}
static void Quad(Vector2 a,Vector2 b,Vector2 c,Vector2 d,Color colour){DrawTriangle(a,b,c,colour);DrawTriangle(a,c,d,colour);}
static void RoadFurniture(const Environment &e){
 if(e.garage)return;
 const int marker=(int)std::floor(e.distance/5.);
 for(int k=marker-18;k<=marker+18;k++)for(int side:{-1,1}){
  float s=(float)(k*5-e.distance),l=side*8.0f;auto a=Project(e.anchor,s,l),b=Project(e.anchor,s+5,l),c=Project(e.anchor,s+5,l,.5f),d=Project(e.anchor,s,l,.5f);
  Quad(a,b,c,d,RGBA(40,48,56));DrawLineEx(c,d,1,RGBA(91,102,111));
  Vector2 post=Project(e.anchor,s,l,.5f),bottom=Project(e.anchor,s,l);DrawLineEx(post,bottom,1,RGBA(107,119,121));
 }
 const int pole=(int)std::floor(e.distance/24.);
 for(int k=pole-3;k<=pole+4;k++)for(int side:{-1,1}){
  float s=(float)(k*24+(side>0?12:0)-e.distance),l=side*8.3f;auto bottom=Project(e.anchor,s,l),top=Project(e.anchor,s,l,5.4f),end=Project(e.anchor,s,side*4.2f,5.4f);
  if(bottom.x<-100||bottom.x>Width+100||bottom.y<-80||top.y>Height+100)continue;
  DrawLineEx(bottom,top,2,RGBA(49,60,71));DrawLineEx(top,end,2,RGBA(86,100,107));
  DrawCircleGradient((int)end.x,(int)end.y,12,RGBA(255,193,107,36),BLANK);DrawRectangle((int)end.x-2,(int)end.y-1,5,2,RGBA(255,228,173));
 }
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
static void Garage(Selection &s,Vector2 mouse,bool &start,bool &quit){
 DrawRectangle(0,0,Width,57,RGBA(9,15,23,235));Label("NUMBER ZERO",24,16,23);Label("NIGHT DRIVE  /  PROTOTYPE 01",25,42,9,Mint);
 Label("CHOOSE YOUR CAR",381,73,10,Muted);
 for(int i=0;i<2;i++){
  Rectangle r{378.f,91.f+i*49,240,42};Box(r,s.car==i?RGBA(36,58,65):Panel);
  Label(i==0?"SILVIA S13 K'S":"CIVIC TYPE R EK9",389,(int)r.y+8,12,s.car==i?Mint:Paper);
  Label(TextFormat("%.0f PS   %s   %.0f kg",nz::Cars[i].peakPS,nz::Cars[i].drive,nz::Cars[i].mass),389,(int)r.y+26,9,Muted);
  if(CheckCollisionPointRec(mouse,r)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){s.car=i;s.paint=i==0?RGBA(229,226,207):RGBA(225,230,222);s.twoTone=i==0;}
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
 if(Button({378,316,240,27},"START NIGHT DRIVE   [ENTER]",mouse,true)||Pressed(KEY_ENTER))start=true;
 Label(nz::Cars[s.car].engine,25,279,13,Paper);Label("STOCK  /  5MT AUTO SHIFT",25,300,10,Muted);Label("W / UP  THROTTLE     S / DOWN  BRAKE",25,322,9,Muted);Label("A / D  STEER     H  HEADLIGHTS     T  TAIL LIGHTS",25,338,8,Muted);
 if(Pressed(KEY_ESCAPE))quit=true;
}
static void Hud(const Selection &s,const nz::Vehicle &v,bool heads,bool tails){
 DrawRectangle(0,0,Width,26,RGBA(8,14,22,233));Label("NUMBER ZERO",15,8,12,Mint);Label("EXPRESSWAY  /  NIGHT LOOP",143,9,9,Muted);Label(TextFormat("%.2f km",v.distance/1000),534,8,11);
 Box({483,282,142,63},RGBA(10,19,28,238));Label(TextFormat("%03d",(int)std::lround(v.speed*3.6)),494,291,28);Label("km/h",553,305,10,Muted);Label(TextFormat("%d",v.gear+1),601,290,24,Mint);Label("AUTO",592,326,8,Muted);
 Label(nz::Cars[s.car].name,18,281,10,Paper);Label(TextFormat("%04d RPM",(int)v.rpm),18,299,11,Mint);
 float rpmFraction=(float)(v.rpm/nz::Cars[s.car].redline);DrawRectangle(18,316,256,5,RGBA(27,42,55));DrawRectangle(18,316,(int)(256*std::clamp(rpmFraction,0.f,1.f)),5,rpmFraction>.92f?Amber:Mint);
 DrawRectangle(291,294,5,27,RGBA(27,42,55));DrawRectangle(291,321-(int)(v.throttle*27),5,(int)(v.throttle*27),Mint);DrawRectangle(303,294,5,27,RGBA(27,42,55));DrawRectangle(303,321-(int)(v.brake*27),5,(int)(v.brake*27),RGBA(233,101,90));Label("G B",288,326,8,Muted);
 Label("W/S  THROTTLE/BRAKE   A/D  STEER   ESC  PAUSE   TAB  GARAGE",18,343,8,Muted);
 Label(heads?"HEAD ON":"HEAD OFF",18,40,8,heads?Amber:Muted);Label(tails?"TAIL ON":"TAIL OFF",78,40,8,tails?Amber:Muted);
 if(v.limiter)Label("STOCK SPEED LIMITER",343,324,9,Amber);
 if(std::abs(v.lateral)>5.9)Label("ROAD EDGE",345,309,9,Amber);
}
static fs::path LocateAssets(){
 std::vector<fs::path> choices={fs::current_path()/"assets",fs::path(GetApplicationDirectory())/"../assets",fs::path(GetApplicationDirectory())/"assets"};
 for(const auto &path:choices)if(fs::exists(path/"cars/catalog.json"))return fs::weakly_canonical(path);
 throw std::runtime_error("Cannot find assets/cars/catalog.json. Run from the Number_Zero folder.");
}
int main(int argc,char **argv){
 bool smoke=false,audioSmoke=false;int smokeCar=0;for(int i=1;i<argc;i++){if(std::string(argv[i])=="--smoke-test")smoke=true;if(std::string(argv[i])=="--audio-smoke-test"){smoke=true;audioSmoke=true;}if(std::string(argv[i])=="--civic")smokeCar=1;}
 SetConfigFlags(FLAG_WINDOW_RESIZABLE|FLAG_VSYNC_HINT|(smoke?FLAG_WINDOW_HIDDEN:0));InitWindow(1280,720,"Number Zero | Highway Prototype");SetWindowMinSize(800,450);SetTargetFPS(60);SetExitKey(KEY_NULL);
 SetTraceLogLevel(LOG_WARNING);
 try{
 const auto assets=LocateAssets();auto root=assets.parent_path();fs::create_directories(root/"build/qa");
 nz::EngineAudio audio(!smoke||audioSmoke);
 if(audioSmoke&&!audio.Ready())throw std::runtime_error("Audio smoke test needs an available output device");
 ShaderProgram carShader(assets/"shaders/car.fs"),roadShader(assets/"shaders/road.fs");
 std::array<CarArt,2> art={LoadCar(assets,nz::Cars[0]),LoadCar(assets,nz::Cars[1])};CarRenderer car;
 RenderTexture2D target=LoadRenderTexture(Width,Height);SetTextureFilter(target.texture,TEXTURE_FILTER_POINT);
 Selection selected;selected.car=smokeCar;if(smokeCar)selected.paint=RGBA(42,89,149);nz::Vehicle vehicle;bool driving=false,paused=false,quit=false,heads=true,tails=true,muted=false;
 double accumulator=0;int frame=0;float bestSpeed=0;const char *qaName=smokeCar?"civic":"silvia";
 while(!WindowShouldClose()&&!quit){
  frame++;framePress.fill(false);for(int key=GetKeyPressed();key>0;key=GetKeyPressed())if(key<(int)framePress.size())framePress[key]=true;float dt=std::min(GetFrameTime(),.1f);if(smoke)dt=1.f/60;
  float fit=std::min(GetScreenWidth()/(float)Width,GetScreenHeight()/(float)Height),scale=fit>=1?std::floor(fit):fit;
  Rectangle destination{(GetScreenWidth()-Width*scale)*.5f,(GetScreenHeight()-Height*scale)*.5f,Width*scale,Height*scale};Vector2 nativeMouse=GetMousePosition(),mouse{(nativeMouse.x-destination.x)/scale,(nativeMouse.y-destination.y)/scale};
  if(smoke&&frame==30){driving=true;vehicle=nz::Vehicle{};vehicle.rpm=nz::Cars[selected.car].idle;}
  if(Pressed(KEY_M))muted=!muted;
  nz::Input input;
  if(driving){
   if(!smoke&&Pressed(KEY_ESCAPE))paused=!paused;
   if(!smoke&&!IsWindowFocused())paused=true;
   if(Pressed(KEY_TAB)){driving=false;paused=false;accumulator=0;}
   if(Pressed(KEY_H))heads=!heads;
   if(Pressed(KEY_T))tails=!tails;
   if(Pressed(KEY_R)){vehicle=nz::Vehicle{};vehicle.rpm=nz::Cars[selected.car].idle;}
   if(smoke){double t=(frame-30)/60.;input.throttle=t<10?1:0;input.brake=t>=10?1:0;input.steer=t>3&&t<4?.65:t>6&&t<7?-.65:0;}
   else{input.throttle=(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))?1:0;input.brake=(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_SPACE))?1:0;input.steer=((IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))?1:0)-((IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))?1:0);}
   if(!paused){accumulator+=dt;while(accumulator>=1./120.){nz::Step(vehicle,nz::Cars[selected.car],input,1./120.);accumulator-=1./120.;}}
  }
  audio.Update(nz::DrivingSound(vehicle,selected.car,driving,paused,muted));
  bool brake=driving&&vehicle.brake>.1;auto env=MakeEnvironment(!driving,vehicle,heads,tails,brake);
  int pose=1;if(driving&&std::abs(vehicle.yaw)>.027)pose=vehicle.yaw>0?2:0;
  int wheel=driving?(vehicle.speed*3.6>75?8:(int)std::floor(vehicle.distance/(2*nz::Pi*nz::Cars[selected.car].radius)*8)%8):0;
  car.Compose(art[selected.car],selected,driving?1:0,pose,wheel);
  Vector2 carAnchor=driving?Project(env.anchor,0,(float)vehicle.lateral):env.anchor;
  Vector2 origin{std::round(carAnchor.x-64),std::round(carAnchor.y-79)};float depth=driving?WorldToCamera(0,(float)vehicle.lateral,0).z:0;
  BeginTextureMode(target);ClearBackground(Ink);Road(roadShader,car,env,origin,depth);RoadFurniture(env);car.Draw(carShader,env,origin,depth);
  if(driving){Hud(selected,vehicle,heads,tails);Label(!audio.Ready()?"AUDIO OFF":muted?"M  MUTED":"M  SOUND ON",155,40,8,Muted);if(paused){DrawRectangle(0,0,Width,Height,RGBA(3,9,17,175));Box({172,119,296,114});Label("PAUSED",286,137,20,Mint);Label("ESC  RESUME",275,170,11);Label("TAB  GARAGE     R  RESET DRIVE",218,192,10,Muted);}}
  else{bool start=false;Garage(selected,mouse,start,quit);if(start){driving=true;paused=false;accumulator=0;vehicle=nz::Vehicle{};vehicle.rpm=nz::Cars[selected.car].idle;}}
  EndTextureMode();BeginDrawing();ClearBackground(RGBA(5,9,14));DrawTexturePro(target.texture,{0,0,(float)Width,-(float)Height},destination,{0,0},0,WHITE);EndDrawing();
  bestSpeed=std::max(bestSpeed,(float)(vehicle.speed*3.6));
  if(smoke&&(frame==12||frame==580||frame==865)){std::string name=std::string(qaName)+(frame==12?"_garage.png":frame==580?"_driving.png":"_braking.png");Image capture=LoadImageFromTexture(target.texture);ImageFlipVertical(&capture);bool saved=ExportImage(capture,(root/"build/qa"/name).string().c_str());UnloadImage(capture);if(!saved)throw std::runtime_error("Could not save graphics test frame");}
  if(smoke&&frame>=900){if(audioSmoke){std::printf("AUDIO SMOKE: %llu samples generated on the device callback\n",(unsigned long long)audio.Samples());if(audio.Samples()<nz::AudioRate*5)throw std::runtime_error("Audio callback did not run continuously");}std::printf("GRAPHICS SMOKE %s: max %.1fkm/h, stopped %.2fkm/h, 0-100 %.2fs\n",qaName,bestSpeed,vehicle.speed*3.6,vehicle.timeTo100);if(bestSpeed<100||vehicle.speed>1)throw std::runtime_error("Graphics driving smoke test failed");quit=true;}
 }
 audio.Close();car.Close();UnloadRenderTexture(target);UnloadShader(carShader.value);UnloadShader(roadShader.value);CloseWindow();return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"ERROR: %s\n",e.what());CloseWindow();return 1;}
}
