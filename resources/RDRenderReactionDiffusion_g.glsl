#version 410

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VertexData {
  vec3 gCubeMapTexCoord;
  vec3 gFaceCenter;
  int gFaceIndex;
} gs_in[];

out vec3 aCubeMapTexCoord;
out vec3 aFaceCenter;

void main() {
  gl_Layer = gs_in[0].gFaceIndex;

  for (int i = 0; i < gl_in.length(); i++) {
    aCubeMapTexCoord = gs_in[i].gCubeMapTexCoord;
    aFaceCenter = gs_in[i].gFaceCenter;
    gl_Position = gl_in[i].gl_Position;
    EmitVertex();
  }

  EndPrimitive();
}
