#include "decal_projection.h"
#include <array>
#include <cstdio>
#include <stdexcept>
static void Require(bool b,const char *message){if(!b)throw std::runtime_error(message);}
static void Near(float a,float b){Require(std::abs(a-b)<.00002f,"Decal moved between camera/steering/pitch views");}
int main(){try{
 constexpr float k=.70710678118f,co=.86602540378f,rad=.01745329252f;
 for(int view=0;view<2;view++)for(int pose=0;pose<3;pose++)for(int pitch=-2;pitch<=2;pitch++){
  for(float forward:{-1.2f,0.f,1.3f})for(float height:{.35f,.8f}){
   // A fixed physical point on the right-side paint, projected by the actual
   // Blender transforms, must recover the same UV in all 30 rendered poses.
   float x=-.82f,y=-forward,z=height,nx=-1,ny=0,nz=0;
   if(view){x=-x;nx=-nx;}float sign=(float)(pose-1);
   z-=.45f;nz::RotateYZ(y,z,-pitch*1.5f*rad);nz::RotateYZ(ny,nz,-pitch*1.5f*rad);
   nz::RotateXZ(x,z,-sign*1.4f*rad);nz::RotateXZ(nx,nz,-sign*1.4f*rad);z+=.45f;
   nz::RotateXY(x,y,sign*7*rad);nz::RotateXY(nx,ny,sign*7*rad);
   float v=view?-1.f:1.f;
   auto surface=nz::RecoverDecalSurface(v*k*(x-y),v*k*.5f*(x+y)+co*z,-v*k*co*(x+y)+.5f*z,
    v*k*(nx-ny),v*k*.5f*(nx+ny)+co*nz,-v*k*co*(nx+ny)+.5f*nz,view,pose,pitch);
   Near(surface.x,-.82f);Near(surface.y,-forward);Near(surface.z,height);Require(nz::DecalSide(surface),"Right-side paint rejected");
   nz::DecalPlacement d;d.forward=.2f;d.height=.6f;d.width=3.1f;d.angle=17;
   auto uv=nz::DecalUV(surface,d,3.f),expected=nz::DecalUV({-.82f,-forward,height,-1,0,0},d,3.f);Near(uv.first,expected.first);Near(uv.second,expected.second);
  }
 }
 Require(!nz::DecalSide({.82f,0,.7f,1,0,0}),"Decal leaks onto opposite side");
 Require(!nz::DecalSide({-.7f,-1,.8f,0,0,1}),"Decal leaks onto bonnet/roof");
 Require(!nz::DecalSide({-.7f,-2,.4f,-.3f,-.95f,0}),"Decal leaks onto front bumper");
 nz::DecalPlacement d;auto centre=nz::DecalUV({-.8f,0,.67f,-1,0,0},d,3);Near(centre.first,.5);Near(centre.second,.5);
 d.forward=.3f;auto moved=nz::DecalUV({-.8f,0,.67f,-1,0,0},d,3);Require(moved.first<centre.first,"Move control has wrong direction");
 d.width=-1;d.height=9;d.forward=99;d.angle=-100;nz::ClampDecal(d);Require(d.width==.3f&&d.height==1.8f&&d.forward==2.5f&&d.angle==-60,"Placement bounds broken");
 std::puts("PASS decals: same physical UV in 30 views, opposite-side/roof/bumper rejection, movement and bounds");return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
