#version 410

#define FLAP_SPEED 0.32

uniform int uGridSide;
uniform int uScreenWidth;
uniform int uScreenHeight;

uniform sampler2D uPositions;
uniform sampler2D uVelocities;

out vec4 FragColor;

void main() {
  vec2 texIndex = gl_FragCoord.xy / vec2(uGridSide, uGridSide);
  vec4 pos = texture(uPositions, texIndex);
  vec3 vel = texture(uVelocities, texIndex).xyz;

  // Make sure the position always stays normalized onto the sphere
  vec3 newPos = normalize(pos.xyz + vel);

  FragColor = vec4(newPos, pos.a + FLAP_SPEED);
}
