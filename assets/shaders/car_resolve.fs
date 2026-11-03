#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0,carFront,propColour,propDepth;
uniform float spritePixels,carDepth;
uniform vec2 resolution,blurPixels;
float unpack16(vec2 bytes){return (round(bytes.x*255.)*256.+round(bytes.y*255.))/65535.;}
vec4 foreground(float carW){
 vec2 screenUV=gl_FragCoord.xy/resolution,stepUV=vec2(blurPixels.x,-blurPixels.y)/resolution;
 vec4 sum=vec4(0);float total=0;
 for(int i=0;i<9;i++){
  float t=float(i)/8.-.5,weight=exp(-t*t*8.);total+=weight;
  vec2 uv=clamp(screenUV+stepUV*t,.5/resolution,1.-.5/resolution);
  vec4 depth=texture(propDepth,uv);if(depth.a<.025)continue;
  float w=(unpack16(depth.rg)-.5)*256.;
  if(w>=carW-.018)sum+=texture(propColour,uv)*weight;
 }
 return sum/total;
}
void main(){
 float footprint=128./spritePixels;vec4 car;vec2 depthUV=vec2(fragTexCoord.x,1.-fragTexCoord.y);
 if(footprint<=1.0001)car=texture(texture0,fragTexCoord);
 else{
  vec2 centre=fragTexCoord*128.,lo=centre-footprint*.5,hi=centre+footprint*.5;ivec2 first=ivec2(floor(lo));vec4 sum=vec4(0);float best=0;
  for(int y=0;y<4;y++)for(int x=0;x<4;x++){
   ivec2 pixel=first+ivec2(x,y);vec2 low=vec2(pixel),overlap=max(vec2(0),min(low+1.,hi)-max(low,lo));
   if(any(lessThan(pixel,ivec2(0)))||any(greaterThanEqual(pixel,ivec2(128))))continue;
   vec4 sampleColour=texelFetch(texture0,pixel,0);float weight=overlap.x*overlap.y;sum+=sampleColour*weight;
   if(sampleColour.a*weight>best){best=sampleColour.a*weight;depthUV=vec2((float(pixel.x)+.5)/128.,1.-(float(pixel.y)+.5)/128.);}
  }
  car=sum.a>.00001?vec4(sum.rgb/sum.a,clamp(sum.a/(footprint*footprint),0.,1.)):vec4(0);
 }
 if(car.a<.00001){finalColor=vec4(0);return;}
 float carW=(unpack16(texture(carFront,depthUV).rg)-.5)*5.2+carDepth;
 vec4 front=foreground(carW);
 finalColor=vec4(car.rgb*(1.-clamp(front.a,0.,1.))+front.rgb,car.a);
}
