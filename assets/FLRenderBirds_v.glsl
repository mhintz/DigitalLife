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
  vec3 position = texture(uBirdPositions, birdIndex).xyz;
  vec2 velocity = texture(uBirdVelocities, birdIndex).xy;
  gl_Position = vec4(position.xy, 0, 1);
  vs_out.velocity = vec4(normalize(velocity), 0, 0);
  vs_out.color = ciColor;
  vs_out.wingPos = position.z;
}
