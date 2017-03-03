#version 330

in vec4 ciPosition;
in vec3 ciTexCoord0;

out vec3 TexCoord0;

uniform mat4 ciModelViewProjection;

void main() {
  TexCoord0 = ciTexCoord0;
  gl_Position = ciModelViewProjection * ciPosition;
}
