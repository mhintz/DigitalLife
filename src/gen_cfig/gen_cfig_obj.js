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

const MAGIC_ARC_LEN = 0.37;
const S_R = 0.5;

// tiers: upper, middle, lower
// When the sphere is sitting with the flat part down,
// upper is the upper tier of pins,
// middle is the pins at the slice,
// and lower is the pins "below" the slice, on the half with the cut through it
// all angles are relative to the slice itself
const MAGIC_ARC_IN_RADIANS = (MAGIC_ARC_LEN / PI) * TWO_PI;
const angle_to_upper = (PI / 2) - (MAGIC_ARC_IN_RADIANS);
const angle_to_middle = (PI / 2) - (2 * MAGIC_ARC_IN_RADIANS);
const angle_to_lower = angle_to_middle - MAGIC_ARC_IN_RADIANS; // below the slice so it should be a negative angle

const z_upper = sin(angle_to_upper) * S_R;
const r_upper = cos(angle_to_upper) * S_R;

const z_middle = sin(angle_to_middle) * S_R;
const r_middle = cos(angle_to_middle) * S_R;

const z_lower = sin(angle_to_lower) * S_R;
// The reason I divide by cos(PI / 6) here is because the bottom layer is not part of the sphere but
// actually a hexagonal base which "sticks out" from the base of the sphere.
// cos(angle_to_lower) * S_R computes a vertex for the bottom hexagon which is
// on the sphere, but the midpoint of any adjacent side will be inside the sphere.
// To change the hexagon size to circumscribe the sphere, I divide by the relative
// distance of the inner midpoint from the hexagon center, which is cos(PI / 6).
const r_lower = cos(angle_to_lower) * S_R / cos(PI / 6);

// top point
add_pos(m, 0, 0, S_R);

// upper tier
const UPPER_SEGMENTS = 8;
for (var i = 0; i < UPPER_SEGMENTS; i++) {
  var plane_angle = i * (TWO_PI / UPPER_SEGMENTS);
  add_pos(m, cos(plane_angle) * r_upper, sin(plane_angle) * r_upper, z_upper);
  if (i != 0) {
    add_face(m, [last_v(m), 0, last_v(m) - 1]);
  }
}

add_face(m, [last_v(m) - (UPPER_SEGMENTS - 1), 0, last_v(m)]);

// second row
const MIDDLE_SEGMENTS = 8;
for (var i = 0; i < MIDDLE_SEGMENTS; i++) {
  var plane_angle = i * (TWO_PI / MIDDLE_SEGMENTS);
  add_pos(m, cos(plane_angle) * r_middle, sin(plane_angle) * r_middle, z_middle);
  if (i != 0) {
    var top_row = last_v(m) - MIDDLE_SEGMENTS;
    add_face(m, [last_v(m), top_row, top_row - 1, last_v(m) - 1]);
  }
}

add_face(m, [last_v(m) - (MIDDLE_SEGMENTS - 1), last_v(m) - (MIDDLE_SEGMENTS - 1) - MIDDLE_SEGMENTS, last_v(m) - (MIDDLE_SEGMENTS - 1) - 1, last_v(m)]);

// third row
// This part is wack, I know, but it works. The change in topology at 3 is a method 
// for getting the six-piece bottom layer to match with the 8-piece middle one
const LOWER_SEGMENTS = 6;
for (var i = 0; i < LOWER_SEGMENTS; i++) {
  var plane_angle = i * (TWO_PI / LOWER_SEGMENTS);
  add_pos(m, cos(plane_angle) * r_lower, sin(plane_angle) * r_lower, z_lower);

  var top_forward = last_v(m) - (MIDDLE_SEGMENTS - 1) + (i > 3 ? 1 : 0);
  var top_back = top_forward - 1;
  add_face(m, [last_v(m), top_forward, top_back]);
  if (i != 0) {
    add_face(m, [last_v(m), top_back, last_v(m) - 1]);
  }

  if (i === 3) {
    add_face(m, [last_v(m), top_forward + 1, top_forward]);
  }
}

var first_bottom_v = last_v(m) - (LOWER_SEGMENTS - 1);
var top_forward = first_bottom_v - MIDDLE_SEGMENTS;
var top_back = first_bottom_v - 1;
add_face(m, [first_bottom_v, top_forward, top_back]);
add_face(m, [first_bottom_v, top_back, last_v(m)]);

fs.writeFileSync('installation_custom_adjusted_projector_sphere_cfig.obj', serializeOBJ(m.cells, m.positions));
