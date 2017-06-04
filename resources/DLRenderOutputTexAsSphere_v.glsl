#version 410

in vec4 ciPosition;

out vec3 aCubeMapTexCoord;

uniform mat4 ciModelViewProjection;

void main() {
  aCubeMapTexCoord = normalize(ciPosition.xyz);
  gl_Position = ciModelViewProjection * ciPosition;
}
