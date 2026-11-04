#version 330
out vec4 finalColor;
uniform vec2 resolution, screenAnchor, carOrigin;
uniform float pixelsPerMetre,spritePixels,carDepth,station,visibleAhead;
uniform sampler2D carAlbedo,carFront,carBack;
uniform int lightCount,streetCount;
uniform vec3 lightPosition[32],lightColour[32];
uniform float lightPower[32],lightRadius[32];
float u16(vec2 v){return (round(v.x*255.)*256.+round(v.y*255.))/65535.;}
bool blocked(vec3 p,vec3 lamp){
 vec3 delta=lamp-p;
 vec2 uv0=(screenAnchor+vec2(p.x,-p.y)*pixelsPerMetre-carOrigin)/spritePixels;
 vec2 duv=vec2(delta.x,-delta.y)*pixelsPerMetre/spritePixels;
 float enter=0.,leave=1.;
 for(int axis=0;axis<2;axis++){
  if(abs(duv[axis])<.000001){if(uv0[axis]<0.||uv0[axis]>1.)return false;}
  else {float a=-uv0[axis]/duv[axis],b=(1.-uv0[axis])/duv[axis];enter=max(enter,min(a,b));leave=min(leave,max(a,b));}
 }
 if(leave<=enter)return false;
 float count=ceil(clamp(length(duv)*(leave-enter)*256.,4.,48.));
 for(int k=0;k<48;k++){
  if(float(k)>=count)break;
  float t=mix(enter,leave,(float(k)+.5)/count);
  if(t*length(delta)<.025)continue;
  vec2 uv=uv0+duv*t;if(texture(carAlbedo,uv).a<.5)continue;
  float front=(u16(texture(carFront,uv).rg)-.5)*12.+carDepth;
  float back=(u16(texture(carBack,uv).gb)-.5)*12.+carDepth;
  float ray=p.z+delta.z*t;
  if(ray<front-.025&&ray>back-.025)return true;
 }return false;
}
void main(){
 vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);
 vec2 camera=vec2((pixel.x-screenAnchor.x)/pixelsPerMetre,(screenAnchor.y-pixel.y)/pixelsPerMetre);
 vec3 p=vec3(camera,-camera.y*1.7320508),normal=vec3(0,.8660254,.5);
 float total=.125,occluded=0.;
 for(int i=0;i<32;i++){
  if(i>=streetCount)break;
  vec3 delta=lightPosition[i]-p;float d2=dot(delta,delta),r2=lightRadius[i]*lightRadius[i];
  float a=max(0.,1.-d2/r2);a*=a;float tail=max(0.,1.-d2/(r2*4.84));a+=.14*tail*tail;
  float energy=a*lightPower[i]*max(0.,dot(normal,normalize(delta)))*dot(pow(lightColour[i],vec3(2.2)),vec3(.2126,.7152,.0722));
  total+=energy;if(energy>.003&&blocked(p,lightPosition[i]))occluded+=energy;
 }
 float fade=clamp((visibleAhead-station)/8.,0.,1.);
 finalColor=vec4(0,0,0,clamp(occluded/total,0.,.85)*fade);
}
