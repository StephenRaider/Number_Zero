#pragma once
#include <algorithm>
#include <cmath>
#include <utility>
namespace nz {
struct DecalPlacement {
 int design=-1;float forward=0,height=.67f,width=2.85f,angle=0;
 unsigned tint=0xe8eaf0ff;
 bool operator==(const DecalPlacement &b)const{return design==b.design&&forward==b.forward&&height==b.height&&width==b.width&&angle==b.angle&&tint==b.tint;}
};
struct DecalSurface {float x,y,z,nx,ny,nz;};
inline void RotateXY(float &x,float &y,float a){float c=std::cos(a),s=std::sin(a),v=x*c-y*s;y=x*s+y*c;x=v;}
inline void RotateXZ(float &x,float &z,float a){float c=std::cos(a),s=std::sin(a),v=x*c+z*s;z=-x*s+z*c;x=v;}
inline void RotateYZ(float &y,float &z,float a){float c=std::cos(a),s=std::sin(a),v=y*c-z*s;z=y*s+z*c;y=v;}
// Inverse of render_modular.py's camera, yaw, roll and pitch matrices.
// Rear imagery is reflected across the vehicle centreline by the existing
// driving mirror; treat it as the right rear quarter of the symmetric body.
inline DecalSurface RecoverDecalSurface(float cx,float cy,float cz,float nx,float ny,float nz,int view,int pose,int pitch){
 constexpr float k=.70710678118f,co=.86602540378f,rad=.01745329252f;
 float v=view==0?1.f:-1.f;
 DecalSurface p{v*(k*cx+k*.5f*cy-k*co*cz),v*(-k*cx+k*.5f*cy-k*co*cz),co*cy+.5f*cz,
  v*(k*nx+k*.5f*ny-k*co*nz),v*(-k*nx+k*.5f*ny-k*co*nz),co*ny+.5f*nz};
 float sign=(float)(pose-1);
 RotateXY(p.x,p.y,-sign*7*rad);RotateXY(p.nx,p.ny,-sign*7*rad);
 p.z-=.45f;RotateXZ(p.x,p.z,sign*1.4f*rad);RotateXZ(p.nx,p.nz,sign*1.4f*rad);
 RotateYZ(p.y,p.z,pitch*1.5f*rad);RotateYZ(p.ny,p.nz,pitch*1.5f*rad);p.z+=.45f;
 if(view==1){p.x=-p.x;p.nx=-p.nx;}
 return p;
}
inline bool DecalSide(const DecalSurface &p){return p.x<-.40f&&p.nx<-.48f&&-p.nx>std::abs(p.ny)*.9f&&p.nz<.62f;}
inline std::pair<float,float> DecalUV(const DecalSurface &p,const DecalPlacement &d,float aspect){
 float x=-p.y-d.forward,y=p.z-d.height;
 RotateXY(x,y,-d.angle*.01745329252f);
 return {.5f+x/d.width,.5f-y/(d.width/aspect)};
}
inline void ClampDecal(DecalPlacement &d){d.forward=std::clamp(d.forward,-2.5f,2.5f);d.height=std::clamp(d.height,-.3f,1.8f);d.width=std::clamp(d.width,.3f,4.8f);d.angle=std::clamp(d.angle,-60.f,60.f);}
}
