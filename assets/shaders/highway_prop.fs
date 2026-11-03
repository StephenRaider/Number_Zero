#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform sampler2D normalMap;
uniform sampler2D depthMap;
uniform sampler2D emissionMap;
uniform sampler2D carAlbedo;
uniform sampler2D carFront;
uniform vec2 resolution,screenAnchor,carOrigin;
uniform float pixelsPerMetre,spritePixels,carDepth,propDepth,depthRange,sourcePixels,station,visibleAhead,gantry;
uniform int lightCount,streetCount,layerMode;
uniform vec3 lightPosition[32],lightColour[32];
uniform float lightPower[32],lightRadius[32];
float unpack16(float high,float low){return (round(high*255.)*256.+round(low*255.))/65535.;}
void main(){
 vec4 base=texture(texture0,fragTexCoord);if(base.a<.035)discard;
 vec2 uv=fragTexCoord;vec4 depth=texture(depthMap,uv);
 // A filtered thin-pole edge still needs a nearby *covered* geometry sample.
 if(depth.a<.01){
  float best=0.;vec2 chosen=uv;
  for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){
   vec2 q=fragTexCoord+vec2(x,y)/sourcePixels;float coverage=texture(depthMap,q).a;
   if(coverage>best){best=coverage;chosen=q;}
  }
  if(best<.01)discard;uv=chosen;depth=texture(depthMap,uv);
 }
 float w=(unpack16(depth.r,depth.g)-.5)*depthRange+propDepth;
 vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);
 vec2 carUV=(pixel-carOrigin)/spritePixels;
 if(layerMode==3&&all(greaterThanEqual(carUV,vec2(0)))&&all(lessThan(carUV,vec2(1)))&&texture(carAlbedo,carUV).a>.5){
  vec4 front=texture(carFront,carUV);float carW=(unpack16(front.r,front.g)-.5)*5.2+carDepth;
  if(w<carW-.018)discard;
 }
 gl_FragDepth=clamp(.5-w/256.,.001,.999);
 if(layerMode==2){
  float encoded=round(clamp(w/256.+.5,0.,1.)*65535.);
  finalColor=vec4(floor(encoded/256.)/255.,mod(encoded,256.)/255.,0.,base.a);return;
 }
 vec4 encodedNormal=texture(normalMap,uv);vec2 xy=vec2(unpack16(encodedNormal.r,encodedNormal.g),unpack16(encodedNormal.b,encodedNormal.a))*2.-1.;
 vec3 normal=normalize(vec3(xy,depth.b*2.-1.));
 vec3 p=vec3((pixel.x-screenAnchor.x)/pixelsPerMetre,(screenAnchor.y-pixel.y)/pixelsPerMetre,w);
 vec3 lighting=mix(vec3(.15,.17,.20),vec3(.24,.25,.28),gantry),specular=vec3(0);
 for(int i=0;i<32;i++){
  if(i>=lightCount)break;
  vec3 delta=lightPosition[i]-p;float d2=dot(delta,delta),r2=lightRadius[i]*lightRadius[i];
  float core=max(0.,1.-d2/r2);float attenuation=core*core;
  if(i<streetCount){float tail=max(0.,1.-d2/(r2*4.84));attenuation+=.14*tail*tail;}
  if(attenuation<=0.)continue;
  vec3 direction=normalize(delta),colour=pow(lightColour[i],vec3(2.2));float diffuse=max(0.,dot(normal,direction));
  lighting+=colour*attenuation*lightPower[i]*diffuse*.8;
  float shine=pow(max(0.,dot(normal,normalize(direction+vec3(0,0,1)))),110.)*.22;
  specular+=colour*attenuation*lightPower[i]*shine;
 }
 // Albedo was packed premultiplied so filtered sprite edges have no dark fringe.
 vec3 albedo=base.rgb/max(base.a,.0001);
 float emitter=texture(emissionMap,uv).r;
 vec3 lit=pow(clamp(albedo,0.,1.),vec3(2.2))*lighting+specular+emitter*vec3(1.8,1.35,.72);
 float fade=1.-smoothstep(visibleAhead-8.,visibleAhead,station);
 finalColor=vec4(pow(max(lit,vec3(0)),vec3(1./2.2)),base.a*fade);
}
