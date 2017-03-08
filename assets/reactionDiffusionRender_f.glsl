#version 410

in highp vec3 aCubeMapTexCoord;
in vec3 aFaceCenter;

uniform samplerCube uGridSampler;

out vec4 FragColor;

#define NUM_COLOR_STOPS 5

vec3[NUM_COLOR_STOPS] colors = vec3[](
  vec3(0.0, 0.0, 0.0), // black
  vec3(1.0, 1.0, 0.702), // yellow
  // vec3(0.011, 0.427, 0.407), // dark turquoise
  vec3(0.188, 0.835, 0.784), // turquoise
  vec3(0, 0, 0.515), // blue
  vec3(1.0, 1.0, 1.0) // white
);

float[NUM_COLOR_STOPS] stops = float[](
  0.0,
  0.09,
  0.19,
  0.45,
  1.0
);

vec3 interpColorScheme(float t) {
  vec3 color = colors[0];
  for (int idx = 1; idx < stops.length(); idx++) {
    color = mix(color, colors[idx], smoothstep(stops[idx - 1], stops[idx], t));
  }
  return color;
}

float max3(vec3 x) {
  return max(x.x, max(x.y, x.z));
}

vec3 projectToCubeMapFace(vec3 cmapCoord) {
  return cmapCoord / max3(cmapCoord * sign(cmapCoord));
}

void main() {
  vec3 normalizedCMCoord = normalize(aCubeMapTexCoord);
  vec3 projectedCMCoord = projectToCubeMapFace(normalizedCMCoord);

  vec3 fromCenter = projectedCMCoord - aFaceCenter;
  vec3 centerAngles = atan(abs(fromCenter));
  vec3 adjustedCoord = projectedCMCoord + ((1 + cos(4 * centerAngles)) / 32) * (1 - abs(aFaceCenter)) * normalize(fromCenter);

  vec4 gridValues = texture(uGridSampler, adjustedCoord);
  float B = gridValues.b;

  FragColor = vec4(interpColorScheme(B), 1.0);
}
