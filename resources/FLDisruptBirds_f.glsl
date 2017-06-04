#version 410

uniform int uGridSide;

uniform sampler2D uPositions;
uniform sampler2D uVelocities;

uniform vec3 uDisruptPoint;

out vec4 FragColor; // new bird velocity

#define DISRUPT_RADIUS 0.35

vec3 setMag(vec3 v, float mag) {
  return normalize(v) * mag;
}

void main() {
  vec2 texIndex = gl_FragCoord.xy / uGridSide;
  vec3 pos = texture(uPositions, texIndex).xyz;
  vec3 vel = texture(uVelocities, texIndex).xyz;

  vec3 fleeVec = pos - uDisruptPoint;  

  if (length(fleeVec) < DISRUPT_RADIUS) {
    FragColor = vec4(setMag(fleeVec - (dot(fleeVec, pos) * normalize(pos)), length(vel)), 1);
  } else {
    FragColor = vec4(vel, 1);
  }
}
