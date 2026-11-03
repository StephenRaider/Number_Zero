#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec2 resolution,blurPixels;
void main(){
 if(length(blurPixels)<.01){finalColor=texture(texture0,fragTexCoord);return;}
 vec2 delta=vec2(blurPixels.x,-blurPixels.y)/resolution;vec4 sum=vec4(0);float total=0;
 for(int i=0;i<9;i++){float t=float(i)/8.-.5,w=exp(-t*t*8.);sum+=texture(texture0,clamp(fragTexCoord+delta*t,.5/resolution,1.-.5/resolution))*w;total+=w;}
 finalColor=sum/total;
}
