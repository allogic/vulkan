#ifndef VDB_H
#define VDB_H

#include "../math/constants.glsl"

#define VDB_MAX_LOD_LEVEL (0x6)
#define VDB_BASE_RES (0x40)
#define VDB_BITS_PER_WORD (0x20)

const int vdb_word_offset[7] = int[7](
  0,
  8192,
  8192 + 1024,
  8192 + 1024 + 128,
  8192 + 1024 + 128 + 16,
  8192 + 1024 + 128 + 16 + 2,
  8192 + 1024 + 128 + 16 + 2 + 1
);

struct vdb_hit_t {
  bool intersect;
  ivec3 position;
};

int vdb_voxels_per_axis(int lod) {
  return VDB_BASE_RES >> lod;
}
int vdb_total_voxel_count(int lod) {
  int n = vdb_voxels_per_axis(lod);

  return n * n * n;
}
int vdb_word_count(int lod) {
  int n = vdb_total_voxel_count(lod);

  return (n + (VDB_BITS_PER_WORD - 1)) >> 5;
}
int vdb_voxel_index(int lod, int x, int y, int z) {
  int n = vdb_voxels_per_axis(lod);

  return x + y * n + z * n * n;
}
int vdb_voxel_size(int lod) {
  return 1 << lod;
}

uint vdb_word_index(int voxel_index) {
  return voxel_index >> 5;
}
uint vdb_bit_mask(int voxel_index) {
  return 1u << (voxel_index & 31);
}

bool vdb_voxel_is_solid(int lod, int x, int y, int z) {
  int i = vdb_voxel_index(lod, x, y, z);

  uint word = vdb_word_offset[lod] + vdb_word_index(i);
  uint bit = vdb_bit_mask(i);

  return (vdb_brick.mask_buffer[word] & bit) != 0u;
}

void vdb_voxel_set(int lod, int x, int y, int z) {
  int i = vdb_voxel_index(lod, x, y, z);

  uint word = vdb_word_offset[lod] + vdb_word_index(i);
  uint bit = vdb_bit_mask(i);

  atomicOr(vdb_brick.mask_buffer[word], bit);
}
void vdb_voxel_clr(int lod, int x, int y, int z) {
  int i = vdb_voxel_index(lod, x, y, z);

  uint word = vdb_word_offset[lod] + vdb_word_index(i);
  uint bit = ~vdb_bit_mask(i);

  atomicAnd(vdb_brick.mask_buffer[word], bit);
}

vdb_hit_t vdb_hdda_raymarch(vec3 ray_origin, vec3 ray_direction, float max_distance) {
  vdb_hit_t hit;

  hit.intersect = false;
  hit.position = ivec3(0);

  // TODO
  int lod = 0; // VDB_MAX_LOD_LEVEL;

  float voxel_size = float(vdb_voxel_size(lod));
  int voxel_count = vdb_total_voxel_count(lod);

  int vx = int((ray_origin.x - (ray_direction.x < 0.0 ? EPSILON_4 : 0.0)) / voxel_size);
  int vy = int((ray_origin.y - (ray_direction.y < 0.0 ? EPSILON_4 : 0.0)) / voxel_size);
  int vz = int((ray_origin.z - (ray_direction.z < 0.0 ? EPSILON_4 : 0.0)) / voxel_size);

  int step_x = int(sign(ray_direction.x));
  int step_y = int(sign(ray_direction.y));
  int step_z = int(sign(ray_direction.z));

  float t_delta_x = (ray_direction.x != 0.0) ? voxel_size / abs(ray_direction.x) : FLT_MAX;
  float t_delta_y = (ray_direction.y != 0.0) ? voxel_size / abs(ray_direction.y) : FLT_MAX;
  float t_delta_z = (ray_direction.z != 0.0) ? voxel_size / abs(ray_direction.z) : FLT_MAX;

  float voxel_boundary_x = (step_x > 0 ? (vx + 1) : vx) * voxel_size;
  float voxel_boundary_y = (step_y > 0 ? (vy + 1) : vy) * voxel_size;
  float voxel_boundary_z = (step_z > 0 ? (vz + 1) : vz) * voxel_size;

  float t_max_x = (voxel_boundary_x - ray_origin.x) / ray_direction.x;
  float t_max_y = (voxel_boundary_y - ray_origin.y) / ray_direction.y;
  float t_max_z = (voxel_boundary_z - ray_origin.z) / ray_direction.z;

  t_max_x = (t_max_x < 0.0) ? 0.0 : t_max_x;
  t_max_y = (t_max_y < 0.0) ? 0.0 : t_max_y;
  t_max_z = (t_max_z < 0.0) ? 0.0 : t_max_z;

  float t = 0.0;

  while (t < max_distance) {

    if (vx < 0 || vy < 0 || vz < 0 || vx >= voxel_count || vy >= voxel_count || vz >= voxel_count) {
      break;
    }

    bool v = vdb_voxel_is_solid(lod, vx, vy, vz);

    if (v && lod > 0) {

      lod--;

      voxel_size = float(vdb_voxel_size(lod));
      voxel_count = vdb_total_voxel_count(lod);

      vx = int(((ray_origin.x - (ray_direction.x < 0.0 ? EPSILON_4 : 0.0)) / voxel_size));
      vy = int(((ray_origin.y - (ray_direction.y < 0.0 ? EPSILON_4 : 0.0)) / voxel_size));
      vz = int(((ray_origin.z - (ray_direction.z < 0.0 ? EPSILON_4 : 0.0)) / voxel_size));

      step_x = int(sign(ray_direction.x));
      step_y = int(sign(ray_direction.y));
      step_z = int(sign(ray_direction.z));

      t_delta_x = (ray_direction.x != 0.0) ? voxel_size / abs(ray_direction.x) : FLT_MAX;
      t_delta_y = (ray_direction.y != 0.0) ? voxel_size / abs(ray_direction.y) : FLT_MAX;
      t_delta_z = (ray_direction.z != 0.0) ? voxel_size / abs(ray_direction.z) : FLT_MAX;

      voxel_boundary_x = (step_x > 0 ? (vx + 1) : vx) * voxel_size;
      voxel_boundary_y = (step_y > 0 ? (vy + 1) : vy) * voxel_size;
      voxel_boundary_z = (step_z > 0 ? (vz + 1) : vz) * voxel_size;

      vec3 next_origin = ray_origin + ray_direction * t;

      t_max_x = (voxel_boundary_x - next_origin.x) / ray_direction.x;
      t_max_y = (voxel_boundary_y - next_origin.y) / ray_direction.y;
      t_max_z = (voxel_boundary_z - next_origin.z) / ray_direction.z;

      t_max_x = (t_max_x < 0.0) ? 0.0 : t_max_x;
      t_max_y = (t_max_y < 0.0) ? 0.0 : t_max_y;
      t_max_z = (t_max_z < 0.0) ? 0.0 : t_max_z;

      continue;
    }

    if (v && lod == 0) {

      hit.intersect = true;
      hit.position = ivec3(vx, vy, vz);

      return hit;
    }

    if (t_max_x < t_max_y) {
      if (t_max_x < t_max_z) {
        vx += step_x;
        t = t_max_x;
        t_max_x += t_delta_x;
      } else {
        vz += step_z;
        t = t_max_z;
        t_max_z += t_delta_z;
      }
    } else {
      if (t_max_y < t_max_z) {
        vy += step_y;
        t = t_max_y;
        t_max_y += t_delta_y;
      } else {
        vz += step_z;
        t = t_max_z;
        t_max_z += t_delta_z;
      }
    }
  }

  return hit;
}

#endif // VDB_H