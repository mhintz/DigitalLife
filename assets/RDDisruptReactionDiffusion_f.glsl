#version 410

in highp vec3 CubeMapTexCoord;
flat in highp vec3 Xinc;
flat in highp vec3 Yinc;

uniform vec3 uDisruptionPoint;

out vec4 FragColor;

void main() {
  if (length(normalize(CubeMapTexCoord) - uDisruptionPoint) < 0.25) {
    FragColor = vec4(0, 1.0, 0, 1.0);
  } else {
    discard;
  }
}
