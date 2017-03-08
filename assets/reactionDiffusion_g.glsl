#version 410

layout (points, invocations = 6) in;
layout (triangle_strip, max_vertices = 4) out;

uniform int gridSideLength;

out highp vec3 CubeMapTexCoord;
flat out highp vec3 Xinc;
flat out highp vec3 Yinc;

vec4[4] rectCorners = vec4[](
  vec4(-1, 1, 0, 1),
  vec4(-1, -1, 0, 1),
  vec4(1, 1, 0, 1),
  vec4(1, -1, 0, 1)
);

vec2[4] rectTex = vec2[](
  vec2(-1, 1),
  vec2(-1, -1),
  vec2(1, 1),
  vec2(1, -1)
);

vec3[6] horIncs = vec3[](
  vec3(0, 0, -1),
  vec3(0, 0, 1),
  vec3(1, 0, 0),
  vec3(1, 0, 0),
  vec3(1, 0, 0),
  vec3(-1, 0, 0)
);

vec3[6] vertIncs = vec3[](
  vec3(0, 1, 0),
  vec3(0, 1, 0),
  vec3(0, 0, -1),
  vec3(0, 0, 1),
  vec3(0, 1, 0),
  vec3(0, 1, 0)
);

void main() {
  // Render into the correct cube map face
  gl_Layer = gl_InvocationID;

  for (int idx = 0; idx < 4; idx++) {
    gl_Position = rectCorners[idx];

    vec2 tex2d = rectTex[idx];

    vec3[6] cubeMapCoords = vec3[](
      vec3(1, -tex2d.t, -tex2d.s), // positive X
      vec3(-1, -tex2d.t, tex2d.s), // negative X
      vec3(tex2d.s, 1, tex2d.t), // positive Y
      vec3(tex2d.s, -1, -tex2d.t), // negative Y
      vec3(tex2d.s, -tex2d.t, 1), // positive Z
      vec3(-tex2d.s, -tex2d.t, -1) // negative Z
    );

    CubeMapTexCoord = normalize(cubeMapCoords[gl_InvocationID]);
    Xinc = horIncs[gl_InvocationID] / gridSideLength;
    Yinc = vertIncs[gl_InvocationID] / gridSideLength;

    EmitVertex();
  }

  EndPrimitive();
}
