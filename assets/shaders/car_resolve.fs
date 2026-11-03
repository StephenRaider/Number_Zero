#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform float spritePixels;
void main(){
 // Exact box coverage of the native lit image. Never interpolate packed normal/depth bytes.
 float footprint=128./spritePixels;
 if(footprint<=1.0001){finalColor=texture(texture0,fragTexCoord);return;}
 vec2 centre=fragTexCoord*128.,lo=centre-footprint*.5,hi=centre+footprint*.5;
 ivec2 first=ivec2(floor(lo));vec4 sum=vec4(0);
 for(int y=0;y<4;y++)for(int x=0;x<4;x++){
  ivec2 pixel=first+ivec2(x,y);vec2 low=vec2(pixel);
  vec2 overlap=max(vec2(0),min(low+1.,hi)-max(low,lo));
  if(any(lessThan(pixel,ivec2(0)))||any(greaterThanEqual(pixel,ivec2(128))))continue;
  sum+=texelFetch(texture0,pixel,0)*(overlap.x*overlap.y);
 }
 float alpha=sum.a/(footprint*footprint);
 finalColor=sum.a>.00001?vec4(sum.rgb/sum.a,clamp(alpha,0.,1.)):vec4(0);
}
