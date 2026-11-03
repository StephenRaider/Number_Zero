#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform float spritePixels;
uniform float headlights;
uniform float taillights;
uniform float braking;
uniform float indicatorLeft;
uniform float indicatorRight;
vec3 seed(vec2 uv){
 if(any(lessThan(uv,vec2(0)))||any(greaterThan(uv,vec2(1))))return vec3(0);
 vec4 e=texture(texture0,uv);
 float turn=e.a<.5?indicatorLeft:indicatorRight;
 return min(max(e.r-.00785,0.)*14.,1.)*headlights*vec3(1.,.89,.69)
  +e.g*max(taillights*.50,braking*.90)*vec3(1.,.035,.012)
  +e.b*turn*.28*vec3(1.,.33,.015);
}
void main(){
 // Expanded draw bounds keep the small Gaussian halo away from atlas clipping.
 vec2 uv=(fragTexCoord*(spritePixels+12.)-6.)/spritePixels;
 vec3 glow=vec3(0);float total=0;
 for(int y=-3;y<=3;y++)for(int x=-3;x<=3;x++){
  float weight=exp(-float(x*x+y*y)*.30);total+=weight;
  glow+=seed(uv+vec2(x,y)*1.15/spritePixels)*weight;
 }
 finalColor=vec4(glow/total*.64,1.);
}
