#version 410

in vec4 ciPosition;
in vec3 ciTexCoord1;

out vec3 TexCoord0;

uniform mat4 ciModelViewProjection;

void main() {
  TexCoord0 = ciTexCoord1;
  gl_Position = ciModelViewProjection * ciPosition;
}
