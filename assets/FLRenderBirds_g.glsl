#version 410

#define PI 3.14159265359
#define BIRD_SIZE 5.0
#define WING_SIZE 15.0
#define MAX_FLAP_ANGLE PI * 0.25
#define FLAP_OFFSET PI * 0.12

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VertexData {
  vec4 velocity;
  vec4 color;
  float wingPos;
} gs_in[];

uniform mat4 ciModelViewProjection;

out vec4 fColor;

vec4 toV4(in vec2 v) { return vec4(v, 0, 0); }

void main() {
  vec2 vel = gs_in[0].velocity.xy;
  vec2 backVec = -vel;
  vec2 wingR = vec2(-vel.y, vel.x);
  vec2 wingL = vec2(vel.y, -vel.x);
  float wingAngle = sin(gs_in[0].wingPos) * MAX_FLAP_ANGLE + FLAP_OFFSET;
  float wingAngleSin = sin(wingAngle);
  float wingAngleCos = cos(wingAngle);

  // right wing
  mat2 angleRotationRight = mat2(wingAngleCos, wingAngleSin, -wingAngleSin, wingAngleCos);

  gl_Position = ciModelViewProjection * (gl_in[0].gl_Position + toV4(WING_SIZE * (angleRotationRight * wingR)));
  fColor = gs_in[0].color;
  EmitVertex();

  gl_Position = ciModelViewProjection * (gl_in[0].gl_Position + toV4(BIRD_SIZE * backVec));
  fColor = gs_in[0].color;
  EmitVertex();

  gl_Position = ciModelViewProjection * (gl_in[0].gl_Position + toV4(BIRD_SIZE * vel));
  fColor = gs_in[0].color;
  EmitVertex();

  // left wing
  mat2 angleRotationLeft = mat2(wingAngleCos, -wingAngleSin, wingAngleSin, wingAngleCos);

  gl_Position = ciModelViewProjection * (gl_in[0].gl_Position + toV4(WING_SIZE * (angleRotationLeft * wingL)));
  fColor = gs_in[0].color;
  EmitVertex();

  EndPrimitive();
}
