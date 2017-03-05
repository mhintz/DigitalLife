#version 410

in highp vec3 TexCoord0;

out highp vec4 FragColor;

uniform samplerCube uCubeMap;

void main() {
  FragColor = texture(uCubeMap, TexCoord0);
}
