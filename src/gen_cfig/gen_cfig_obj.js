var serializeOBJ = require('serialize-wavefront-obj');
var fs = require('fs');

function to_vec(x, y, z) { return [x, y, z]; }

function mag_sq(v) {
  var x = v[0];
  var y = v[1];
  var z = v[2];
  return x * x + y * y + z * z;
}

function mag(v) { return Math.sqrt(mag_sq(v)); }

function add_pos(mesh, x, y, z) { return mesh.positions.push(to_vec(x, y, z)); }
function add_face(mesh, f) { return mesh.cells.push(f); }
function verts(mesh) { return mesh.positions.length; }
function last_v(mesh) { return verts(mesh) - 1; }

// In this coordinate system is right-handed, with z pointing "up" (like Blender)

var m = {
  positions: [],
  cells: [],
};

const PI = Math.PI;
const TWO_PI = 2 * PI;
const sin = Math.sin;
const cos = Math.cos;

const MAGIC_ARC_LEN = 37;
const S_R = 0.5;

const angle_a = (MAGIC_ARC_LEN / (100 * PI)) * TWO_PI;
const angle_b = (2 * MAGIC_ARC_LEN / (100 * PI)) * TWO_PI;
const angle_c = PI - angle_a - angle_b;

const Z3 = -cos(angle_a) * S_R;
const R3 = sin(angle_a) * S_R;
const Z2 = -cos(angle_b) * S_R;
const R2 = sin(angle_b) * S_R;
const Z1 = cos(angle_c) * S_R;
const R1 = sin(angle_c) * S_R;

const SEGMENTS = 8;
const SEG_ANGLE = 2 * Math.PI / SEGMENTS;

// top point
add_pos(m, 0, 0, S_R);

// first row
for (var i = 0; i < SEGMENTS; i++) {
  var angle = i * SEG_ANGLE;
  add_pos(m, cos(angle) * R1, sin(angle) * R1, Z1);
  if (i != 0) {
    add_face(m, [last_v(m), 0, last_v(m) - 1]);
  }
}

add_face(m, [last_v(m) - (SEGMENTS - 1), 0, last_v(m)]);

// second row
for (var i = 0; i < SEGMENTS; i++) {
  var angle = i * SEG_ANGLE;
  add_pos(m, cos(angle) * R2, sin(angle) * R2, Z2);
  if (i != 0) {
    var top_row = last_v(m) - SEGMENTS;
    add_face(m, [last_v(m), top_row, top_row - 1, last_v(m) - 1]);
  }
}

add_face(m, [last_v(m) - (SEGMENTS - 1), last_v(m) - (SEGMENTS - 1) - SEGMENTS, last_v(m) - (SEGMENTS - 1) - 1, last_v(m)]);

// third row
for (var i = 0; i < SEGMENTS; i++) {
  var angle = i * SEG_ANGLE;
  add_pos(m, cos(angle) * R3, sin(angle) * R3, Z3);
  if (i != 0) {
    var top_row = last_v(m) - SEGMENTS;
    add_face(m, [last_v(m), top_row, top_row - 1, last_v(m) - 1]);
  }
}

add_face(m, [last_v(m) - (SEGMENTS - 1), last_v(m) - (SEGMENTS - 1) - SEGMENTS, last_v(m) - (SEGMENTS - 1) - 1, last_v(m)]);

// last point

add_pos(m, 0, 0, -S_R);

for (var i = 1; i < SEGMENTS; i++) {
  add_face(m, [last_v(m), last_v(m) - i, last_v(m) - i - 1]);
}

add_face(m, [last_v(m), last_v(m) - SEGMENTS, last_v(m) - 1])

fs.writeFileSync('installation_custom_adjusted_projector_sphere_cfig.obj', serializeOBJ(m.cells, m.positions));
