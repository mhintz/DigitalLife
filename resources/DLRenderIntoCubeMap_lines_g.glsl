#version 410

layout (lines, invocations = 6) in;
layout (line_strip, max_vertices = 2) out;

in VertexData {
  vec4 gColor;
  vec2 gTexCoord0;
} gs_in[];

out vec4 aColor;
out vec2 aTexCoord0;

layout(std140) uniform uMatrices {
  mat4 viewProjectionMatrix[6];
};

void main() {
  gl_Layer = gl_InvocationID;

  for (int i = 0; i < gl_in.length(); i++) {
    aColor = gs_in[i].gColor;
    aTexCoord0 = gs_in[i].gTexCoord0;
    gl_Position = viewProjectionMatrix[gl_InvocationID] * gl_in[i].gl_Position;
    EmitVertex();
  }

  EndPrimitive();
}

