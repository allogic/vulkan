#include <pch.h>

static void vdb_debug_lod(int32_t brick_index, int8_t lod);

vdb_t g_vdb = {0};

static int32_t const s_vdb_word_offset[7] = {
  0,
  8192,
  8192 + 1024,
  8192 + 1024 + 128,
  8192 + 1024 + 128 + 16,
  8192 + 1024 + 128 + 16 + 2,
  8192 + 1024 + 128 + 16 + 2 + 1,
};

static vector4_t const s_vdb_lod_color[7] = {
  {0.0F, 0.5F, 0.0F, 0.5F},
  {0.1F, 0.4F, 0.0F, 0.5F},
  {0.2F, 0.3F, 0.0F, 0.5F},
  {0.2F, 0.2F, 0.0F, 0.5F},
  {0.3F, 0.2F, 0.0F, 0.5F},
  {0.4F, 0.1F, 0.0F, 0.5F},
  {0.5F, 0.0F, 0.0F, 0.5F},
};

void vdb_create(ivector3_t dimension) {
  g_vdb.brick = (vdb_brick_t *)HEAP_ALLOC(VDB_INITIAL_TABLE_COUNT * sizeof(vdb_brick_t), 1, 0);
  g_vdb.brick_count = dimension.x * dimension.y * dimension.z;
  g_vdb.dimension = dimension;
  g_vdb.color = (vector4_t *)HEAP_ALLOC(sizeof(vector4_t) * 1000, 1, 0);

  // TODO: remove this
  for (int i = 0; i < 1000; i++) {
    g_vdb.color[i] = (vector4_t){(float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, 1.0F};
  }

  uint64_t mask_buffer_size_lod6 = 1;
  uint64_t mask_buffer_size_lod5 = 1;
  uint64_t mask_buffer_size_lod4 = 2;
  uint64_t mask_buffer_size_lod3 = 16;
  uint64_t mask_buffer_size_lod2 = 128;
  uint64_t mask_buffer_size_lod1 = 1024;
  uint64_t mask_buffer_size_lod0 = 8192;

  uint64_t mask_buffer_size =
    mask_buffer_size_lod6 +
    mask_buffer_size_lod5 +
    mask_buffer_size_lod4 +
    mask_buffer_size_lod3 +
    mask_buffer_size_lod2 +
    mask_buffer_size_lod1 +
    mask_buffer_size_lod0;

  for (int32_t x = 0; x < dimension.x; x++) {
    for (int32_t y = 0; y < dimension.y; y++) {
      for (int32_t z = 0; z < dimension.z; z++) {

        ivector3_t brick_position = {x, y, z};
        ivector3_t brick_position_scaled = ivector3_muls(brick_position, VDB_BASE_RES);

        int32_t brick_index = vec_to_index(brick_position, g_vdb.dimension);

        g_vdb.brick[brick_index].position = brick_position_scaled;
        g_vdb.brick[brick_index].mask_buffer = buffer_create_storage_coherent(0, sizeof(uint32_t) * mask_buffer_size); // TODO
      }
    }
  }
}
void vdb_debug(vector3_t ray_origin, vector3_t ray_direction) {
  int32_t brick_index = 0;
  int32_t brick_count = g_vdb.brick_count;

  while (brick_index < brick_count) {

    vdb_debug_lod(brick_index, 0);
    vdb_debug_lod(brick_index, 1);
    vdb_debug_lod(brick_index, 2);
    vdb_debug_lod(brick_index, 3);
    vdb_debug_lod(brick_index, 4);
    vdb_debug_lod(brick_index, 5);
    vdb_debug_lod(brick_index, 6);

    brick_index++;
  }
}
vdb_hit_t vdb_raymarch(vector3_t ray_origin, vector3_t ray_direction, float max_distance) {
  vdb_hit_t hit = {0};

  int32_t iter = 0;
  int32_t lod = VDB_MAX_LOD_LEVEL;
  int32_t voxels_per_axis = vdb_voxels_per_axis(lod);
  int32_t voxel_size = vdb_voxel_size(lod);

  vector3_t hit_position = ray_origin;
  vector3_t prev_hit_position = hit_position;

  ivector3_t step_direction = {
    (int32_t)fsignf(ray_direction.x),
    (int32_t)fsignf(ray_direction.y),
    (int32_t)fsignf(ray_direction.z),
  };
  ivector3_t brick_index = {
    (int32_t)floorf(hit_position.x / VDB_MAX_LOD_LEVEL),
    (int32_t)floorf(hit_position.y / VDB_MAX_LOD_LEVEL),
    (int32_t)floorf(hit_position.z / VDB_MAX_LOD_LEVEL),
  };
  ivector3_t prev_brick_index = brick_index;
  ivector3_t voxel_index = {
    (int32_t)floorf(hit_position.x / voxel_size),
    (int32_t)floorf(hit_position.y / voxel_size),
    (int32_t)floorf(hit_position.z / voxel_size),
  };
  ivector3_t prev_voxel_index = voxel_index;

  vector3_t next_boundary = {
    ((step_direction.x > 0) ? (float)(voxel_index.x + 1) : (float)voxel_index.x) * (float)voxel_size,
    ((step_direction.y > 0) ? (float)(voxel_index.y + 1) : (float)voxel_index.y) * (float)voxel_size,
    ((step_direction.z > 0) ? (float)(voxel_index.z + 1) : (float)voxel_index.z) * (float)voxel_size,
  };
  vector3_t ray_direction_inv = {
    (ray_direction.x == 0.0F) ? FLT_MAX : 1.0F / ray_direction.x,
    (ray_direction.y == 0.0F) ? FLT_MAX : 1.0F / ray_direction.y,
    (ray_direction.z == 0.0F) ? FLT_MAX : 1.0F / ray_direction.z,
  };
  vector3_t t_max = {
    (next_boundary.x - hit_position.x) * ray_direction_inv.x,
    (next_boundary.y - hit_position.y) * ray_direction_inv.y,
    (next_boundary.z - hit_position.z) * ray_direction_inv.z,
  };
  vector3_t t_delta = {
    fabsf(ray_direction_inv.x * (float)voxel_size),
    fabsf(ray_direction_inv.y * (float)voxel_size),
    fabsf(ray_direction_inv.z * (float)voxel_size),
  };

  float t = 0.0F;

  while (t < max_distance) {

    if (iter > 0) {
      renderer_draw_debug_line(prev_hit_position, hit_position, g_vdb.color[iter - 1]);
    }

    prev_hit_position = hit_position;
    prev_brick_index = brick_index;
    prev_voxel_index = voxel_index;

    int8_t in_bounds = (voxel_index.x >= 0) && (voxel_index.x < voxels_per_axis) &&
                       (voxel_index.y >= 0) && (voxel_index.y < voxels_per_axis) &&
                       (voxel_index.z >= 0) && (voxel_index.z < voxels_per_axis);

    if (in_bounds) {

      int8_t is_solid = vdb_voxel_is_solid(0, lod, voxel_index);

      if (is_solid) {

        if (lod > 0) {

          lod--;

          voxels_per_axis = vdb_voxels_per_axis(lod);
          voxel_size = vdb_voxel_size(lod);

          // TODO
          // brick_index = (ivector3_t){
          //   (int32_t)floorf(hit_position.x / VDB_MAX_LOD_LEVEL),
          //   (int32_t)floorf(hit_position.y / VDB_MAX_LOD_LEVEL),
          //   (int32_t)floorf(hit_position.z / VDB_MAX_LOD_LEVEL),
          // };
          voxel_index = (ivector3_t){
            (int32_t)floorf(hit_position.x / voxel_size),
            (int32_t)floorf(hit_position.y / voxel_size),
            (int32_t)floorf(hit_position.z / voxel_size),
          };

          next_boundary = (vector3_t){
            ((step_direction.x > 0) ? (float)(voxel_index.x + 1) : (float)voxel_index.x) * (float)voxel_size,
            ((step_direction.y > 0) ? (float)(voxel_index.y + 1) : (float)voxel_index.y) * (float)voxel_size,
            ((step_direction.z > 0) ? (float)(voxel_index.z + 1) : (float)voxel_index.z) * (float)voxel_size,
          };
          t_max = (vector3_t){
            (next_boundary.x - hit_position.x) * ray_direction_inv.x,
            (next_boundary.y - hit_position.y) * ray_direction_inv.y,
            (next_boundary.z - hit_position.z) * ray_direction_inv.z,
          };
          t_delta = (vector3_t){
            fabsf(ray_direction_inv.x * (float)voxel_size),
            fabsf(ray_direction_inv.y * (float)voxel_size),
            fabsf(ray_direction_inv.z * (float)voxel_size),
          };

          continue;
        }

        hit.intersect = 1;
        hit.brick_index = brick_index;
        hit.voxel_index = voxel_index;
        hit.hit_position = hit_position;

        return hit;
      }
    } else if (lod == VDB_MAX_LOD_LEVEL) {

      break;
    }

    renderer_draw_debug_box(
      (vector3_t){(float)prev_voxel_index.x * voxel_size, (float)prev_voxel_index.y * voxel_size, (float)prev_voxel_index.z * voxel_size},
      (vector3_t){(float)voxel_size, (float)voxel_size, (float)voxel_size},
      g_vdb.color[iter]);

    float t_voxel = fminf(t_max.x, fminf(t_max.y, t_max.z));
    float dist = t_voxel + EPSILON_6;

    if (t_voxel <= EPSILON_6) {
      t_voxel = 0.0F;
      dist = 0.0F;
    }

    if ((t + dist) >= max_distance) {
      break;
    }

    // assert(t_voxel >= -EPSILON_6);

    int32_t step_x = fabsf(t_max.x - t_voxel) < EPSILON_6;
    int32_t step_y = fabsf(t_max.y - t_voxel) < EPSILON_6;
    int32_t step_z = fabsf(t_max.z - t_voxel) < EPSILON_6;

    if (step_x) {
      voxel_index.x += step_direction.x;
      t_max.x += t_delta.x;
    }

    if (step_y) {
      voxel_index.y += step_direction.y;
      t_max.y += t_delta.y;
    }

    if (step_z) {
      voxel_index.z += step_direction.z;
      t_max.z += t_delta.z;
    }

    if (dist > 0.0F) {
      t += dist;
      hit_position = vector3_add(hit_position, vector3_muls(ray_direction, dist));

      // TODO
      // if ((iter % 100) == 0) {
      //   hit_position = vector3_add(ray_origin, vector3_muls(ray_direction, t));
      // }

      t_max.x -= dist;
      t_max.y -= dist;
      t_max.z -= dist;
    }

    t_max.x = fmaxf(t_max.x, 0.0F);
    t_max.y = fmaxf(t_max.y, 0.0F);
    t_max.z = fmaxf(t_max.z, 0.0F);

    if (!step_x && !step_y && !step_z && dist <= 0.0F) {
      break;
    }

    iter++;
  }

  return hit;
}
void vdb_destroy(void) {
  int32_t brick_index = 0;
  int32_t brick_count = g_vdb.brick_count;

  while (brick_index < brick_count) {

    buffer_destroy(&g_vdb.brick[brick_index].mask_buffer);

    brick_index++;
  }

  HEAP_FREE(g_vdb.brick);
}

int32_t vdb_voxels_per_axis(int8_t lod) {
  return VDB_BASE_RES >> lod;
}
int32_t vdb_total_voxel_count(int8_t lod) {
  int32_t n = vdb_voxels_per_axis(lod);

  return n * n * n;
}
int32_t vdb_word_count(int8_t lod) {
  int32_t n = vdb_total_voxel_count(lod);

  return (n + (VDB_BITS_PER_WORD - 1)) >> 5;
}
int32_t vdb_voxel_index(int8_t lod, ivector3_t index) {
  int32_t n = vdb_voxels_per_axis(lod);

  return index.x + index.y * n + index.z * n * n;
}
int32_t vdb_voxel_size(int8_t lod) {
  return 1 << lod;
}
int32_t vdb_word_index(int32_t voxel_index) {
  return voxel_index >> 5;
}

int8_t vdb_voxel_is_solid(int32_t brick_index, int8_t lod, ivector3_t index) {
  int32_t i = vdb_voxel_index(lod, index);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t bit = 1U << (i & 31);

  uint32_t *mask = (uint32_t *)g_vdb.brick[brick_index].mask_buffer.mapped_memory;

  return (mask[word] & bit) != 0u;
}

void vdb_voxel_set(int32_t brick_index, int8_t lod, ivector3_t index) {
  int32_t i = vdb_voxel_index(lod, index);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t bit = 1U << (i & 31);

  uint32_t *mask = (uint32_t *)g_vdb.brick[brick_index].mask_buffer.mapped_memory;

  mask[word] |= bit;
}
void vdb_voxel_clr(int32_t brick_index, int8_t lod, ivector3_t index) {
  int32_t i = vdb_voxel_index(lod, index);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t bit = ~(1U << (i & 31));

  uint32_t *mask = (uint32_t *)g_vdb.brick[brick_index].mask_buffer.mapped_memory;

  mask[word] &= bit;
}

static void vdb_debug_lod(int32_t brick_index, int8_t lod) {
  ivector3_t brick_position = g_vdb.brick[brick_index].position;

  int32_t voxel_count = vdb_voxels_per_axis(lod);
  int32_t voxel_size = vdb_voxel_size(lod);

  for (int32_t x = 0; x < voxel_count; x++) {
    for (int32_t y = 0; y < voxel_count; y++) {
      for (int32_t z = 0; z < voxel_count; z++) {

        ivector3_t voxel_position = {x, y, z};

        int8_t solid = vdb_voxel_is_solid(brick_index, lod, voxel_position);

        if (solid) {

          ivector3_t position = ivector3_add(ivector3_muls(voxel_position, voxel_size), brick_position);

          renderer_draw_debug_box(
            (vector3_t){(float)position.x, (float)position.y, (float)position.z},
            (vector3_t){(float)voxel_size, (float)voxel_size, (float)voxel_size},
            s_vdb_lod_color[lod]);
        }
      }
    }
  }
}
