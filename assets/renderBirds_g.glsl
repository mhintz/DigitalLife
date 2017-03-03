#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

in VertexData {
  vec4 velocity;
  vec4 color;
} gs_in[];

uniform mat4 ciModelViewProjection;
uniform float uBirdSize;

out vec4 fColor;

void main() {
  vec4 vel = uBirdSize * gs_in[0].velocity;

  gl_Position = ciModelViewProjection * (gl_in[0].gl_Position + vel);
  fColor = gs_in[0].color;
  EmitVertex();

  vec4 backVec = -vel;
  vec4 backPerp = 0.5 * vec4(-backVec.y, backVec.x, 0, 0);

  gl_Position = ciModelViewProjection * (gl_in[0].gl_Position + backVec - backPerp);
  fColor = gs_in[0].color;
  EmitVertex();

  gl_Position = ciModelViewProjection * (gl_in[0].gl_Position + backVec + backPerp);
  fColor = gs_in[0].color;
  EmitVertex();

  EndPrimitive();
}
