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

static int32_t const s_vdb_voxel_count[7] = {
  1 * 1 * 1,
  2 * 2 * 2,
  4 * 4 * 4,
  8 * 8 * 8,
  16 * 16 * 16,
  32 * 32 * 32,
  64 * 64 * 64,
};

static int32_t const s_vdb_mask_buffer_size = 9364;

#if 0
static vector4_t const s_vdb_lod_color[7] = {
  {0.0F, 0.5F, 0.0F, 0.5F},
  {0.1F, 0.4F, 0.0F, 0.5F},
  {0.2F, 0.3F, 0.0F, 0.5F},
  {0.2F, 0.2F, 0.0F, 0.5F},
  {0.3F, 0.2F, 0.0F, 0.5F},
  {0.4F, 0.1F, 0.0F, 0.5F},
  {0.5F, 0.0F, 0.0F, 0.5F},
};
#endif

void vdb_create(ivector3_t dimension) {
  g_vdb.brick = (vdb_brick_t *)HEAP_ALLOC(VDB_INITIAL_TABLE_COUNT * sizeof(vdb_brick_t), 1, 0);
  g_vdb.brick_count = dimension.x * dimension.y * dimension.z;
  g_vdb.dimension = dimension;

  for (int32_t x = 0; x < dimension.x; x++) {
    for (int32_t y = 0; y < dimension.y; y++) {
      for (int32_t z = 0; z < dimension.z; z++) {

        ivector3_t brick_position = {x, y, z};
        ivector3_t brick_position_scaled = ivector3_muls(brick_position, VDB_BASE_RES);

        int32_t brick_index = vec_to_index(brick_position, g_vdb.dimension);

        g_vdb.brick[brick_index].position = brick_position_scaled;
        g_vdb.brick[brick_index].mask_buffer = buffer_create_storage(0, sizeof(uint32_t) * s_vdb_mask_buffer_size);
      }
    }
  }
}
void vdb_debug(vector3_t ray_origin, vector3_t ray_direction) {
  int32_t brick_index = 0;
  int32_t brick_count = g_vdb.brick_count;

  while (brick_index < brick_count) {

    // vdb_debug_lod(brick_index, 0);
    // vdb_debug_lod(brick_index, 1);
    // vdb_debug_lod(brick_index, 2);
    // vdb_debug_lod(brick_index, 3);
    // vdb_debug_lod(brick_index, 4);
    // vdb_debug_lod(brick_index, 5);
    vdb_debug_lod(brick_index, 6);

    brick_index++;
  }
}
vdb_hit_t vdb_raymarch(vector3_t ray_origin, vector3_t ray_direction, float max_distance, int32_t max_iteration) {
  vdb_hit_t hit = {0};
  vdb_lod_t lod_stack[VDB_MAX_LOD_LEVEL] = {0};

  int32_t iter = 0;
  int32_t lod = VDB_MAX_LOD_LEVEL;
  int32_t voxels_per_axis = vdb_voxels_per_axis(lod);
  int32_t voxel_size = vdb_voxel_size(lod);
  int32_t brick_index = -1;
  int32_t stack_depth = 0;

  vector3_t position = ray_origin;
  vector3_t prev_position = ray_origin;
  vector3_t ray_direction_inv = {
    (ray_direction.x == 0.0F) ? FLT_MAX : 1.0F / ray_direction.x,
    (ray_direction.y == 0.0F) ? FLT_MAX : 1.0F / ray_direction.y,
    (ray_direction.z == 0.0F) ? FLT_MAX : 1.0F / ray_direction.z,
  };
  vector3_t next_boundary;
  vector3_t t_max;

  ivector3_t step_direction = {
    (int32_t)fsignf(ray_direction.x),
    (int32_t)fsignf(ray_direction.y),
    (int32_t)fsignf(ray_direction.z),
  };
  ivector3_t prev_brick_position = {
    (int32_t)floorf(position.x / VDB_BASE_RES),
    (int32_t)floorf(position.y / VDB_BASE_RES),
    (int32_t)floorf(position.z / VDB_BASE_RES),
  };

  float t = 0.0F;

  while (t < max_distance && iter < max_iteration) {

#if 0
    if (iter > 0) {
      renderer_draw_debug_line(prev_position, position, g_vdb.color[iter - 1]);
    }
#endif

    prev_position = position;

    ivector3_t brick_position = {
      (int32_t)floorf(position.x / VDB_BASE_RES),
      (int32_t)floorf(position.y / VDB_BASE_RES),
      (int32_t)floorf(position.z / VDB_BASE_RES),
    };

    vector3_t brick_origin = {
      (float)brick_position.x * VDB_BASE_RES,
      (float)brick_position.y * VDB_BASE_RES,
      (float)brick_position.z * VDB_BASE_RES,
    };

    if (brick_position.x != prev_brick_position.x ||
        brick_position.y != prev_brick_position.y ||
        brick_position.z != prev_brick_position.z) {

      lod = VDB_MAX_LOD_LEVEL;
      voxels_per_axis = vdb_voxels_per_axis(lod);
      voxel_size = vdb_voxel_size(lod);
      stack_depth = 0;
    }

    prev_brick_position = brick_position;

    while (stack_depth > 0) {

      vdb_lod_t *parent = &lod_stack[stack_depth - 1];

      if (position.x >= parent->min_bounds.x && position.x < parent->max_bounds.x &&
          position.y >= parent->min_bounds.y && position.y < parent->max_bounds.y &&
          position.z >= parent->min_bounds.z && position.z < parent->max_bounds.z) {

        break;
      }

      lod = parent->lod;
      voxels_per_axis = vdb_voxels_per_axis(lod);
      voxel_size = vdb_voxel_size(lod);

      stack_depth--;
    }

    ivector3_t voxel_position = {
      (int32_t)floorf((position.x - brick_origin.x) / (float)voxel_size),
      (int32_t)floorf((position.y - brick_origin.y) / (float)voxel_size),
      (int32_t)floorf((position.z - brick_origin.z) / (float)voxel_size),
    };

    voxel_position.x = (int32_t)fmaxf(0.0F, fminf((float)(voxels_per_axis - 1), (float)voxel_position.x));
    voxel_position.y = (int32_t)fmaxf(0.0F, fminf((float)(voxels_per_axis - 1), (float)voxel_position.y));
    voxel_position.z = (int32_t)fmaxf(0.0F, fminf((float)(voxels_per_axis - 1), (float)voxel_position.z));

    int32_t in_brick = (brick_position.x >= 0 && brick_position.x < g_vdb.dimension.x &&
                        brick_position.y >= 0 && brick_position.y < g_vdb.dimension.y &&
                        brick_position.z >= 0 && brick_position.z < g_vdb.dimension.z);

    int32_t in_voxel = (voxel_position.x >= 0 && voxel_position.x < voxels_per_axis &&
                        voxel_position.y >= 0 && voxel_position.y < voxels_per_axis &&
                        voxel_position.z >= 0 && voxel_position.z < voxels_per_axis);

    brick_index = vec_to_index(brick_position, g_vdb.dimension);

    if (in_brick && in_voxel) {

      int8_t solid = vdb_voxel_is_solid(brick_index, lod, voxel_position);

      if (solid) {

        if (lod > 0) {

          if (stack_depth < VDB_MAX_LOD_LEVEL) {

            vector3_t current_min = {
              brick_origin.x + (float)voxel_position.x * (float)voxel_size,
              brick_origin.y + (float)voxel_position.y * (float)voxel_size,
              brick_origin.z + (float)voxel_position.z * (float)voxel_size,
            };
            vector3_t current_max = {
              current_min.x + (float)voxel_size,
              current_min.y + (float)voxel_size,
              current_min.z + (float)voxel_size,
            };

            lod_stack[stack_depth].lod = lod;
            lod_stack[stack_depth].min_bounds = current_min;
            lod_stack[stack_depth].max_bounds = current_max;

            stack_depth++;
          }

          lod--;

          voxels_per_axis = vdb_voxels_per_axis(lod);
          voxel_size = vdb_voxel_size(lod);

          continue;
        }

        hit.intersect = 1;
        hit.brick_position = brick_position;
        hit.voxel_position = voxel_position;
        hit.hit_position = position;

        return hit;
      }
    }

#if 0
    renderer_draw_debug_box(
      (vector3_t){brick_origin.x + (float)voxel_position.x * (float)voxel_size,
                  brick_origin.y + (float)voxel_position.y * (float)voxel_size,
                  brick_origin.z + (float)voxel_position.z * (float)voxel_size},
      (vector3_t){(float)voxel_size, (float)voxel_size, (float)voxel_size},
      g_vdb.color[iter]);
#endif

    next_boundary = (vector3_t){
      brick_origin.x + ((step_direction.x > 0) ? (float)(voxel_position.x + 1) : (float)voxel_position.x) * (float)voxel_size,
      brick_origin.y + ((step_direction.y > 0) ? (float)(voxel_position.y + 1) : (float)voxel_position.y) * (float)voxel_size,
      brick_origin.z + ((step_direction.z > 0) ? (float)(voxel_position.z + 1) : (float)voxel_position.z) * (float)voxel_size,
    };
    t_max = (vector3_t){
      fmaxf((next_boundary.x - position.x) * ray_direction_inv.x, 0.0F),
      fmaxf((next_boundary.y - position.y) * ray_direction_inv.y, 0.0F),
      fmaxf((next_boundary.z - position.z) * ray_direction_inv.z, 0.0F),
    };

    float t_exit = fminf(t_max.x, fminf(t_max.y, t_max.z));
    float dist = t_exit + EPSILON_6;

    if (t_exit <= 0.0F) {
      dist = EPSILON_4;
    }

    if ((t + dist) >= max_distance) {
      break;
    }

    t += dist;

    position.x += ray_direction.x * dist;
    position.y += ray_direction.y * dist;
    position.z += ray_direction.z * dist;

    if ((iter % 20) == 0) {
      position = vector3_add(ray_origin, vector3_muls(ray_direction, t));
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
  return s_vdb_voxel_count[lod];
}
int32_t vdb_word_count(int8_t lod) {
  return (s_vdb_voxel_count[lod] + (VDB_BITS_PER_WORD - 1)) >> 5;
}
int32_t vdb_voxel_index(int8_t lod, ivector3_t voxel_position) {
  switch (lod) {
    case 0: {
      return voxel_position.x + (voxel_position.y << 6) + (voxel_position.z << 12);
    }
    case 1: {
      return voxel_position.x + (voxel_position.y << 5) + (voxel_position.z << 10);
    }
    case 2: {
      return voxel_position.x + (voxel_position.y << 4) + (voxel_position.z << 8);
    }
    case 3: {
      return voxel_position.x + (voxel_position.y << 3) + (voxel_position.z << 6);
    }
    case 4: {
      return voxel_position.x + (voxel_position.y << 2) + (voxel_position.z << 4);
    }
    case 5: {
      return voxel_position.x + (voxel_position.y << 1) + (voxel_position.z << 2);
    }
    case 6: {
      return 0;
    }
  }

  return -1;
}
int32_t vdb_voxel_size(int8_t lod) {
  return 1 << lod;
}
int32_t vdb_word_index(int32_t voxel_index) {
  return voxel_index >> 5;
}

int8_t vdb_voxel_is_solid(int32_t brick_index, int8_t lod, ivector3_t voxel_position) {
  int32_t i = vdb_voxel_index(lod, voxel_position);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t *mask = (uint32_t *)g_vdb.brick[brick_index].mask_buffer.mapped_memory;

  return (mask[word] & (1U << (i & 31)));
}

void vdb_voxel_set(int32_t brick_index, int8_t lod, ivector3_t voxel_position) {
  int32_t i = vdb_voxel_index(lod, voxel_position);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t *mask = (uint32_t *)g_vdb.brick[brick_index].mask_buffer.mapped_memory;

  mask[word] |= (1U << (i & 31));
}
void vdb_voxel_clr(int32_t brick_index, int8_t lod, ivector3_t voxel_position) {
  int32_t i = vdb_voxel_index(lod, voxel_position);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t *mask = (uint32_t *)g_vdb.brick[brick_index].mask_buffer.mapped_memory;

  mask[word] &= ~(1U << (i & 31));
}

static void vdb_debug_lod(int32_t brick_index, int8_t lod) {
#if 0
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
#endif
}
