#version 410

#define PI 3.14159265359
#define BIRD_SIZE 5.0
#define WING_SIZE 15.0
#define MAX_FLAP_ANGLE PI * 0.25
#define FLAP_OFFSET PI * 0.12

layout(points, invocations = 6) in;
// layout(triangle_strip, max_vertices = 4) out;
layout(triangle_strip, max_vertices = 3) out;

in VertexData {
  vec4 velocity;
  vec4 color;
  float wingPos;
} gs_in[];

layout(std140) uniform uMatrices {
  mat4 viewProjectionMatrix[6];
};

out vec4 aColor;

void main() {
  gl_Layer = gl_InvocationID;

  vec3 pos = gl_in[0].gl_Position.xyz;
  vec3 vel = gs_in[0].velocity.xyz;
  vec3 backVec = -vel;
  vec3 wingR = normalize(cross(pos, vel));
  vec3 wingL = normalize(cross(vel, pos));
  float wingAngle = sin(gs_in[0].wingPos) * MAX_FLAP_ANGLE + FLAP_OFFSET;
  float wingAngleSin = sin(wingAngle);
  float wingAngleCos = cos(wingAngle);

  // right wing
  // Eventually this needs to be a mat3 :/
  // mat2 angleRotationRight = mat2(wingAngleCos, wingAngleSin, -wingAngleSin, wingAngleCos);

  // gl_Position = viewProjectionMatrix[gl_InvocationID] * (gl_in[0].gl_Position + toV4(WING_SIZE * (angleRotationRight * wingR)));
  gl_Position = viewProjectionMatrix[gl_InvocationID] * vec4(pos + 0.005 * wingR, 1);
  aColor = gs_in[0].color;
  EmitVertex();

  // gl_Position = viewProjectionMatrix[gl_InvocationID] * (gl_in[0].gl_Position + toV4(BIRD_SIZE * backVec));
  gl_Position = viewProjectionMatrix[gl_InvocationID] * vec4(pos + 0.005 * wingL, 1);
  aColor = gs_in[0].color;
  EmitVertex();

  // gl_Position = viewProjectionMatrix[gl_InvocationID] * (gl_in[0].gl_Position + toV4(BIRD_SIZE * vel));
  gl_Position = viewProjectionMatrix[gl_InvocationID] * vec4(pos + 0.025 * vel, 1);
  aColor = gs_in[0].color;
  EmitVertex();

  // left wing
  // Eventually this needs to be a mat3 :/
  // mat2 angleRotationLeft = mat2(wingAngleCos, -wingAngleSin, wingAngleSin, wingAngleCos);

  // gl_Position = viewProjectionMatrix[gl_InvocationID] * (gl_in[0].gl_Position + toV4(WING_SIZE * (angleRotationLeft * wingL)));
  // aColor = gs_in[0].color;
  // EmitVertex();

  EndPrimitive();
}
