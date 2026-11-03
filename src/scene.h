#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
namespace nz {
struct ViewportLayout {int width,height,scale;};
inline ViewportLayout MakeViewport(int windowWidth,int windowHeight){
 int scale=std::max(1,std::min(windowWidth/640,windowHeight/360));
 return {(windowWidth+scale-1)/scale,(windowHeight+scale-1)/scale,scale};
}
constexpr float CameraK=.70710678118f,CameraSin=.5f,CameraCos=.86602540378f;
constexpr float VisibleAhead=28.f,VisibleBehind=8.f,VisibleHalfWidth=8.5f;
struct DrivingView {float pixelsPerMetre,x,y;};
inline DrivingView FitDrivingView(int width,int height){
 float span=VisibleAhead+VisibleBehind+2*VisibleHalfWidth;
 float ppm=std::min((width-32)/(span*CameraK),(height-32)/(span*CameraK*CameraSin));
 float usedWidth=span*CameraK*ppm,usedHeight=span*CameraK*CameraSin*ppm;
 return {ppm,(width-usedWidth)*.5f+(VisibleBehind+VisibleHalfWidth)*CameraK*ppm,
  (height-usedHeight)*.5f+(VisibleAhead+VisibleHalfWidth)*CameraK*CameraSin*ppm};
}
inline float SpriteYaw(double yaw){return std::abs(yaw)>.027?(yaw>0?.1221730476f:-.1221730476f):0.f;}
inline std::pair<float,float> HeadlampPosition(float yaw,int side){
 return {2.f*std::cos(yaw)-side*.56f*std::sin(yaw),2.f*std::sin(yaw)+side*.56f*std::cos(yaw)};
}
inline float HeadlampBeam(float s,float l,float yaw){
 float ahead=s*std::cos(yaw)+l*std::sin(yaw)-2;
 if(ahead<=0||ahead>=30)return 0;
 float lateral=-s*std::sin(yaw)+l*std::cos(yaw),width=.19f+ahead*.065f;
 float a=(lateral-.56f)/width,b=(lateral+.56f)/width;
 return (std::exp(-a*a*2)+std::exp(-b*b*2))*std::pow(1-ahead/30,1.6f);
}
inline std::array<float,3> GarageKeyLight(float clock){
 float angle=clock*.55f,radius=3.8f,height=3.2f;
 return {radius*std::cos(angle),height*CameraCos+radius*std::sin(angle)*CameraSin,height*CameraSin-radius*std::sin(angle)*CameraCos};
}
inline std::array<float,3> NortheastCamera(float s,float l,float z){
 return {(s+l)*CameraK,z*CameraCos+(s-l)*CameraK*CameraSin,(-s+l)*CameraK*CameraCos+z*CameraSin};
}
inline std::pair<float,float> NortheastGround(float x,float y){
 return {(x/CameraK+y/(CameraK*CameraSin))*.5f,(x/CameraK-y/(CameraK*CameraSin))*.5f};
}
inline std::pair<int,int> StreetRange(double distance,float minS,float maxS,int side){
 double phase=side>0?12.:0.;
 return {(int)std::ceil((distance+minS-25.3-phase)/24.),(int)std::floor((distance+maxS+25.3-phase)/24.)};
}
inline double StreetFalloff(double distance){
 double core=std::max(0.,1-distance*distance/(11.5*11.5));
 double tail=std::max(0.,1-distance*distance/(25.3*25.3));
 return core*core+.14*tail*tail;
}
}
