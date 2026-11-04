#pragma once
#include <fstream>
#include <sstream>
struct DecalArt {std::string id,name;bool mono;int width,height;std::vector<Color> pixels;Texture2D preview;};
struct DecalLibrary {
 std::vector<DecalArt> entries;
 explicit DecalLibrary(const fs::path &assets){
  std::ifstream input(assets/"decals/catalog.txt");std::string line;
  while(std::getline(input,line)){
   if(!line.empty()&&line.back()=='\r')line.pop_back();
   if(line.empty())continue;
   std::istringstream row(line);std::string id,name,mono,file;
   if(!std::getline(row,id,'|')||!std::getline(row,name,'|')||!std::getline(row,mono,'|')||!std::getline(row,file))throw std::runtime_error("Invalid decal catalog");
   Image im=LoadImage((assets/"decals"/file).string().c_str());if(!IsImageValid(im))throw std::runtime_error("Missing decal: "+file);ImageFormat(&im,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
   DecalArt art{id,name,mono=="1",im.width,im.height,{},LoadTextureFromImage(im)};art.pixels.resize(im.width*im.height);std::memcpy(art.pixels.data(),im.data,art.pixels.size()*sizeof(Color));UnloadImage(im);SetTextureFilter(art.preview,TEXTURE_FILTER_BILINEAR);entries.push_back(std::move(art));
  }
  if(entries.empty())throw std::runtime_error("Empty decal catalog");
 }
 int Find(const std::string &id)const{for(int i=0;i<(int)entries.size();i++)if(entries[i].id==id)return i;return -1;}
 // Bilinear premultiplied sampling avoids dark/coloured fringes on transparent ink.
 std::array<float,4> Sample(const DecalArt &art,float u,float v,unsigned tint)const{
  if(u<0||u>1||v<0||v>1)return {0,0,0,0};
  float x=u*art.width-.5f,y=v*art.height-.5f;int ix=(int)std::floor(x),iy=(int)std::floor(y);float fx=x-ix,fy=y-iy;std::array<float,4> out{};
  for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++){
   Color p=art.pixels[std::clamp(iy+dy,0,art.height-1)*art.width+std::clamp(ix+dx,0,art.width-1)];float w=(dx?fx:1-fx)*(dy?fy:1-fy)*p.a/255.f;
   out[0]+=(art.mono?(tint>>24)&255:p.r)*w;out[1]+=(art.mono?(tint>>16)&255:p.g)*w;out[2]+=(art.mono?(tint>>8)&255:p.b)*w;out[3]+=w;
  }return out;
 }
 void Close(){for(auto &d:entries)UnloadTexture(d.preview);}
};
