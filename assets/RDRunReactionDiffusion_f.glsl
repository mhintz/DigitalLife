#version 410

in highp vec3 CubeMapTexCoord;
flat in highp vec3 Xinc;
flat in highp vec3 Yinc;

uniform samplerCube uPrevFrame;

out vec4 FragColor;

float convoluteA(float ul, float u, float ur, float l, float c, float r, float bl, float b, float br) {
  return (
    0.05 * ul +
    0.2 * u +
    0.05 * ur +
    0.2 * l +
    -1. * c +
    0.2 * r +
    0.05 * bl +
    0.2 * b +
    0.05 * br
  );
}

float convoluteB(float ul, float u, float ur, float l, float c, float r, float bl, float b, float br) {
  return (
    0.05 * ul +
    0.2 * u +
    0.05 * ur +
    0.2 * l +
    -1. * c +
    0.2 * r +
    0.05 * bl +
    0.2 * b +
    0.05 * br
  );
}

const float diffusionRateA = 1.0;
const float diffusionRateB = 0.5;

uniform float feedRateA;
uniform float killRateB;

// To make stuff get kind weird:
// const float diffusionRateA = 0.4;
// const float diffusionRateB = 0.05;

float max3(vec3 val) {
  return max(max(val.x, val.y), val.z);
}

void main() {
  vec4 ul = texture(uPrevFrame, CubeMapTexCoord - Xinc + Yinc);
  vec4 u = texture(uPrevFrame, CubeMapTexCoord + Yinc);
  vec4 ur = texture(uPrevFrame, CubeMapTexCoord + Xinc + Yinc);
  vec4 l = texture(uPrevFrame, CubeMapTexCoord - Xinc);
  vec4 cur = texture(uPrevFrame, CubeMapTexCoord);
  vec4 r = texture(uPrevFrame, CubeMapTexCoord + Xinc);
  vec4 bl = texture(uPrevFrame, CubeMapTexCoord - Xinc - Yinc);
  vec4 b = texture(uPrevFrame, CubeMapTexCoord - Yinc);
  vec4 br = texture(uPrevFrame, CubeMapTexCoord + Xinc - Yinc);

  float curA = cur.g;
  float curB = cur.b;

  float ABB = curA * curB * curB;

  float diffA = diffusionRateA * convoluteA(ul.g, u.g, ur.g, l.g, curA, r.g, bl.g, b.g, br.g);
  float diffB = diffusionRateB * convoluteB(ul.b, u.b, ur.b, l.b, curB, r.b, bl.b, b.b, br.b);

  float newA = curA + (diffA - ABB + feedRateA * (1.0 - curA));
  float newB = curB + (diffB + ABB - (feedRateA + killRateB) * curB);

  // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  // FragColor = texture(uPrevFrame, CubeMapTexCoord);
  // FragColor = vec4(CubeMapTexCoord, 1);
  // FragColor = vec4(normalize(CubeMapTexCoord), 1);
  FragColor = vec4(0.0, newA, newB, 1.0);
}
