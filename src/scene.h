#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
namespace nz {
constexpr float CameraK=.70710678118f,CameraSin=.5f,CameraCos=.86602540378f;
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
