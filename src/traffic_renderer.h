#pragma once
struct TrafficRenderer {
 struct Frame {std::array<Texture2D,5> textures{};std::vector<Color> albedo;std::vector<unsigned char> paint;std::vector<Color> geometry;Texture2D volume{};Vector4 bounds{0,0,1,1};};
 struct Actor {int id=0,lastType=-1,lastPose=-1,lastFrame=-1,lastPaint=-1;Texture2D albedo{};RenderTexture2D shaded{};Vector2 origin{};float depth=0,station=0;int type=0,pose=0,frame=0;bool brake=false;int indicator=0;};
 std::array<std::array<std::array<Frame,4>,3>,nz::TrafficTypes.size()> art;
 std::map<int,Actor> actors;std::vector<int> visible;int totalDrawn=0,totalWarnings=0;
 struct Rival {CarRenderer *car=nullptr;Vector2 origin{};float depth=0,station=0,lateral=0,yaw=0;bool active=false,present=false;Environment light{};} rival;
 Texture2D rivalVolume{};std::vector<Color> rivalGeometry;int rivalDrawn=0;
 ShaderProgram composite;RenderTexture2D shadowAtlas{};
 explicit TrafficRenderer(const fs::path &assets):composite(assets/"shaders/traffic_composite.fs"){
  shadowAtlas=LoadRenderTexture(1024,2048);SetTextureFilter(shadowAtlas.texture,TEXTURE_FILTER_POINT);SetTextureWrap(shadowAtlas.texture,TEXTURE_WRAP_CLAMP);
  Image blank=GenImageColor(128,128,BLANK);rivalVolume=LoadTextureFromImage(blank);UnloadImage(blank);SetTextureFilter(rivalVolume,TEXTURE_FILTER_POINT);rivalGeometry.resize(128*128);
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
    if(k==2){a.geometry.resize(256*256);auto pixels=(Color*)im.data;for(int i=0;i<256*256;i++)a.geometry[i]={pixels[i].r,pixels[i].g,0,0};}
    if(k==3){auto pixels=(Color*)im.data;for(int i=0;i<256*256;i++){if(pixels[i].a<128)a.geometry[i]={0,0,0,0};else{a.geometry[i].b=pixels[i].g;a.geometry[i].a=pixels[i].b;}}int x0=256,y0=256,x1=0,y1=0;for(int i=0;i<256*256;i++)if(a.geometry[i].r||a.geometry[i].g){x0=std::min(x0,i%256);y0=std::min(y0,i/256);x1=std::max(x1,i%256+1);y1=std::max(y1,i/256+1);}a.bounds={(float)x0/256,(float)y0/256,(float)x1/256,(float)y1/256};Image volume{a.geometry.data(),256,256,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};a.volume=LoadTextureFromImage(volume);SetTextureFilter(a.volume,TEXTURE_FILTER_POINT);}
    UnloadImage(im);
   }
  }
 }
 void Prepare(const nz::TrafficWorld &world,const nz::Vehicle &player,ShaderProgram &shader,const Environment &env){
  visible.clear();for(auto it=actors.begin();it!=actors.end();){bool live=std::any_of(world.cars.begin(),world.cars.end(),[&](const nz::TrafficCar &a){return a.id==it->first;});if(!live){UnloadTexture(it->second.albedo);UnloadRenderTexture(it->second.shaded);it=actors.erase(it);}else ++it;}
  for(const auto &car:world.cars){
   float station=(float)(car.station-player.distance);if(station>=nz::VisibleAhead+35.f||station<-28)continue;
   auto anchor=Project(env.anchor,station,(float)car.lateral);float scale=PixelsPerMetre/26.f,size=256*scale;Vector2 origin{std::round(anchor.x-128*scale),std::round(anchor.y-170*scale)};
   if(origin.x+size< -PixelsPerMetre*9||origin.y+size< -PixelsPerMetre*9||origin.x>Width+PixelsPerMetre*9||origin.y>Height+PixelsPerMetre*9)continue;
   auto &a=actors[car.id];if(a.id==0){a.id=car.id;Image blank=GenImageColor(256,256,BLANK);a.albedo=LoadTextureFromImage(blank);UnloadImage(blank);SetTextureFilter(a.albedo,TEXTURE_FILTER_POINT);a.shaded=LoadRenderTexture(256,256);SetTextureFilter(a.shaded.texture,TEXTURE_FILTER_POINT);}
   int pose=car.lateralSpeed>.15?2:car.lateralSpeed<-.15?0:1,frame=car.speed>1?(int)(car.distance/(2*nz::Pi*.31)*4)%4:0;const auto &maps=art[car.type][pose][frame];
   if(a.lastType!=car.type||a.lastPose!=pose||a.lastFrame!=frame||a.lastPaint!=car.paint){auto pixels=maps.albedo;const auto &finish=nz::GetTrafficPaint(car.type,car.paint);Color paint{finish.r,finish.g,finish.b,255};for(size_t i=0;i<pixels.size();i++)if(maps.paint[i]){pixels[i].r=(unsigned char)(pixels[i].r*paint.r/255);pixels[i].g=(unsigned char)(pixels[i].g*paint.g/255);pixels[i].b=(unsigned char)(pixels[i].b*paint.b/255);}UpdateTexture(a.albedo,pixels.data());a.lastType=car.type;a.lastPose=pose;a.lastFrame=frame;a.lastPaint=car.paint;}
   a.origin=origin;a.station=station;a.depth=WorldToCamera(station,(float)car.lateral,0).z;a.type=car.type;a.pose=pose;a.frame=frame;a.brake=car.braking;a.indicator=car.indicator;
   Environment lighting=env;lighting.heads=lighting.tails=true;lighting.brake=car.braking;bool flash=std::fmod(world.clock,1.15)<.53;lighting.indicatorLeft=car.indicator<0&&flash?1.f:0.f;lighting.indicatorRight=car.indicator>0&&flash?1.f:0.f;
   // Match the player's small local head/tail point lights, with anchors scaled
   // to this car rather than inheriting only the player's nearby lamp spill.
   const auto &spec=nz::TrafficTypes[car.type];float yaw=(pose-1)*3.f*(float)nz::Pi/180.f;
   auto spill=[&](float forward,float side,float height,Vector3 colour,float power,float radius){
    if(lighting.lights.size()>=32)return;
    auto point=WorldToCamera(station+forward*std::cos(yaw)-side*std::sin(yaw),(float)car.lateral+forward*std::sin(yaw)+side*std::cos(yaw),height);
    lighting.lights.push_back({point,colour,power,radius});
   };
   for(int side:{-1,1})spill((float)spec.length*.45f,side*(float)spec.width*.33f,.5f,{1,.88f,.65f},.42f,3.2f);
   spill(-(float)spec.length*.45f,0,.45f,{1,.08f,.04f},car.braking?1.35f:.45f,3.5f);
   shader.Lights(lighting);shader.Vec2("carOrigin",origin);shader.Float("carDepth",a.depth);shader.Float("sourcePixels",256);shader.Float("depthRange",12);shader.Float("spritePixels",size);
   BeginTextureMode(a.shaded);ClearBackground(BLANK);BeginShaderMode(shader.value);shader.Texture("normalMap",maps.textures[1]);shader.Texture("depthMap",maps.textures[2]);shader.Texture("auxiliaryMap",maps.textures[3]);shader.Texture("emissionMap",maps.textures[4]);DrawTexture(a.albedo,0,0,WHITE);EndShaderMode();EndTextureMode();visible.push_back(car.id);
  }
  BeginTextureMode(shadowAtlas);ClearBackground(BLANK);rlSetBlendFactors(RL_ONE,RL_ZERO,RL_FUNC_ADD);BeginBlendMode(BLEND_CUSTOM);
  for(size_t i=0;i<visible.size();i++){const auto &a=actors.at(visible[i]);DrawTexture(art[a.type][a.pose][a.frame].volume,(int)(i%4)*256,(int)(i/4)*256,WHITE);}
  EndBlendMode();EndTextureMode();
 }
 void Opponent(CarRenderer &car,const Environment &light,Vector2 origin,float depth,float station,float lateral,float yaw,bool enabled){
  rival={&car,origin,depth,station,lateral,yaw,false,enabled,light};
  if(!enabled||station>nz::VisibleAhead+35||station< -28)return;
  float size=128*PixelsPerMetre/26.f,pad=PixelsPerMetre*9;
  if(origin.x+size< -pad||origin.y+size< -pad||origin.x>Width+pad||origin.y>Height+pad)return;
  rival.active=true;
  for(int i=0;i<128*128;i++){
   if(car.output[0][i].a<128){rivalGeometry[i]=BLANK;continue;}
   auto front=car.output[2][i],back=car.output[3][i];
   auto convert=[](int value){return (unsigned)std::lround(std::clamp(.5+((value/65535.)-.5)*5.2/12.,0.,1.)*65535);};
   unsigned f=convert(front.r*256+front.g),b=convert(back.g*256+back.b);rivalGeometry[i]=RGBA(f/256,f%256,b/256,b%256);
  }
  UpdateTexture(rivalVolume,rivalGeometry.data());int cell=(int)visible.size();
  BeginTextureMode(shadowAtlas);rlSetBlendFactors(RL_ONE,RL_ZERO,RL_FUNC_ADD);BeginBlendMode(BLEND_CUSTOM);DrawTexture(rivalVolume,(cell%4)*256+64,(cell/4)*256+91,WHITE);EndBlendMode();EndTextureMode();
 }
 void RoadLights(ShaderProgram &shader,const nz::TrafficWorld &world,const nz::Vehicle &player,bool enabled){
  std::array<float,48> beams{};int count=0;
  if(enabled&&rival.present&&rival.station>=-44&&rival.station<nz::VisibleAhead){beams[0]=rival.station;beams[1]=rival.lateral;beams[2]=rival.yaw;beams[3]=2.24f;count=1;}
  for(const auto &a:world.cars){float s=(float)(a.station-player.distance);if(s< -44||s>nz::VisibleAhead||count==12)continue;beams[count*4]=s;beams[count*4+1]=(float)a.lateral;beams[count*4+2]=(float)std::atan2(a.lateralSpeed,std::max(a.speed,4.));beams[count*4+3]=(float)nz::TrafficTypes[a.type].length*.5f;++count;}
  std::array<float,128> volumes{},bounds{};int actorsCount=enabled?(int)visible.size():0;
  for(int i=0;i<actorsCount;i++){const auto &a=actors.at(visible[i]);volumes[i*4]=a.origin.x;volumes[i*4+1]=a.origin.y;volumes[i*4+2]=a.depth;volumes[i*4+3]=(float)i;const auto &box=art[a.type][a.pose][a.frame].bounds;bounds[i*4]=box.x;bounds[i*4+1]=box.y;bounds[i*4+2]=box.z;bounds[i*4+3]=box.w;}
  if(enabled&&rival.active){
   int i=actorsCount++;float scale=PixelsPerMetre/26.f;volumes[i*4]=rival.origin.x-64*scale;volumes[i*4+1]=rival.origin.y-91*scale;volumes[i*4+2]=rival.depth;volumes[i*4+3]=(float)i;
   auto box=rival.car->shadowBounds;bounds[i*4]=(box.x*128+64)/256;bounds[i*4+1]=(box.y*128+91)/256;bounds[i*4+2]=(box.z*128+64)/256;bounds[i*4+3]=(box.w*128+91)/256;
  }
  shader.Int("trafficShadowCount",actorsCount);int volumeLoc=shader.At("trafficOccluder[0]");if(volumeLoc>=0&&actorsCount)SetShaderValueV(shader.value,volumeLoc,volumes.data(),SHADER_UNIFORM_VEC4,actorsCount);
  int boundsLoc=shader.At("trafficBounds[0]");if(boundsLoc>=0&&actorsCount)SetShaderValueV(shader.value,boundsLoc,bounds.data(),SHADER_UNIFORM_VEC4,actorsCount);
  shader.Int("trafficCount",count);int loc=shader.At("trafficBeam[0]");if(loc>=0&&count)SetShaderValueV(shader.value,loc,beams.data(),SHADER_UNIFORM_VEC4,count);
 }
 void Draw(int mode){
  if(visible.empty()&&!rival.active)return;
  rlDrawRenderBatchActive();rlEnableDepthTest();
  for(int id:visible){const auto &a=actors.at(id);const auto &maps=art[a.type][a.pose][a.frame];float size=256*PixelsPerMetre/26.f;composite.Int("layerMode",mode);composite.Float("geometryRange",12);composite.Float("nativeSize",256);composite.Float("spritePixels",size);composite.Float("baseDepth",a.depth);composite.Float("station",a.station);composite.Float("visibleAhead",nz::VisibleAhead);
   BeginShaderMode(composite.value);composite.Texture("frontMap",maps.textures[2]);DrawTexturePro(a.shaded.texture,{0,0,256,-256},{a.origin.x,a.origin.y,size,size},{0,0},0,WHITE);EndShaderMode();++totalDrawn;
  }
  if(rival.active){float size=128*PixelsPerMetre/26.f;composite.Int("layerMode",mode);composite.Float("nativeSize",128);composite.Float("geometryRange",5.2f);composite.Float("spritePixels",size);composite.Float("baseDepth",rival.depth);composite.Float("station",rival.station);
   BeginShaderMode(composite.value);composite.Texture("frontMap",rival.car->textures[2]);DrawTexturePro(rival.car->shaded.texture,{0,0,128,-128},{rival.origin.x,rival.origin.y,size,size},{0,0},0,WHITE);EndShaderMode();++rivalDrawn;
  }rlDrawRenderBatchActive();rlDisableDepthTest();
 }
 void Glow(ShaderProgram &glow,const nz::TrafficWorld &world,const Environment &env){
  rlSetBlendFactorsSeparate(RL_ONE,RL_ONE,RL_ZERO,RL_ONE,RL_FUNC_ADD,RL_FUNC_ADD);BeginBlendMode(BLEND_CUSTOM_SEPARATE);for(int id:visible){const auto &a=actors.at(id);const auto &maps=art[a.type][a.pose][a.frame];Environment e=env;e.heads=e.tails=true;e.brake=a.brake;bool flash=std::fmod(world.clock,1.15)<.53;e.indicatorLeft=a.indicator<0&&flash?1.f:0;e.indicatorRight=a.indicator>0&&flash?1.f:0;glow.Lights(e);float size=256*PixelsPerMetre/26.f;glow.Float("spritePixels",size);BeginShaderMode(glow.value);DrawTexturePro(maps.textures[4],{0,0,256,256},{a.origin.x-6,a.origin.y-6,size+12,size+12},{0,0},0,WHITE);EndShaderMode();}
  if(rival.active){glow.Lights(rival.light);float size=128*PixelsPerMetre/26.f;glow.Float("spritePixels",size);BeginShaderMode(glow.value);DrawTexturePro(rival.car->textures[4],{0,0,128,128},{rival.origin.x-6,rival.origin.y-6,size+12,size+12},{0,0},0,WHITE);EndShaderMode();}
  EndBlendMode();
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
 void ProbeLighting(ShaderProgram &road){
  constexpr int n=64;float scale=PixelsPerMetre/26.f;Vector2 anchor{32,48};
  auto target=LoadRenderTexture(n,n);Image alpha=GenImageColor(128,128,BLANK),front=GenImageColor(128,128,BLANK),back=GenImageColor(128,128,BLANK),volume=GenImageColor(256,256,BLANK);
  auto encoded=[](float value){unsigned v=(unsigned)std::lround(value*65535);return std::pair<int,int>{v/256,v%256};};
  auto pf=encoded(.5f+1.f/5.2f),pb=encoded(.5f-1.f/5.2f),tf=encoded(.5f+1.f/12.f),tb=encoded(.5f-1.f/12.f);
  for(int y=55;y<79;y++)for(int x=48;x<80;x++){
   ImageDrawPixel(&alpha,x,y,WHITE);ImageDrawPixel(&front,x,y,RGBA(pf.first,pf.second,0));ImageDrawPixel(&back,x,y,RGBA(0,pb.first,pb.second));
   ImageDrawPixel(&volume,x+64,y+91,RGBA(tf.first,tf.second,tb.first,tb.second));
  }
  auto a=LoadTextureFromImage(alpha),f=LoadTextureFromImage(front),b=LoadTextureFromImage(back),v=LoadTextureFromImage(volume);UnloadImage(alpha);UnloadImage(front);UnloadImage(back);UnloadImage(volume);
  BeginTextureMode(shadowAtlas);ClearBackground(BLANK);rlSetBlendFactors(RL_ONE,RL_ZERO,RL_FUNC_ADD);BeginBlendMode(BLEND_CUSTOM);DrawTexture(v,0,0,WHITE);EndBlendMode();EndTextureMode();
  Environment env{};env.anchor=anchor;env.lights={{WorldToCamera(-3,-3,5),{1,.85f,.66f},.7f,18},{WorldToCamera(4,4,5),{.65f,.8f,1},.3f,18}};env.streetCount=2;
  road.Lights(env);road.Vec2("resolution",{n,n});road.Float("roadRepeat",15);road.Float("carDepth",0);road.Int("shadowProbe",1);road.Vec4("carBounds",{48.f/128,55.f/128,80.f/128,79.f/128});
  Vector4 box{112.f/256,146.f/256,144.f/256,170.f/256};int boxLoc=road.At("trafficBounds[0]");SetShaderValueV(road.value,boxLoc,&box,SHADER_UNIFORM_VEC4,1);
  std::array<float,4> actor{{anchor.x-128*scale,anchor.y-170*scale,0,0}};int loc=road.At("trafficOccluder[0]");SetShaderValueV(road.value,loc,actor.data(),SHADER_UNIFORM_VEC4,1);
  auto render=[&](int mode){
   road.Vec2("carOrigin",mode==1?Vector2{anchor.x-64*scale,anchor.y-79*scale}:Vector2{-9999,-9999});road.Int("trafficShadowCount",mode==2?1:0);
   BeginTextureMode(target);ClearBackground(BLANK);BeginShaderMode(road.value);road.TextureSlot("carAlbedo",a,5);road.TextureSlot("carFront",f,6);road.TextureSlot("carBack",b,7);road.TextureSlot("trafficGeometry",shadowAtlas.texture,8);DrawRectangle(0,0,n,n,WHITE);EndShaderMode();for(int slot=5;slot<=8;slot++)ShaderProgram::ClearSlot(slot);EndTextureMode();
   Image im=LoadImageFromTexture(target.texture);ImageFormat(&im,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);std::vector<Color> result(n*n);std::memcpy(result.data(),im.data,result.size()*sizeof(Color));UnloadImage(im);return result;
  };
  auto clear=render(0),player=render(1),traffic=render(2);int shadowed=0,different=0;
  for(int i=0;i<n*n;i++){shadowed+=player[i].r+player[i].g+player[i].b+3<clear[i].r+clear[i].g+clear[i].b;different+=std::abs((int)player[i].r-traffic[i].r)+std::abs((int)player[i].g-traffic[i].g)+std::abs((int)player[i].b-traffic[i].b)>3;}
  if(shadowed<8||different>8)throw std::runtime_error("Traffic/player per-light shadow parity failed: "+std::to_string(shadowed)+" shadow pixels, "+std::to_string(different)+" mismatches");
  road.Int("shadowProbe",0);road.Int("trafficShadowCount",0);UnloadTexture(a);UnloadTexture(f);UnloadTexture(b);UnloadTexture(v);UnloadRenderTexture(target);
  std::printf("SHARED CAR LIGHTING PROBE: %d shadow pixels; %d player/traffic mismatches; same per-light attenuation and ambient\n",shadowed,different);
 }
 void ProbeDepth(){
  auto target=LoadRenderTexture(16,16);Image im=GenImageColor(16,16,RGBA(128,0,0));auto depth=LoadTextureFromImage(im);UnloadImage(im);
  im=GenImageColor(16,16,WHITE);auto colour=LoadTextureFromImage(im);UnloadImage(im);
  composite.Float("geometryRange",12);composite.Float("nativeSize",16);composite.Float("spritePixels",16);composite.Float("station",0);composite.Float("visibleAhead",nz::VisibleAhead);
  auto draw=[&](float w,Color tint,int mode){composite.Int("layerMode",mode);composite.Float("baseDepth",w);BeginShaderMode(composite.value);composite.Texture("frontMap",depth);DrawTexture(colour,0,0,tint);EndShaderMode();};
  for(bool reverse:{false,true}){
   BeginTextureMode(target);ClearBackground(BLANK);rlEnableDepthTest();if(reverse){draw(2,RGBA(0,240,0),0);draw(-2,RED,0);}else{draw(-2,RED,0);draw(2,RGBA(0,240,0),0);}rlDrawRenderBatchActive();rlDisableDepthTest();EndTextureMode();
   Image result=LoadImageFromTexture(target.texture);Color pixel=GetImageColor(result,8,8);UnloadImage(result);if(pixel.g<230||pixel.r>5)throw std::runtime_error("Traffic depth order depends on draw order");
  }
  BeginTextureMode(target);ClearBackground(BLANK);rlEnableDepthTest();draw(2,WHITE,2);rlDrawRenderBatchActive();rlDisableDepthTest();EndTextureMode();Image result=LoadImageFromTexture(target.texture);Color pixel=GetImageColor(result,8,8);UnloadImage(result);
  float decoded=((pixel.r*256.f+pixel.g)/65535.f-.5f)*256;if(std::abs(decoded-2)>.006f)throw std::runtime_error("Traffic global depth encoding differs from the player compositor");
  composite.Int("layerMode",0);composite.Float("station",nz::VisibleAhead+4.f);
  BeginTextureMode(target);ClearBackground(BLANK);draw(2,WHITE,0);EndTextureMode();result=LoadImageFromTexture(target.texture);pixel=GetImageColor(result,8,8);UnloadImage(result);if(pixel.a!=255)throw std::runtime_error("Traffic still fades its geometry alpha at the horizon");
  UnloadTexture(depth);UnloadTexture(colour);UnloadRenderTexture(target);std::puts("TRAFFIC DEPTH PROBE: order-independent colour and encoded global depth pass");
 }
 void Close(){for(auto &row:art)for(auto &poses:row)for(auto &frame:poses){for(auto tex:frame.textures)UnloadTexture(tex);UnloadTexture(frame.volume);}for(auto &entry:actors){UnloadTexture(entry.second.albedo);UnloadRenderTexture(entry.second.shaded);}UnloadShader(composite.value);UnloadRenderTexture(shadowAtlas);UnloadTexture(rivalVolume);}
};
