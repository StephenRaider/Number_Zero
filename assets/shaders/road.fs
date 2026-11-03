#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform vec2 resolution;
uniform vec2 screenAnchor;
uniform float travelled;
uniform float garage;
uniform float playerLateral;
uniform float headlights;
uniform float taillights;
uniform float braking;
uniform vec2 carOrigin;
uniform float carDepth;
uniform sampler2D carAlbedo;
uniform sampler2D carFront;
uniform sampler2D carBack;
uniform int lightCount;
uniform int streetCount;
uniform vec3 lightPosition[12];
uniform vec3 lightColour[12];
uniform float lightPower[12];
uniform float lightRadius[12];
float unpack16(float high,float low){return (round(high*255.)*256.+round(low*255.))/65535.;}
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
bool shadow(vec3 p,vec3 lamp){
 vec3 delta=lamp-p;
 for(int k=1;k<28;k++){
  float t=float(k)/28.;vec3 q=p+delta*t;
  vec2 pixel=screenAnchor+vec2(q.x,-q.y)*26.;vec2 uv=(pixel-carOrigin)/128.;
  if(any(lessThan(uv,vec2(0)))||any(greaterThanEqual(uv,vec2(1))))continue;
  if(texture(carAlbedo,uv).a<.5)continue;
  vec4 a=texture(carFront,uv),b=texture(carBack,uv);
  float front=(unpack16(a.r,a.g)-.5)*5.2+carDepth;
  float back=(unpack16(b.g,b.b)-.5)*5.2+carDepth;
  if(q.z<front-.025&&q.z>back-.03)return true;
 }
 return false;
}
void main(){
 vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);
 vec2 camera=vec2((pixel.x-screenAnchor.x)/26.,(screenAnchor.y-pixel.y)/26.);
 float sum=camera.y/.35355339,difference=camera.x/.70710678;
 float s=(sum-difference)*.5,lateral=(sum+difference)*.5,absoluteS=s+travelled;
 vec3 p=vec3(camera,-camera.y*1.7320508),normal=vec3(0,.8660254,.5);
 float noise=hash(floor(vec2(absoluteS,lateral)*10.));
 vec3 base=vec3(.145,.16,.19)*( .89+.16*noise);
 if(garage>.5){
  base=vec3(.20,.225,.25)*( .96+.04*noise);
  float grid=min(abs(fract(absoluteS/2.)-.5),abs(fract(lateral/2.)-.5));
  if(grid<.015)base*=.7;
 }else{
  if(abs(lateral)<7.2){
   base=vec3(.205,.215,.24)*(.92+.12*noise);
   for(int i=-1;i<=1;i++)if(abs(lateral-float(i)*3.5)<.055&&mod(absoluteS,9.)<3.)base=vec3(.77,.79,.76);
   if(abs(abs(lateral)-6.9)<.07)base=vec3(.78,.68,.36);
   for(int i=0;i<4;i++)if(abs(lateral-(-5.25+float(i)*3.5))<.8)base*=.94;
  }else if(abs(lateral)<8.2)base=vec3(.25,.265,.29);
  else base=vec3(.075,.105,.11)*(.8+.3*noise);
  if(abs(abs(lateral)-7.65)<.18&&mod(absoluteS,5.)<.5)base=vec3(.66,.65,.52);
 }
 vec3 lighting=vec3(.105,.12,.16);int key=0;float nearest=1e10;
 for(int i=0;i<12;i++){
  if(i>=lightCount)break;
  vec3 delta=lightPosition[i]-p;float d2=dot(delta,delta);
  float a=max(0.,1.-d2/(lightRadius[i]*lightRadius[i]));a*=a;
  float diff=max(0.,dot(normal,normalize(delta)));
  lighting+=pow(lightColour[i],vec3(2.2))*a*lightPower[i]*diff;
  if(i<streetCount&&d2<nearest){key=i;nearest=d2;}
 }
 if(abs(pixel.x-carOrigin.x-64.)<145.&&abs(pixel.y-carOrigin.y-65.)<105.&&streetCount>0){
  if(shadow(p,lightPosition[key]))lighting*=.36;
 }
 vec3 lit=pow(base,vec3(2.2))*lighting;
 if(garage<.5&&headlights>.5){
  float ahead=s-1.35;
  if(ahead>0.&&ahead<30.){
   float width=.65+ahead*.14;float side=(lateral-playerLateral)/width;
   float beam=exp(-side*side*3.0)*pow(1.-ahead/30.,1.6);
   lit+=pow(base,vec3(2.2))*vec3(1.2,1.06,.75)*beam*1.8;
  }
 }
 // Mild blue ambient falloff keeps the outer scene quiet without hiding the road.
 float vignette=1.-.22*pow(length((pixel/resolution-.5)*vec2(1.,.8)),1.5);
 finalColor=vec4(pow(max(lit,vec3(0)),vec3(1./2.2))*vignette,1.);
}
