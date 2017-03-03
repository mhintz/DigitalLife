#version 410

in vec4 ciPosition;
in vec4 ciColor;
in vec2 ciTexCoord0;

out VertexData {
  vec4 gColor;
  vec2 gTexCoord0;
} vs_out;

uniform mat4 ciModelMatrix;

void main() {
  vs_out.gColor = ciColor;
  vs_out.gTexCoord0 = ciTexCoord0;
  gl_Position = ciModelMatrix * ciPosition;
}
