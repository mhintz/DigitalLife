#version 410

in vec4 ciPosition;
in vec3 ciTexCoord0;
in int aFaceIndex;

out VertexData {
  int gFaceIndex;
  vec3 gCubeMapTexCoord;
  vec3 gFaceCenter;
} vs_out;

#define NUM_SIDES 6

vec3[NUM_SIDES] faceCenters = vec3[](
  vec3(1, 0, 0),
  vec3(-1, 0, 0),
  vec3(0, 1, 0),
  vec3(0, -1, 0),
  vec3(0, 0, 1),
  vec3(0, 0, -1)
);

void main() {
  vs_out.gFaceIndex = aFaceIndex;
  vs_out.gCubeMapTexCoord = ciTexCoord0;
  vs_out.gFaceCenter = faceCenters[aFaceIndex];
  gl_Position = ciPosition;
}
