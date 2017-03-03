#version 330

in vec2 birdIndex;
in vec4 ciColor;

uniform sampler2D uBirdPositions;
uniform sampler2D uBirdVelocities;

out VertexData {
  vec4 velocity;
  vec4 color;
} vs_out;

void main() {
  vec2 position = texture(uBirdPositions, birdIndex).xy;
  vec2 velocity = texture(uBirdVelocities, birdIndex).xy;
  gl_Position = vec4(position, 0, 1);
  vs_out.velocity = vec4(normalize(velocity), 0, 0);
  vs_out.color = ciColor;
}
