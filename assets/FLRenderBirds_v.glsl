#version 410

in vec2 birdIndex;
in vec4 ciColor;

uniform sampler2D uBirdPositions;
uniform sampler2D uBirdVelocities;

out VertexData {
  vec4 velocity;
  vec4 color;
  float wingPos;
} vs_out;

void main() {
  vec4 position = texture(uBirdPositions, birdIndex);
  vec3 velocity = texture(uBirdVelocities, birdIndex).xyz;
  gl_Position = vec4(position.xyz, 1);
  vs_out.velocity = vec4(normalize(velocity), 0);
  vs_out.color = ciColor;
  vs_out.wingPos = position.a;
}
