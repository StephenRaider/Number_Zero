#pragma once
#include <limits>
static void SaveDecalSettings(const fs::path &path,const Selection &selection,const DecalLibrary &lib){
 Selection s=selection;s.CapturePaint();
 std::ofstream out(path);out.precision(std::numeric_limits<float>::max_digits10);out<<"NZ_GARAGE_4\n"<<nz::Cars.size()<<'\n'<<s.car<<' '<<s.manual<<'\n';
 for(int car=0;car<(int)nz::Cars.size();car++){const auto &p=s.finishes[car];out<<car<<' '<<(unsigned)ColorToInt(p.paint)<<' '<<p.twoTone<<' '<<p.blackHood<<' '<<p.hue<<' '<<p.saturation<<' '<<p.value<<' '<<s.limiterDeleted[car]<<'\n';}
 for(int car=0;car<(int)nz::Cars.size();car++)for(int layer=0;layer<3;layer++){
  const auto &d=s.decals[car][layer];out<<car<<' '<<layer<<' '<<(d.design>=0?lib.entries[d.design].id:"none")<<' '<<d.forward<<' '<<d.height<<' '<<d.width<<' '<<d.angle<<' '<<d.tint<<'\n';
 }
}
static void LoadDecalSettings(const fs::path &path,Selection &s,const DecalLibrary &lib){
 std::ifstream in(path);std::string tag;if(!(in>>tag)||(tag!="NZ_DECALS_1"&&tag!="NZ_GARAGE_2"&&tag!="NZ_GARAGE_3"&&tag!="NZ_GARAGE_4"))return;
 Selection loaded=s;loaded.limiterDeleted.fill(false);
 if(tag=="NZ_GARAGE_2"||tag=="NZ_GARAGE_3"||tag=="NZ_GARAGE_4"){
  int savedCars=2;if((tag=="NZ_GARAGE_3"||tag=="NZ_GARAGE_4")&&(!(in>>savedCars)||savedCars<1||savedCars>(int)nz::Cars.size()))return;
  int selected;bool manual;if(!(in>>selected>>manual)||selected<0||selected>=(int)nz::Cars.size())return;loaded.car=selected;loaded.manual=manual;
  for(int row=0;row<savedCars;row++){
   int car;unsigned colour;bool twoTone,hood;float hue,saturation,value;
   if(!(in>>car>>colour>>twoTone>>hood>>hue>>saturation>>value)||car!=row||!std::isfinite(hue)||!std::isfinite(saturation)||!std::isfinite(value))return;
   if(tag=="NZ_GARAGE_4"&&!(in>>loaded.limiterDeleted[car]))return;
   loaded.finishes[car]={GetColor(colour),twoTone,hood,std::clamp(hue,0.f,360.f),std::clamp(saturation,0.f,1.f),std::clamp(value,0.f,1.f)};
  }
  loaded.RestorePaint();
 }
 int car,layer;std::string id;nz::DecalPlacement d;
 while(in>>car>>layer>>id>>d.forward>>d.height>>d.width>>d.angle>>d.tint){
  if(car<0||car>=(int)nz::Cars.size()||layer<0||layer>2||!std::isfinite(d.forward)||!std::isfinite(d.height)||!std::isfinite(d.width)||!std::isfinite(d.angle))continue;
  d.design=lib.Find(id);nz::ClampDecal(d);loaded.decals[car][layer]=d;
 }
 s=loaded;
}
static void StudioBackdrop(){
 Vector2 off{(Width-640)*.5f,(Height-360)*.5f};
 DrawRectangleGradientV(0,0,Width,Height,RGBA(49,55,64),RGBA(76,84,94));
 DrawEllipse((int)(186+off.x),(int)(239+off.y),127,32,RGBA(32,38,46,170));
 for(int x:{27,337}){DrawRectangle((int)(x-5+off.x),(int)(107+off.y),11,124,RGBA(205,224,238,12));DrawRectangle((int)(x-2+off.x),(int)(112+off.y),5,114,RGBA(214,230,241,48));DrawRectangle((int)(x+off.x),(int)(117+off.y),1,104,RGBA(236,242,248,220));}
 DrawRectangle((int)(98+off.x),(int)(91+off.y),176,1,RGBA(238,242,248,110));
}
static bool DecalEditor(Selection &s,const DecalLibrary &lib,int &view,int &layer,Vector2 mouse,float dt){
 DrawRectangle(0,0,640,57,RGBA(13,21,30,245));Label("DECAL STUDIO",24,15,22);Label(TextFormat("%s / RIGHT SIDE",nz::Cars[s.car].name),25,42,9,Mint);
 if(Button({25,67,94,20},"FRONT VIEW",mouse,view==0))view=0;
 if(Button({125,67,94,20},"REAR VIEW",mouse,view==1))view=1;
 Label("FIXED SOFTBOX / LIGHT BARS",25,275,9,Paper);Label("Drag on car, or use arrows to move",25,294,9,Paper);Label("One position follows every driving pose",25,310,8,Muted);
 for(int i=0;i<3;i++)if(Button({378.f+i*80,65,74,21},TextFormat("LAYER %d",i+1),mouse,layer==i))layer=i;
 auto &d=s.decals[s.car][layer];int count=(int)lib.entries.size();
 if(Button({378,96,27,20},"<",mouse))lib.Select(d,d.design<0?count-1:(d.design+count-1)%count);
 if(Button({591,96,27,20},">",mouse))lib.Select(d,(d.design+1)%count);
 if(d.design>=0){
  const auto &art=lib.entries[d.design];Label(TextFormat("%02d / %02d",d.design+1,count),485,89,8,Muted);
  std::string name=art.name;if(name.size()>24)name=name.substr(0,23)+".";Label(name.c_str(),410,102,9,Paper);
  DrawRectangle(378,124,240,47,RGBA(45,53,65));
  float factor=std::min(224.f/art.width,39.f/art.height);float w=art.width*factor,h=art.height*factor;
  DrawTexturePro(art.preview,{0,0,(float)art.width,(float)art.height},{498-w*.5f,147.5f-h*.5f,w,h},{0,0},0,art.mono?GetColor(d.tint):WHITE);
 }else{Label("NO DECAL / choose with < or >",392,141,10,Muted);}
 float wheel=GetMouseWheelMove();if(CheckCollisionPointRec(mouse,{378,95,240,78})&&wheel)lib.Select(d,(std::max(0,d.design)+(wheel>0?1:count-1))%count);
 Label("MOVE ON SIDE PANEL",378,180,9,Muted);
 if(Button({378,194,55,19},"LEFT",mouse))d.forward-=.06f;
 if(Button({439,194,55,19},"RIGHT",mouse))d.forward+=.06f;
 if(Button({501,194,55,19},"UP",mouse))d.height+=.035f;
 if(Button({563,194,55,19},"DOWN",mouse))d.height-=.035f;
 d.forward+=((IsKeyDown(KEY_RIGHT)?1.f:0)-(IsKeyDown(KEY_LEFT)?1.f:0))*.65f*dt;
 d.height+=((IsKeyDown(KEY_UP)?1.f:0)-(IsKeyDown(KEY_DOWN)?1.f:0))*.4f*dt;
 // Drag on the isometric vertical side plane. Screen-right means toward the
 // front in both quarter views; its vertical slope changes sign at the rear.
 static bool dragging=false;static Vector2 previous{};
 if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){dragging=CheckCollisionPointRec(mouse,{43,100,281,163});previous=mouse;}
 if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))dragging=false;
 if(dragging&&d.design>=0){Vector2 delta{mouse.x-previous.x,mouse.y-previous.y};float forward=delta.x/(52*K);d.forward+=forward;d.height+=(-delta.y/52+(view==0?1.f:-1.f)*K*.5f*forward)/ElevationCos;previous=mouse;}
 Label(TextFormat("SIZE %.2fm",d.width),378,225,9,Muted);
 if(Button({482,220,38,19},"-",mouse))d.width-=.15f;
 if(Button({526,220,38,19},"+",mouse))d.width+=.15f;
 Label(TextFormat("ANGLE %+.0f",d.angle),378,250,9,Muted);
 if(Button({482,245,38,19},"-",mouse))d.angle-=5;
 if(Button({526,245,38,19},"+",mouse))d.angle+=5;
 nz::ClampDecal(d);
 Label(d.design>=0&&!lib.entries[d.design].mono?"ORIGINAL COLOURS FIXED":"INK COLOUR",378,273,9,Muted);
 const Color colours[]={Paper,RGBA(20,24,31),RGBA(235,53,76),RGBA(255,178,38),RGBA(52,158,230),RGBA(55,210,159),RGBA(190,111,224),RGBA(222,148,188)};
 for(int i=0;i<8;i++){Rectangle r{378.f+i*30,287,24,14};DrawRectangleRec(r,colours[i]);if((unsigned)ColorToInt(colours[i])==d.tint)DrawRectangleLinesEx({r.x-1,r.y-1,r.width+2,r.height+2},1,Mint);if(CheckCollisionPointRec(mouse,r)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))d.tint=ColorToInt(colours[i]);}
 if(Button({378,313,73,19},"REMOVE",mouse))d.design=-1;
 if(Button({457,313,73,19},"RESET",mouse)){int id=d.design;d=nz::DecalPlacement{};lib.Select(d,id);}
 return Button({536,313,82,27},"DONE / SAVE",mouse,true)||Pressed(KEY_ESCAPE);
}
