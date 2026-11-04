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
uniform float pixelsPerMetre;
uniform float spritePixels;
uniform vec2 carOrigin;
uniform float carDepth;
uniform int lightCount;
uniform int streetCount;
uniform vec3 lightPosition[32];
uniform vec3 lightColour[32];
uniform float lightPower[32];
uniform float lightRadius[32];
uniform float indicatorLeft;
uniform float indicatorRight;
uniform float headlights;
uniform float taillights;
uniform float braking;
uniform float studioLight;
float unpack16(float high,float low){return (round(high*255.0)*256.0+round(low*255.0))/65535.0;}
float band(float x){float n=x*10.0,b=floor(n),t=smoothstep(.20,.80,fract(n));return (b+t)/10.0;}
vec3 emission(vec2 uv){
 vec4 texel=texture(emissionMap,uv);vec3 e=texel.rgb;float indicator=texel.a<.5?indicatorLeft:indicatorRight;
 return e.r*headlights*vec3(1.25,1.08,.75)+e.g*max(taillights*.6,braking*1.9)*vec3(1.0,.025,.012)+e.b*indicator*vec3(1.8,.48,.015);
}
bool occluded(vec3 p,vec3 lamp){
 vec3 delta=lamp-p;
 vec2 uv0=fragTexCoord,duv=vec2(delta.x,-delta.y)*pixelsPerMetre/spritePixels;
 float enter=0.,leave=1.;
 for(int axis=0;axis<2;axis++){
  if(abs(duv[axis])<.000001){if(uv0[axis]<0.||uv0[axis]>1.)return false;}
  else{float a=-uv0[axis]/duv[axis],b=(1.-uv0[axis])/duv[axis];enter=max(enter,min(a,b));leave=min(leave,max(a,b));}
 }
 if(leave<=enter)return false;
 float count=ceil(clamp(length(duv)*(leave-enter)*128.*1.25,4.,40.));
 for(int k=0;k<40;k++){
  if(float(k)>=count)break;
  float t=mix(enter,leave,(float(k)+.5)/count);
  if(t*length(delta)<.10)continue;
  vec2 uv=uv0+duv*t;
  if(texture(texture0,uv).a<.5)continue;
  vec4 a=texture(depthMap,uv),b=texture(auxiliaryMap,uv);
  float front=(unpack16(a.r,a.g)-.5)*5.2+carDepth;
  float back=(unpack16(b.g,b.b)-.5)*5.2+carDepth;
  float ray=p.z+delta.z*t;
  if(ray<front-.045&&ray>back-.025)return true;
 }
 return false;

}
void main(){
 vec4 base=texture(texture0,fragTexCoord);
 if(base.a<.5){finalColor=vec4(0);return;}
 vec4 n=texture(normalMap,fragTexCoord),d=texture(depthMap,fragTexCoord),a=texture(auxiliaryMap,fragTexCoord);
 vec2 xy=vec2(unpack16(n.r,n.g),unpack16(n.b,a.r))*2.0-1.0;
 vec3 normal=normalize(vec3(xy,sqrt(max(0.,1.-dot(xy,xy)))));
 vec2 pixel=carOrigin+fragTexCoord*spritePixels;
 vec3 p=vec3((pixel.x-screenAnchor.x)/pixelsPerMetre,(screenAnchor.y-pixel.y)/pixelsPerMetre,(unpack16(d.r,d.g)-.5)*5.2+carDepth);
 // Material finish is independent of the base colour, including black paint and grey glazing.
 float paint=step(.25,d.a)*(1.-step(.75,d.a)),glass=step(.75,d.a);
 float diffuseResponse=mix(.72,.46,paint);diffuseResponse=mix(diffuseResponse,.045,glass);
 float shininess=mix(150.,420.,paint);shininess=mix(shininess,850.,glass);
 float specularStrength=mix(1.8,13.,paint);specularStrength=mix(specularStrength,20.,glass);
 float fresnel=pow(1.-max(normal.z,0.),5.);
 vec3 diffuse=vec3(0),specular=vec3(0);
 for(int i=0;i<32;i++){
  if(i>=lightCount)break;
  vec3 delta=lightPosition[i]-p;float distance2=dot(delta,delta);
  float att=max(0.,1.-distance2/(lightRadius[i]*lightRadius[i]));att*=att;
  if(i<streetCount){float tail=max(0.,1.-distance2/(lightRadius[i]*lightRadius[i]*4.84));att+=.14*tail*tail;}
  if(att<=0.)continue;
  vec3 direction=normalize(delta);float lambert=max(0.,dot(normal,direction));
  if(lambert<=0.||occluded(p,lightPosition[i]))continue;
  vec3 colour=pow(lightColour[i],vec3(2.2));
  diffuse+=colour*att*lambert*lightPower[i]*diffuseResponse;
  vec3 halfway=normalize(direction+vec3(0,0,1));
  // Point-source clearcoat: one narrow reflection, with no broad secondary specular wash.
  float highlight=pow(max(0.,dot(normal,halfway)),shininess)*d.b*specularStrength*(.85+.95*fresnel);
  specular+=colour*att*highlight*lightPower[i];
 }
 float peak=max(diffuse.r,max(diffuse.g,diffuse.b));if(peak>0.)diffuse*=band(peak)/peak;
 float hemi=.105*(.55+.45*max(0.,dot(normal,vec3(0,.8660254,.5))));
 // A grey base and weak cool environment reflection keep glazing readable between small lamp glints.
 float ambient=mix(hemi,.30,glass);
 vec3 environment=glass*vec3(.012,.018,.028)*(.04+.96*fresnel);
 vec3 lit=pow(base.rgb,vec3(2.2))*(vec3(ambient,.97*ambient,1.13*ambient)+diffuse)+specular+environment;
 if(studioLight>.5){
  // Fixed large softboxes: broad diffuse fill with long narrow bar reflections.
  // Directional analytic strips have no moving point-source hotspots.
  float fill=.62+.23*max(0.,normal.y)+.10*abs(normal.x);
  float bars=exp(-pow((normal.x-.42)/.055,2.))*smoothstep(-.1,.4,normal.y)
            +.65*exp(-pow((normal.x+.50)/.065,2.))*smoothstep(-.2,.3,normal.y);
  lit=pow(base.rgb,vec3(2.2))*fill+vec3(.86,.93,1.)*bars*mix(.08,.60,max(paint,glass));
 }
 lit+=emission(fragTexCoord)*1.1;
 finalColor=vec4(pow(max(lit,vec3(0)),vec3(1./2.2)),1.0);
}
