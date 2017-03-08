#version 410

in highp vec3 aCubeMapTexCoord;

out vec4 FragColor;

uniform samplerCube uCubeMapTex;

void main() {
  FragColor = texture(uCubeMapTex, aCubeMapTexCoord);
}
