#version 410

#define BIRD_SIZE 0.01
#define WING_SIZE 0.025
#define PI 3.14159265359
#define MAX_FLAP_ANGLE PI * 0.25
#define FLAP_OFFSET PI * 0.12

layout(points, invocations = 6) in;
layout(triangle_strip, max_vertices = 4) out;

in VertexData {
  vec4 velocity;
  vec4 color;
  float wingPos;
} gs_in[];

layout(std140) uniform uMatrices {
  mat4 viewProjectionMatrix[6];
};

out vec4 aColor;

// Thanks GLM!!! http://glm.g-truc.net/0.9.8/api/a00169.html
mat3 rotation_matrix(vec3 inAxis, float angle) {
  vec3 axis = normalize(inAxis);
  float s = sin(angle);
  float c = cos(angle);
  vec3 temp = (1.0 - c) * axis;

  return mat3(c + temp.x * axis.x,            temp.x * axis.y + s * axis.z,   temp.x * axis.z - s * axis.y,
              temp.y * axis.x - s * axis.z,   c + temp.y * axis.y,            temp.y * axis.z + s * axis.x,
              temp.z * axis.x + s * axis.y,   temp.z * axis.y - s * axis.x,   c + temp.z * axis.z);
}

void main() {
  gl_Layer = gl_InvocationID;

  vec3 pos = normalize(gl_in[0].gl_Position.xyz);
  vec3 vel = normalize(gs_in[0].velocity.xyz);
  vec3 backVec = -vel;
  vec3 wingR = normalize(cross(pos, vel));
  vec3 wingL = -wingR;
  float wingAngle = sin(gs_in[0].wingPos) * MAX_FLAP_ANGLE + FLAP_OFFSET;

  // right wing
  mat3 angleRotationRight = rotation_matrix(pos, wingAngle);

  gl_Position = viewProjectionMatrix[gl_InvocationID] * vec4(pos + WING_SIZE * (angleRotationRight * wingR), 1);
  aColor = gs_in[0].color;
  EmitVertex();

  gl_Position = viewProjectionMatrix[gl_InvocationID] * vec4(pos + BIRD_SIZE * vel, 1);
  aColor = gs_in[0].color;
  EmitVertex();

  gl_Position = viewProjectionMatrix[gl_InvocationID] * vec4(pos + BIRD_SIZE * backVec, 1);
  aColor = gs_in[0].color;
  EmitVertex();

  // left wing
  mat3 angleRotationLeft = rotation_matrix(pos, -wingAngle);

  gl_Position = viewProjectionMatrix[gl_InvocationID] * vec4(pos + WING_SIZE * (angleRotationLeft * wingL), 1);
  aColor = gs_in[0].color;
  EmitVertex();

  EndPrimitive();
}
