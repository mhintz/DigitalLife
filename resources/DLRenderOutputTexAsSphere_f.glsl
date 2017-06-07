#version 410

in highp vec3 aCubeMapTexCoord;

out vec4 FragColor;

uniform samplerCube uCubeMapTex;
uniform float uFrameAlpha;

void main() {
  FragColor = uFrameAlpha * texture(uCubeMapTex, aCubeMapTexCoord);
}
