#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform sampler2D texture0,frontMap;
uniform float nativeSize,spritePixels,baseDepth,station,visibleAhead;
uniform int layerMode;
float unpack16(vec2 v){return (round(v.x*255.)*256.+round(v.y*255.))/65535.;}
void main(){
 float footprint=nativeSize/spritePixels;vec4 colour;vec2 depthUV=vec2(fragTexCoord.x,1.-fragTexCoord.y);
 if(footprint<=1.0001)colour=texture(texture0,fragTexCoord);
 else{
  vec2 centre=fragTexCoord*nativeSize,lo=centre-footprint*.5,hi=centre+footprint*.5;ivec2 first=ivec2(floor(lo));vec4 sum=vec4(0);float best=0.;
  for(int y=0;y<4;y++)for(int x=0;x<4;x++){
   ivec2 p=first+ivec2(x,y);if(any(lessThan(p,ivec2(0)))||any(greaterThanEqual(p,ivec2(nativeSize))))continue;
   vec2 a=vec2(p),overlap=max(vec2(0),min(a+1.,hi)-max(a,lo));float weight=overlap.x*overlap.y;vec4 sampleColour=texelFetch(texture0,p,0);sum+=sampleColour*weight;
   if(sampleColour.a*weight>best){best=sampleColour.a*weight;depthUV=vec2((float(p.x)+.5)/nativeSize,1.-(float(p.y)+.5)/nativeSize);}
  }
  colour=sum.a>.00001?vec4(sum.rgb/sum.a,clamp(sum.a/(footprint*footprint),0.,1.)):vec4(0);
 }
 colour*=fragColor;
 if(colour.a<.02)discard;
 float w=(unpack16(texture(frontMap,depthUV).rg)-.5)*12.+baseDepth;gl_FragDepth=clamp(.5-w/256.,.001,.999);

 if(layerMode==2){float encoded=round(clamp(w/256.+.5,0.,1.)*65535.);finalColor=vec4(floor(encoded/256.)/255.,mod(encoded,256.)/255.,0.,colour.a);}
 else finalColor=vec4(colour.rgb,colour.a);
}
