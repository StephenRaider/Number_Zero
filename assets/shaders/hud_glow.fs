#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec2 resolution;
void main(){
 vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);if(pixel.x>172.||pixel.y<resolution.y-126.)discard;
 vec3 glow=vec3(0);float total=0;
 for(int y=-2;y<=2;y++)for(int x=-2;x<=2;x++){float w=exp(-float(x*x+y*y)*.65);glow+=max(texture(texture0,fragTexCoord+vec2(x,y)*1.1/resolution).rgb-vec3(.12),vec3(0))*w;total+=w;}
 finalColor=vec4(glow/total*.24,1.);
}
