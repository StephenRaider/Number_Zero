#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform sampler2D texture0;
uniform sampler2D normalMap;
uniform sampler2D depthMap;
uniform sampler2D auxiliaryMap;
uniform sampler2D emissionMap;
uniform vec2 screenAnchor;
uniform vec2 carOrigin;
uniform float carDepth;
uniform int lightCount;
uniform vec3 lightPosition[12];
uniform vec3 lightColour[12];
uniform float lightPower[12];
uniform float lightRadius[12];
uniform float headlights;
uniform float taillights;
uniform float braking;
float unpack16(float high,float low){return (round(high*255.0)*256.0+round(low*255.0))/65535.0;}
float band(float x){float n=x*7.0,b=floor(n),t=smoothstep(.34,.66,fract(n));return (b+t)/7.0;}
vec3 emission(vec2 uv){
 vec3 e=texture(emissionMap,uv).rgb;
 return e.r*headlights*vec3(1.25,1.08,.75)+e.g*max(taillights*.6,braking*1.9)*vec3(1.0,.025,.012);
}
bool occluded(vec3 p,vec3 lamp){
 vec3 delta=lamp-p;
 float steps=clamp(length(delta.xy)*26.0,6.0,22.0);
 for(int step=2;step<22;step++){
  if(float(step)>=steps)break;
  float t=float(step)/steps;
  vec2 uv=fragTexCoord+vec2(delta.x,-delta.y)*26.0/128.0*t;
  if(any(lessThan(uv,vec2(0)))||any(greaterThanEqual(uv,vec2(1))))continue;
  if(texture(texture0,uv).a<.5)continue;
  vec4 d=texture(depthMap,uv),a=texture(auxiliaryMap,uv);
  float front=(unpack16(d.r,d.g)-.5)*5.2+carDepth;
  float back=(unpack16(a.g,a.b)-.5)*5.2+carDepth;
  float ray=p.z+delta.z*t;
  if(ray<front-.045&&ray>back-.02)return true;
 }
 return false;
}
void main(){
 vec4 base=texture(texture0,fragTexCoord);
 if(base.a<.5){
  vec3 glow=vec3(0);
  for(int i=0;i<8;i++){
   float angle=float(i)*.785398;
   vec2 dir=vec2(cos(angle),sin(angle))/128.0;
   glow+=emission(fragTexCoord+dir*1.6)*.06+emission(fragTexCoord+dir*3.0)*.025;
  }
  float alpha=clamp(max(glow.r,max(glow.g,glow.b)),0.,.45);
  finalColor=vec4(glow/max(alpha,.001),alpha);return;
 }
 vec4 n=texture(normalMap,fragTexCoord),d=texture(depthMap,fragTexCoord),a=texture(auxiliaryMap,fragTexCoord);
 vec2 xy=vec2(unpack16(n.r,n.g),unpack16(n.b,a.r))*2.0-1.0;
 vec3 normal=normalize(vec3(xy,sqrt(max(0.,1.-dot(xy,xy)))));
 vec2 pixel=carOrigin+fragTexCoord*128.0;
 vec3 p=vec3((pixel.x-screenAnchor.x)/26.,(screenAnchor.y-pixel.y)/26.,(unpack16(d.r,d.g)-.5)*5.2+carDepth);
 vec3 diffuse=vec3(0),specular=vec3(0);
 for(int i=0;i<12;i++){
  if(i>=lightCount)break;
  vec3 delta=lightPosition[i]-p;float distance2=dot(delta,delta);
  float att=max(0.,1.-distance2/(lightRadius[i]*lightRadius[i]));att*=att;
  if(att<=0.)continue;
  vec3 direction=normalize(delta);float lambert=max(0.,dot(normal,direction));
  if(lambert<=0.||occluded(p,lightPosition[i]))continue;
  vec3 colour=pow(lightColour[i],vec3(2.2));
  diffuse+=colour*att*lambert*lightPower[i];
  vec3 halfway=normalize(direction+vec3(0,0,1));
  float highlight=pow(max(0.,dot(normal,halfway)),75.0)*d.b*1.35;
  specular+=colour*att*highlight*lightPower[i];
 }
 float peak=max(diffuse.r,max(diffuse.g,diffuse.b));if(peak>0.)diffuse*=band(peak)/peak;
 float hemi=.13*(.55+.45*max(0.,dot(normal,vec3(0,.8660254,.5))));
 vec3 lit=pow(base.rgb,vec3(2.2))*(vec3(hemi,.97*hemi,1.13*hemi)+diffuse)+specular;
 lit+=emission(fragTexCoord)*1.1;
 finalColor=vec4(pow(max(lit,vec3(0)),vec3(1./2.2)),1.0);
}
