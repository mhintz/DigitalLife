#version 410

in highp vec3 TexCoord0;

out highp vec4 FragColor;

uniform samplerCube uCubeMap;
uniform float uFrameAlpha;

void main() {
  FragColor = uFrameAlpha * texture(uCubeMap, TexCoord0);
}
