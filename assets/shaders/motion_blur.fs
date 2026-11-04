#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec2 resolution,blurPixels,screenAnchor;
uniform float pixelsPerMetre,visibleAhead,driving;
void main(){
 vec4 colour;
 if(length(blurPixels)<.01)colour=texture(texture0,fragTexCoord);
 else{
  vec2 delta=vec2(blurPixels.x,-blurPixels.y)/resolution;vec4 sum=vec4(0);float total=0;
  for(int i=0;i<9;i++){float t=float(i)/8.-.5,w=exp(-t*t*8.);sum+=texture(texture0,clamp(fragTexCoord+delta*t,.5/resolution,1.-.5/resolution))*w;total+=w;}
  colour=sum/total;
 }
 // One opaque atmospheric horizon covers road, vehicles, props and their lights.
 // Objects keep their geometric coverage instead of becoming translucent.
 if(driving>.5){
  vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);
  vec2 camera=(vec2(pixel.x,-pixel.y)-vec2(screenAnchor.x,-screenAnchor.y))/pixelsPerMetre;
  float station=(camera.x/.70710678+camera.y/.35355339)*.5;
  float haze=smoothstep(visibleAhead-8.,visibleAhead,station);
  colour.rgb=pow(mix(pow(max(colour.rgb,vec3(0)),vec3(2.2)),vec3(.0012,.0020,.0035),haze),vec3(1./2.2));
 }
 finalColor=colour;
}
