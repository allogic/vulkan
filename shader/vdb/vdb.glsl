#ifndef VDB_H
#define VDB_H

#include "../math/constants.glsl"

#define VDB_BRICK_MAX_LOD_LEVEL (0x6)
#define VDB_BRICK_BASE_RES (0x40)
#define VDB_BITS_PER_WORD (0x20)

const int vdb_brick_word_offset[7] = int[7](
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

int vdb_brick_voxels_per_axis(int lod) {
  return VDB_BRICK_BASE_RES >> lod;
}
int vdb_brick_total_voxel_count(int lod) {
  int n = vdb_brick_voxels_per_axis(lod);

  return n * n * n;
}
int vdb_brick_word_count(int lod) {
  int n = vdb_brick_total_voxel_count(lod);

  return (n + (VDB_BITS_PER_WORD - 1)) >> 5;
}
int vdb_brick_voxel_index(int lod, int x, int y, int z) {
  int n = vdb_brick_voxels_per_axis(lod);

  return x + y * n + z * n * n;
}
int vdb_brick_voxel_size(int lod) {
  return 1 << lod;
}

uint vdb_brick_word_index(int voxel_index) {
  return voxel_index >> 5;
}
uint vdb_brick_bit_mask(int voxel_index) {
  return 1u << (voxel_index & 31);
}

bool vdb_brick_voxel_is_solid(int lod, int x, int y, int z) {
  int i = vdb_brick_voxel_index(lod, x, y, z);

  uint word = vdb_brick_word_offset[lod] + vdb_brick_word_index(i);
  uint bit = vdb_brick_bit_mask(i);

  return (vdb_brick.mask_buffer[word] & bit) != 0u;
}

void vdb_brick_set(int lod, int x, int y, int z) {
  int i = vdb_brick_voxel_index(lod, x, y, z);

  uint word = vdb_brick_word_offset[lod] + vdb_brick_word_index(i);
  uint bit = vdb_brick_bit_mask(i);

  atomicOr(vdb_brick.mask_buffer[word], bit);
}
void vdb_brick_clr(int lod, int x, int y, int z) {
  int i = vdb_brick_voxel_index(lod, x, y, z);

  uint word = vdb_brick_word_offset[lod] + vdb_brick_word_index(i);
  uint bit = ~vdb_brick_bit_mask(i);

  atomicAnd(vdb_brick.mask_buffer[word], bit);
}

/*
int vdb_brick_voxel_count(int lod) {
  switch (lod) {
    case 0: return VDB_BRICK_VOXEL_COUNT_LOD0;
    case 1: return VDB_BRICK_VOXEL_COUNT_LOD1;
    case 2: return VDB_BRICK_VOXEL_COUNT_LOD2;
    case 3: return VDB_BRICK_VOXEL_COUNT_LOD3;
    case 4: return VDB_BRICK_VOXEL_COUNT_LOD4;
    case 5: return VDB_BRICK_VOXEL_COUNT_LOD5;
    case 6: return VDB_BRICK_VOXEL_COUNT_LOD6;
  }

  return -1;
}
int vdb_brick_voxel_size(int lod) {
  return 1 << lod;
}
int vdb_brick_voxel_index(int lod, int x, int y, int z) {
  switch (lod) {
    case 0: return x + (y << 6) + (z << 12);
    case 1: return x + (y << 5) + (z << 10);
    case 2: return x + (y << 4) + (z << 8);
    case 3: return x + (y << 3) + (z << 6);
    case 4: return x + (y << 2) + (z << 4);
    case 5: return x + (y << 1) + (z << 2);
    case 6: return 0;
  }

  return -1;
}

bool vdb_brick_voxel_is_solid(int lod, int x, int y, int z) {
  int i = vdb_brick_voxel_index(lod, x, y, z);

  switch (lod) {
    case 0: return bool((vdb_brick.mask_buffer[vdb_brick_mask_offset_lod0 + (i >> 5)] >> (i & 31)) & 1);
    case 1: return bool((vdb_brick.mask_buffer[vdb_brick_mask_offset_lod1 + (i >> 5)] >> (i & 31)) & 1);
    case 2: return bool((vdb_brick.mask_buffer[vdb_brick_mask_offset_lod2 + (i >> 5)] >> (i & 31)) & 1);
    case 3: return bool((vdb_brick.mask_buffer[vdb_brick_mask_offset_lod3 + (i >> 5)] >> (i & 31)) & 1);
    case 4: return bool((vdb_brick.mask_buffer[vdb_brick_mask_offset_lod4 + (i >> 5)] >> (i & 31)) & 1);
    case 5: return bool((vdb_brick.mask_buffer[vdb_brick_mask_offset_lod5           ] >>  i      ) & 1);
    case 6: return bool((vdb_brick.mask_buffer[vdb_brick_mask_offset_lod6           ]            ) & 1);
  }

  return false;
}

void vdb_brick_set(int lod, int x, int y, int z) {
  int i = vdb_brick_voxel_index(lod, x, y, z);

  switch (lod) {
    case 0: atomicOr(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod0 + (i >> 5)], (1u << (i & 31))); break;
    case 1: atomicOr(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod1 + (i >> 5)], (1u << (i & 31))); break;
    case 2: atomicOr(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod2 + (i >> 5)], (1u << (i & 31))); break;
    case 3: atomicOr(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod3 + (i >> 5)], (1u << (i & 31))); break;
    case 4: atomicOr(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod4 + (i >> 5)], (1u << (i & 31))); break;
    case 5: atomicOr(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod5           ], (1u << (i & 31))); break;
    case 6: atomicOr(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod6           ],  1u             ); break;
  }
}
void vdb_brick_clr(int lod, int x, int y, int z) {
  int i = vdb_brick_voxel_index(lod, x, y, z);

  switch (lod) {
    case 0: atomicAnd(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod0 + (i >> 5)], ~(1u << (i & 31))); break;
    case 1: atomicAnd(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod1 + (i >> 5)], ~(1u << (i & 31))); break;
    case 2: atomicAnd(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod2 + (i >> 5)], ~(1u << (i & 31))); break;
    case 3: atomicAnd(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod3 + (i >> 5)], ~(1u << (i & 31))); break;
    case 4: atomicAnd(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod4 + (i >> 5)], ~(1u << (i & 31))); break;
    case 5: atomicAnd(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod5           ], ~(1u << (i & 31))); break;
    case 6: atomicAnd(vdb_brick.mask_buffer[vdb_brick_mask_offset_lod6           ], ~(1u            )); break;
  }
}
*/

vdb_hit_t vdb_hdda_raymarch(vec3 ray_origin, vec3 ray_direction, float max_distance) {
  vdb_hit_t hit;

  hit.intersect = false;
  hit.position = ivec3(0);

  // TODO
  int lod = 0; // VDB_BRICK_MAX_LOD_LEVEL;

  float voxel_size = float(vdb_brick_voxel_size(lod));
  int voxel_count = vdb_brick_total_voxel_count(lod);

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

    bool v = vdb_brick_voxel_is_solid(lod, vx, vy, vz);

    if (v && lod > 0) {

      lod--;

      voxel_size = float(vdb_brick_voxel_size(lod));
      voxel_count = vdb_brick_total_voxel_count(lod);

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