#version 410

in highp vec3 CubeMapTexCoord;
flat in highp vec3 Xinc;
flat in highp vec3 Yinc;

uniform vec3 uDisruptionPoint;

out vec4 FragColor;

#define DISRUPT_RADIUS 0.45

void main() {
  if (length(normalize(CubeMapTexCoord) - uDisruptionPoint) < DISRUPT_RADIUS) {
    FragColor = vec4(0, 1.0, 0, 1.0);
  } else {
    discard;
  }
}
