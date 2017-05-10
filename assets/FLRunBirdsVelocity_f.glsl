#version 410

uniform int uGridSide;

uniform sampler2D uPositions;
uniform sampler2D uVelocities;

out vec4 FragColor;

#define SELF_EPSILON 0.0000001

uniform float uMinSpeed;
uniform float uMaxSpeed;

uniform float uMinForce;
uniform float uMaxForce;

uniform float uSeparationDist;
uniform float uSeparationMod;
uniform float uAlignDist;
uniform float uAlignMod;
uniform float uCohesionDist;
uniform float uCohesionMod;

vec3 limit(vec3 v, float lo, float hi) {
  float len = length(v);
  return max(lo, min(len, hi)) * normalize(v);
}

vec3 flockAccel(in vec3 selfPos, in vec3 selfVel) {
  vec3 sepSteer = vec3(0);
  int separationNeighbors = 0;

  vec3 alignSteer = vec3(0);
  int alignNeighbors = 0;

  vec3 cohesionPosition = vec3(0);
  int cohesionNeighbors = 0;

  for (float x = 0; x < 1.0; x += 1.0 / uGridSide) {
    for (float y = 0; y < 1.0; y += 1.0 / uGridSide) {
      vec2 texUV = vec2(x, y);
      vec3 otherPos = texture(uPositions, texUV).xyz;
      vec3 otherVel = texture(uVelocities, texUV).xyz;
      float dist = length(otherPos - selfPos);

      float isOther = float(dist > SELF_EPSILON);

      bool withinSeparationDist = dist < isOther * uSeparationDist;
      vec3 fromOther = normalize(selfPos - otherPos) / dist;
      sepSteer += float(withinSeparationDist) * fromOther;
      separationNeighbors += int(withinSeparationDist) * 1;

      bool withinAlignmentDist = dist < isOther * uAlignDist;
      alignSteer += float(withinAlignmentDist) * otherVel;
      alignNeighbors += int(withinAlignmentDist) * 1;

      bool withinCohesionDist = dist < isOther * uCohesionDist;
      cohesionPosition += float(withinCohesionDist) * otherPos;
      cohesionNeighbors += int(withinCohesionDist) * 1;
    }
  }

  if (separationNeighbors > 0) {
    sepSteer /= float(separationNeighbors);
    if (length(sepSteer) > 0) {
      sepSteer = normalize(sepSteer) - selfVel;
      sepSteer = limit(sepSteer, uMinForce, uMaxForce);
    }
  }

  if (alignNeighbors > 0) {
    alignSteer /= float(alignNeighbors);
    alignSteer = normalize(alignSteer) - selfVel;
    alignSteer = limit(alignSteer, uMinForce, uMaxForce);
  }

  vec3 cohesionSteer = vec3(0);
  if (cohesionNeighbors > 0) {
    cohesionPosition /= float(cohesionNeighbors);
    cohesionSteer = normalize(cohesionPosition - selfPos) - selfVel;
    cohesionSteer = limit(cohesionSteer, uMinForce, uMaxForce);
  }

  sepSteer *= uSeparationMod;
  alignSteer *= uAlignMod;
  cohesionSteer *= uCohesionMod;

  return sepSteer + alignSteer + cohesionSteer;
}

void main() {
  vec2 texIndex = gl_FragCoord.xy / uGridSide;
  vec3 pos = texture(uPositions, texIndex).xyz;
  vec3 vel = texture(uVelocities, texIndex).xyz;

  vec3 acc = flockAccel(pos, vel);

  vel = vel + acc;
  // Project the velocity so it's tangent to the sphere
  vel = vel - (dot(vel, pos) * normalize(pos));
  vel = limit(vel, uMinSpeed, uMaxSpeed);

  FragColor = vec4(vel, 1);
}
