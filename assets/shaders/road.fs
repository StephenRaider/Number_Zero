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
uniform vec3 lightPosition[24];
uniform vec3 lightColour[24];
uniform float lightPower[24];
uniform float lightRadius[24];
float unpack16(float high,float low){return (round(high*255.)*256.+round(low*255.))/65535.;}
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
bool shadow(vec3 p,vec3 lamp){
 vec3 delta=lamp-p;
 vec2 uv0=(screenAnchor+vec2(p.x,-p.y)*26.-carOrigin)/128.,duv=vec2(delta.x,-delta.y)*26./128.;
 float enter=0.,leave=1.;
 for(int axis=0;axis<2;axis++){
  if(abs(duv[axis])<.000001){if(uv0[axis]<0.||uv0[axis]>1.)return false;}
  else{float a=-uv0[axis]/duv[axis],b=(1.-uv0[axis])/duv[axis];enter=max(enter,min(a,b));leave=min(leave,max(a,b));}
 }
 if(leave<=enter)return false;
 float count=ceil(clamp(length(duv)*(leave-enter)*128.*1.25,4.,64.));
 for(int k=0;k<64;k++){
  if(float(k)>=count)break;
  float t=mix(enter,leave,(float(k)+.5)/count);
  if(t*length(delta)<.035)continue;
  vec2 uv=uv0+duv*t;
  if(texture(carAlbedo,uv).a<.5)continue;
  vec4 a=texture(carFront,uv),b=texture(carBack,uv);
  float front=(unpack16(a.r,a.g)-.5)*5.2+carDepth;
  float back=(unpack16(b.g,b.b)-.5)*5.2+carDepth;
  float ray=p.z+delta.z*t;
  if(ray<front-.045&&ray>back-.025)return true;
 }
 return false;

}
void main(){
 vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);
 vec2 camera=vec2((pixel.x-screenAnchor.x)/26.,(screenAnchor.y-pixel.y)/26.);
 float sum=camera.y/.35355339,difference=camera.x/.70710678;
 float s=garage>.5?(sum-difference)*.5:(difference+sum)*.5;
 float lateral=garage>.5?(sum+difference)*.5:(difference-sum)*.5,absoluteS=s+travelled;
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
 vec3 lighting=vec3(.105,.12,.16);
 for(int i=0;i<24;i++){
  if(i>=lightCount)break;
  vec3 delta=lightPosition[i]-p;float d2=dot(delta,delta);
  float a=max(0.,1.-d2/(lightRadius[i]*lightRadius[i]));a*=a;
  if(i<streetCount){float tail=max(0.,1.-d2/(lightRadius[i]*lightRadius[i]*4.84));a+=.14*tail*tail;}
  if(a*lightPower[i]<.0001)continue;
  float diff=max(0.,dot(normal,normalize(delta)));
  float visibility=i<streetCount&&shadow(p,lightPosition[i])?.20:1.;
  lighting+=pow(lightColour[i],vec3(2.2))*a*lightPower[i]*diff*visibility;
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
