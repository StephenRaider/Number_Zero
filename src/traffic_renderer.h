#pragma once
struct TrafficRenderer {
 struct Frame {std::array<Texture2D,5> textures{};std::vector<Color> albedo;std::vector<unsigned char> paint;};
 struct Actor {int id=0,lastType=-1,lastPose=-1,lastFrame=-1,lastPaint=-1;Texture2D albedo{};RenderTexture2D shaded{};Vector2 origin{};float depth=0,station=0;int type=0,pose=0,frame=0;bool brake=false;int indicator=0;};
 std::array<std::array<std::array<Frame,4>,3>,nz::TrafficTypes.size()> art;
 std::map<int,Actor> actors;std::vector<int> visible;int totalDrawn=0,totalWarnings=0;
 ShaderProgram composite,shadow;
 explicit TrafficRenderer(const fs::path &assets):composite(assets/"shaders/traffic_composite.fs"),shadow(assets/"shaders/traffic_shadow.fs"){
  const char *kinds[]={"albedo","normal","pack","pack2","emit"};
  for(int type=0;type<(int)nz::TrafficTypes.size();type++)for(int pose=0;pose<3;pose++)for(int frame=0;frame<4;frame++){
   auto &a=art[type][pose][frame];
   for(int k=0;k<5;k++){
    auto p=assets/"traffic"/nz::TrafficTypes[type].id/(std::string(PoseNames[pose])+"_f"+std::to_string(frame)+"_"+kinds[k]+".png");Image im=LoadImage(p.string().c_str());
    if(!IsImageValid(im)||im.width!=256||im.height!=256)throw std::runtime_error("Invalid traffic asset: "+p.string());
    ImageFormat(&im,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    a.textures[k]=LoadTextureFromImage(im);SetTextureFilter(a.textures[k],TEXTURE_FILTER_POINT);SetTextureWrap(a.textures[k],TEXTURE_WRAP_CLAMP);
    if(k==0){a.albedo.resize(256*256);std::memcpy(a.albedo.data(),im.data,a.albedo.size()*sizeof(Color));}
    if(k==2){a.paint.resize(256*256);auto data=(Color*)im.data;for(int i=0;i<256*256;i++)a.paint[i]=data[i].a==128;}
    UnloadImage(im);
   }
  }
 }
 void Prepare(const nz::TrafficWorld &world,const nz::Vehicle &player,ShaderProgram &shader,const Environment &env){
  visible.clear();for(auto it=actors.begin();it!=actors.end();){bool live=std::any_of(world.cars.begin(),world.cars.end(),[&](const nz::TrafficCar &a){return a.id==it->first;});if(!live){UnloadTexture(it->second.albedo);UnloadRenderTexture(it->second.shaded);it=actors.erase(it);}else ++it;}
  for(const auto &car:world.cars){
   float station=(float)(car.station-player.distance);if(station>=nz::VisibleAhead||station<-18)continue;
   auto anchor=Project(env.anchor,station,(float)car.lateral);float scale=PixelsPerMetre/26.f,size=256*scale;Vector2 origin{std::round(anchor.x-128*scale),std::round(anchor.y-170*scale)};
   if(origin.x+size<0||origin.y+size<0||origin.x>Width||origin.y>Height)continue;
   auto &a=actors[car.id];if(a.id==0){a.id=car.id;Image blank=GenImageColor(256,256,BLANK);a.albedo=LoadTextureFromImage(blank);UnloadImage(blank);SetTextureFilter(a.albedo,TEXTURE_FILTER_POINT);a.shaded=LoadRenderTexture(256,256);SetTextureFilter(a.shaded.texture,TEXTURE_FILTER_POINT);}
   int pose=car.lateralSpeed>.15?2:car.lateralSpeed<-.15?0:1,frame=car.speed>1?(int)(car.distance/(2*nz::Pi*.31)*4)%4:0;const auto &maps=art[car.type][pose][frame];
   if(a.lastType!=car.type||a.lastPose!=pose||a.lastFrame!=frame||a.lastPaint!=car.paint){auto pixels=maps.albedo;const auto &finish=nz::GetTrafficPaint(car.type,car.paint);Color paint{finish.r,finish.g,finish.b,255};for(size_t i=0;i<pixels.size();i++)if(maps.paint[i]){pixels[i].r=(unsigned char)(pixels[i].r*paint.r/255);pixels[i].g=(unsigned char)(pixels[i].g*paint.g/255);pixels[i].b=(unsigned char)(pixels[i].b*paint.b/255);}UpdateTexture(a.albedo,pixels.data());a.lastType=car.type;a.lastPose=pose;a.lastFrame=frame;a.lastPaint=car.paint;}
   a.origin=origin;a.station=station;a.depth=WorldToCamera(station,(float)car.lateral,0).z;a.type=car.type;a.pose=pose;a.frame=frame;a.brake=car.braking;a.indicator=car.indicator;
   Environment lighting=env;lighting.heads=lighting.tails=true;lighting.brake=car.braking;bool flash=std::fmod(world.clock,1.15)<.53;lighting.indicatorLeft=car.indicator<0&&flash?1.f:0.f;lighting.indicatorRight=car.indicator>0&&flash?1.f:0.f;
   shader.Lights(lighting);shader.Vec2("carOrigin",origin);shader.Float("carDepth",a.depth);shader.Float("sourcePixels",256);shader.Float("depthRange",12);shader.Float("spritePixels",size);
   BeginTextureMode(a.shaded);ClearBackground(BLANK);BeginShaderMode(shader.value);shader.Texture("normalMap",maps.textures[1]);shader.Texture("depthMap",maps.textures[2]);shader.Texture("auxiliaryMap",maps.textures[3]);shader.Texture("emissionMap",maps.textures[4]);DrawTexture(a.albedo,0,0,WHITE);EndShaderMode();EndTextureMode();visible.push_back(car.id);
  }
 }
 void RoadLights(ShaderProgram &shader,const nz::TrafficWorld &world,const nz::Vehicle &player){
  std::array<float,48> beams{};int count=0;
  for(const auto &a:world.cars){float s=(float)(a.station-player.distance);if(s< -44||s>nz::VisibleAhead||count==12)continue;beams[count*4]=s;beams[count*4+1]=(float)a.lateral;beams[count*4+2]=(float)std::atan2(a.lateralSpeed,std::max(a.speed,4.));beams[count*4+3]=(float)nz::TrafficTypes[a.type].length*.5f;++count;}
  shader.Int("trafficCount",count);int loc=shader.At("trafficBeam[0]");if(loc>=0&&count)SetShaderValueV(shader.value,loc,beams.data(),SHADER_UNIFORM_VEC4,count);
 }
 void Shadows(const Environment &env,const nz::TrafficWorld &world,const nz::Vehicle &player){
  (void)world;(void)player;
  shadow.Lights(env);shadow.Vec2("resolution",{(float)Width,(float)Height});
  for(int id:visible){const auto &a=actors.at(id);const auto &maps=art[a.type][a.pose][a.frame];
   float size=256*PixelsPerMetre/26.f,pad=PixelsPerMetre*9;
   shadow.Vec2("carOrigin",a.origin);shadow.Float("spritePixels",size);shadow.Float("carDepth",a.depth);shadow.Float("station",a.station);
   BeginShaderMode(shadow.value);shadow.Texture("carAlbedo",maps.textures[0]);shadow.Texture("carFront",maps.textures[2]);shadow.Texture("carBack",maps.textures[3]);
   DrawRectangleRec({a.origin.x-pad,a.origin.y-pad,size+2*pad,size+2*pad},WHITE);EndShaderMode();
  }
 }
 void Draw(int mode){
  if(visible.empty())return;
  rlDrawRenderBatchActive();rlEnableDepthTest();
  for(int id:visible){const auto &a=actors.at(id);const auto &maps=art[a.type][a.pose][a.frame];float size=256*PixelsPerMetre/26.f;composite.Int("layerMode",mode);composite.Float("nativeSize",256);composite.Float("spritePixels",size);composite.Float("baseDepth",a.depth);composite.Float("station",a.station);composite.Float("visibleAhead",nz::VisibleAhead);
   BeginShaderMode(composite.value);composite.Texture("frontMap",maps.textures[2]);DrawTexturePro(a.shaded.texture,{0,0,256,-256},{a.origin.x,a.origin.y,size,size},{0,0},0,WHITE);EndShaderMode();++totalDrawn;
  }rlDrawRenderBatchActive();rlDisableDepthTest();
 }
 void Glow(ShaderProgram &glow,const nz::TrafficWorld &world,const Environment &env){
  rlSetBlendFactorsSeparate(RL_ONE,RL_ONE,RL_ZERO,RL_ONE,RL_FUNC_ADD,RL_FUNC_ADD);BeginBlendMode(BLEND_CUSTOM_SEPARATE);for(int id:visible){const auto &a=actors.at(id);const auto &maps=art[a.type][a.pose][a.frame];Environment e=env;e.heads=e.tails=true;e.brake=a.brake;bool flash=std::fmod(world.clock,1.15)<.53;e.indicatorLeft=a.indicator<0&&flash?1.f:0;e.indicatorRight=a.indicator>0&&flash?1.f:0;glow.Lights(e);float fade=std::clamp((nz::VisibleAhead-a.station)/8.f,0.f,1.f);glow.Float("headlights",fade);glow.Float("taillights",fade);glow.Float("braking",a.brake?fade:0);glow.Float("indicatorLeft",e.indicatorLeft*fade);glow.Float("indicatorRight",e.indicatorRight*fade);float size=256*PixelsPerMetre/26.f;glow.Float("spritePixels",size);BeginShaderMode(glow.value);DrawTexturePro(maps.textures[4],{0,0,256,256},{a.origin.x-6,a.origin.y-6,size+12,size+12},{0,0},0,WHITE);EndShaderMode();}EndBlendMode();
 }
 void Warnings(const nz::TrafficWorld &world,const nz::Vehicle &player,const Environment &env){
  std::array<const nz::TrafficCar*,4> nearest{};std::array<double,4> strength{};
  for(const auto &car:world.cars){double alpha=nz::TrafficEntryWarning(car,{player.distance,player.lateral,player.speed},nz::VisibleAhead);if(alpha<=0)continue;
   int lane=std::clamp((int)std::lround((car.lateral+5.25)/3.5),0,3);
   if(!nearest[lane]||car.station<nearest[lane]->station){nearest[lane]=&car;strength[lane]=alpha;}
  }
  for(int lane=0;lane<4;lane++)if(nearest[lane]){
   Vector2 p=Project(env.anchor,nz::VisibleAhead-1.5f,(float)nearest[lane]->lateral);
   p.x=std::round(std::clamp(p.x,10.f,Width-10.f));p.y=std::round(std::clamp(p.y,14.f,Height-10.f));
   float pulse=.84f+.16f*std::sin((float)world.clock*8.f);int alpha=(int)(255*strength[lane]);
   DrawTriangle({p.x,p.y-12},{p.x-8,p.y+3},{p.x+8,p.y+3},RGBA(17,22,28,alpha));
   DrawTriangle({p.x,p.y-10},{p.x-6,p.y+2},{p.x+6,p.y+2},RGBA((int)(255*pulse),(int)(199*pulse),87,alpha));
   DrawRectangle((int)p.x-1,(int)p.y-6,2,4,RGBA(26,28,30,alpha));DrawPixel((int)p.x,(int)p.y,RGBA(26,28,30,alpha));++totalWarnings;
  }
 }
 void ProbeDepth(){
  auto target=LoadRenderTexture(16,16);Image im=GenImageColor(16,16,RGBA(128,0,0));auto depth=LoadTextureFromImage(im);UnloadImage(im);
  im=GenImageColor(16,16,WHITE);auto colour=LoadTextureFromImage(im);UnloadImage(im);
  composite.Float("nativeSize",16);composite.Float("spritePixels",16);composite.Float("station",0);composite.Float("visibleAhead",nz::VisibleAhead);
  auto draw=[&](float w,Color tint,int mode){composite.Int("layerMode",mode);composite.Float("baseDepth",w);BeginShaderMode(composite.value);composite.Texture("frontMap",depth);DrawTexture(colour,0,0,tint);EndShaderMode();};
  for(bool reverse:{false,true}){
   BeginTextureMode(target);ClearBackground(BLANK);rlEnableDepthTest();if(reverse){draw(2,RGBA(0,240,0),0);draw(-2,RED,0);}else{draw(-2,RED,0);draw(2,RGBA(0,240,0),0);}rlDrawRenderBatchActive();rlDisableDepthTest();EndTextureMode();
   Image result=LoadImageFromTexture(target.texture);Color pixel=GetImageColor(result,8,8);UnloadImage(result);if(pixel.g<230||pixel.r>5)throw std::runtime_error("Traffic depth order depends on draw order");
  }
  BeginTextureMode(target);ClearBackground(BLANK);rlEnableDepthTest();draw(2,WHITE,2);rlDrawRenderBatchActive();rlDisableDepthTest();EndTextureMode();Image result=LoadImageFromTexture(target.texture);Color pixel=GetImageColor(result,8,8);UnloadImage(result);
  float decoded=((pixel.r*256.f+pixel.g)/65535.f-.5f)*256;if(std::abs(decoded-2)>.006f)throw std::runtime_error("Traffic global depth encoding differs from the player compositor");
  UnloadTexture(depth);UnloadTexture(colour);UnloadRenderTexture(target);std::puts("TRAFFIC DEPTH PROBE: order-independent colour and encoded global depth pass");
 }
 void Close(){for(auto &row:art)for(auto &poses:row)for(auto &frame:poses)for(auto tex:frame.textures)UnloadTexture(tex);for(auto &entry:actors){UnloadTexture(entry.second.albedo);UnloadRenderTexture(entry.second.shaded);}UnloadShader(composite.value);UnloadShader(shadow.value);}
};
