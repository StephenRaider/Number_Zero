#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform vec2 resolution;
uniform vec2 screenAnchor;
uniform float pixelsPerMetre;
uniform float spritePixels;
uniform float travelled;
uniform float garage;
uniform float playerLateral;
uniform float carYaw;
uniform float visibleAhead,raceFinishStation;
uniform int raceMode;
uniform float beamReach;
uniform float beamBase;
uniform float beamSpread;
uniform float headlights;
uniform int trafficCount;
uniform vec4 trafficBeam[12];
uniform float taillights;
uniform float braking;
uniform vec2 carOrigin;
uniform float carDepth;
uniform sampler2D roadSurface;
uniform sampler2D roadPatched;
uniform sampler2D roadShoulder;
uniform sampler2D roadMarkings;
uniform float roadRepeat;
uniform sampler2D carAlbedo;
uniform sampler2D carFront;
uniform sampler2D carBack;
uniform sampler2D trafficGeometry;
uniform int trafficShadowCount,shadowProbe;
uniform vec4 trafficOccluder[32],trafficBounds[32],carBounds;
uniform int lightCount;
uniform int streetCount;
uniform vec3 lightPosition[32];
uniform vec3 lightColour[32];
uniform float lightPower[32];
uniform float lightRadius[32];
float unpack16(float high,float low){return (round(high*255.)*256.+round(low*255.))/65535.;}
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
bool shadow(vec3 p,vec3 lamp,int actor){
 vec3 delta=lamp-p;bool player=actor<0;
 vec2 origin=player?carOrigin:trafficOccluder[actor].xy;
 float size=player?spritePixels:256.*pixelsPerMetre/26.;
 float baseDepth=player?carDepth:trafficOccluder[actor].z;
 float range=player?5.2:12.,nativeSize=player?128.:256.;
 vec2 uv0=(screenAnchor+vec2(p.x,-p.y)*pixelsPerMetre-origin)/size,duv=vec2(delta.x,-delta.y)*pixelsPerMetre/size;
 vec4 bounds=player?carBounds:trafficBounds[actor];
 float enter=0.,leave=1.;
 for(int axis=0;axis<2;axis++){
  if(abs(duv[axis])<.000001){if(uv0[axis]<bounds[axis]||uv0[axis]>bounds[axis+2])return false;}
  else{float a=(bounds[axis]-uv0[axis])/duv[axis],b=(bounds[axis+2]-uv0[axis])/duv[axis];enter=max(enter,min(a,b));leave=min(leave,max(a,b));}
 }
 if(leave<=enter)return false;
 float count=ceil(clamp(length(duv)*(leave-enter)*nativeSize*1.25,4.,64.));
 for(int k=0;k<64;k++){
  if(float(k)>=count)break;
  float t=mix(enter,leave,(float(k)+.5)/count);if(t*length(delta)<.035)continue;
  vec2 uv=uv0+duv*t;float front,back;
  if(player){
   if(texture(carAlbedo,uv).a<.5)continue;
   front=unpack16(texture(carFront,uv).r,texture(carFront,uv).g);
   back=unpack16(texture(carBack,uv).g,texture(carBack,uv).b);
  }else{
   float cell=trafficOccluder[actor].w;
   vec2 atlas=(vec2(mod(cell,4.),floor(cell/4.))*256.+uv*256.)/vec2(1024,2048);
   vec4 depths=texture(trafficGeometry,vec2(atlas.x,1.-atlas.y));
   front=unpack16(depths.r,depths.g);if(front==0.)continue;
   back=unpack16(depths.b,depths.a);
  }
  front=(front-.5)*range+baseDepth;back=(back-.5)*range+baseDepth;
  float ray=p.z+delta.z*t;if(ray<front-.045&&ray>back-.025)return true;
 }
 return false;
}
void main(){
 vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);
 vec2 camera=vec2((pixel.x-screenAnchor.x)/pixelsPerMetre,(screenAnchor.y-pixel.y)/pixelsPerMetre);
 float sum=camera.y/.35355339,difference=camera.x/.70710678;
 float s=garage>.5?(difference-sum)*.5:(difference+sum)*.5;
 float lateral=garage>.5?(sum+difference)*.5:(difference-sum)*.5,absoluteS=s+travelled;
 vec3 p=vec3(camera,-camera.y*1.7320508),normal=vec3(0,.8660254,.5);
 float noise=hash(floor(vec2(absoluteS,lateral)*10.));
 vec3 base=vec3(.145,.16,.19)*( .89+.16*noise);
 if(garage>.5){
  base=vec3(.20,.225,.25)*( .96+.04*noise);
  float grid=min(abs(fract(absoluteS/2.)-.5),abs(fract(lateral/2.)-.5));
  if(grid<.015)base*=.7;
 }else{
  if(abs(lateral)<7.0){
   vec2 roadUV=vec2((lateral+7.)/7.,absoluteS/10.5);
   vec3 asphalt=texture(roadSurface,roadUV).rgb;
   float section=hash(vec2(floor(absoluteS/roadRepeat),floor((lateral+7.)/7.)));
   float wear=section>.72?.28:0.;
   base=mix(asphalt,texture(roadPatched,roadUV).rgb,wear);
   float paint=texture(roadMarkings,vec2(roadUV.x,absoluteS/roadRepeat)).r;
   base=mix(base,vec3(.74,.75,.72),paint);
   // Small dry wheel-track variation remains fixed in world space.
   for(int lane=0;lane<4;lane++)if(abs(lateral-(-5.25+float(lane)*3.5))<.6)base*=.97;
  }else if(abs(lateral)<8.2){
   base=texture(roadShoulder,vec2(lateral*.7,absoluteS*.10)).rgb*.72;
   if(abs(abs(lateral)-7.85)<.04)base*=.65;
  }else{
   // Quiet shoulder/embankment beyond the guardrail instead of a flat empty strip.
   float drainage=.5+.5*sin(absoluteS*1.7+lateral*2.3);
   base=vec3(.065,.087,.083)*(.85+.12*noise+.06*drainage);
  }

 }
 if(garage<.5&&raceMode>0&&abs(lateral)<7.&&abs(s-raceFinishStation)<.6){
  float checker=mod(floor((lateral+7.)*2.)+floor((s-raceFinishStation+.6)*2.),2.);
  base=mix(vec3(.055,.065,.075),vec3(.68,.70,.67),checker);
 }
 vec3 lighting=vec3(.105,.12,.16);
 for(int i=0;i<32;i++){
  if(i>=lightCount)break;
  vec3 delta=lightPosition[i]-p;float d2=dot(delta,delta);
  float a=max(0.,1.-d2/(lightRadius[i]*lightRadius[i]));a*=a;
  if(i<streetCount){float tail=max(0.,1.-d2/(lightRadius[i]*lightRadius[i]*4.84));a+=.14*tail*tail;}
  if(a*lightPower[i]<.0001)continue;
  float diff=max(0.,dot(normal,normalize(delta)));
  bool blocked=false;
  if(i<streetCount){
   blocked=shadow(p,lightPosition[i],-1);
   for(int actor=0;actor<32;actor++){if(actor>=trafficShadowCount||blocked)break;blocked=shadow(p,lightPosition[i],actor);}
  }
  float visibility=blocked?.20:1.;
  lighting+=pow(lightColour[i],vec3(2.2))*a*lightPower[i]*diff*visibility;
 }
 if(shadowProbe>0){finalColor=vec4(lighting,1);return;}
 vec3 lit=pow(base,vec3(2.2))*lighting;
 if(headlights>.5){
  vec2 offset=vec2(s,lateral-playerLateral);
  float ahead=dot(offset,vec2(cos(carYaw),sin(carYaw)))-2.;
  float across=dot(offset,vec2(-sin(carYaw),cos(carYaw)));
  if(ahead>0.&&ahead<beamReach){
   float width=beamBase+ahead*beamSpread;
   float left=(across+.56)/width,right=(across-.56)/width;
   float beam=(exp(-left*left*2.)+exp(-right*right*2.))*pow(1.-ahead/beamReach,1.6)*smoothstep(0.,.25,ahead);
   lit+=pow(base,vec3(2.2))*vec3(1.2,1.06,.75)*beam*1.3;
  }
 }
 for(int i=0;i<12;i++){
  if(i>=trafficCount)break;
  vec2 delta=vec2(s-trafficBeam[i].x,lateral-trafficBeam[i].y);float yaw=trafficBeam[i].z;
  float ahead=dot(delta,vec2(cos(yaw),sin(yaw)))-trafficBeam[i].w;
  if(ahead<=0.||ahead>=32.)continue;
  float across=dot(delta,vec2(-sin(yaw),cos(yaw))),width=.24+ahead*.12;
  float a=(across-.56)/width,b=(across+.56)/width;
  float beam=(exp(-a*a*2.)+exp(-b*b*2.))*pow(1.-ahead/32.,1.6)*smoothstep(0.,.25,ahead);
  lit+=pow(base,vec3(2.2))*vec3(1.1,1.03,.83)*beam*.65;
 }
 // Same longitudinal visibility for every display. Extra width cannot reveal earlier hazards.
 // Horizon haze is applied once after opaque world compositing.
 // Mild blue ambient falloff keeps the outer scene quiet without hiding the road.
 float vignette=1.-.22*pow(length((pixel/resolution-.5)*vec2(1.,.8)),1.5);
 finalColor=vec4(pow(max(lit,vec3(0)),vec3(1./2.2))*vignette,1.);
}
