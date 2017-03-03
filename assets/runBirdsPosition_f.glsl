#version 330

uniform int uGridSide;
uniform int uScreenWidth;
uniform int uScreenHeight;

uniform sampler2D uPositions;
uniform sampler2D uVelocities;

out vec4 FragColor;

void main() {
  vec2 texIndex = gl_FragCoord.xy / vec2(uGridSide, uGridSide);
  vec2 pos = texture(uPositions, texIndex).xy;
  vec2 vel = texture(uVelocities, texIndex).xy;

  vec2 newPos = pos + vel;

  // Apply bounds
  if (newPos.x < 0) { newPos.x = uScreenWidth; }
  if (newPos.y < 0) { newPos.y = uScreenHeight; }
  if (newPos.x > uScreenWidth) { newPos.x = 0; }
  if (newPos.y > uScreenHeight) { newPos.y = 0; }

  FragColor = vec4(newPos, 0, 1);
}
