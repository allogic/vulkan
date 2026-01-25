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
  g_vdb.bricks = (vdb_brick_t *)HEAP_ALLOC(VDB_INITIAL_TABLE_COUNT * sizeof(vdb_brick_t), 1, 0);
  g_vdb.brick_count = dimension.x * dimension.y * dimension.z;
  g_vdb.dimension = dimension;

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

        int32_t brick_index = vec_to_index(brick_position, g_vdb.dimension);

        g_vdb.bricks[brick_index].position = brick_position;
        g_vdb.bricks[brick_index].mask_buffer = buffer_create_storage_coherent(0, sizeof(uint32_t) * mask_buffer_size); // TODO
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

  vdb_raymarch(ray_origin, ray_direction, 1000.0F);
}
vdb_hit_t vdb_raymarch(vector3_t ray_origin, vector3_t ray_direction, float max_distance) {
  vdb_hit_t hit = {
    .ao = 1.0F,
  };

  vector3_t prev_position = ray_origin; // TODO: remove me

  int32_t lod = 0; // VDB_MAX_LOD_LEVEL;
  int32_t hit_axis = -1;
  int32_t brick_index = 0;

  float voxel_size = (float)vdb_voxel_size(lod);
  int32_t voxel_count = vdb_total_voxel_count(lod);

  float t = 0.0F;

  ivector3_t index = {
    (int32_t)floorf(ray_origin.x / voxel_size),
    (int32_t)floorf(ray_origin.y / voxel_size),
    (int32_t)floorf(ray_origin.z / voxel_size),
  };

  ivector3_t step = {
    signf(ray_direction.x),
    signf(ray_direction.y),
    signf(ray_direction.z),
  };

  vector3_t direction_inv = {
    ray_direction.x != 0.0F ? 1.0F / ray_direction.x : FLT_MAX,
    ray_direction.y != 0.0F ? 1.0F / ray_direction.y : FLT_MAX,
    ray_direction.z != 0.0F ? 1.0F / ray_direction.z : FLT_MAX,
  };

  vector3_t next_boundary = {
    (step.x > 0 ? (float)(index.x + 1) : (float)(index.x)) * voxel_size,
    (step.y > 0 ? (float)(index.y + 1) : (float)(index.y)) * voxel_size,
    (step.z > 0 ? (float)(index.z + 1) : (float)(index.z)) * voxel_size,
  };

  vector3_t t_max = {
    (next_boundary.x - ray_origin.x) * direction_inv.x,
    (next_boundary.y - ray_origin.y) * direction_inv.y,
    (next_boundary.z - ray_origin.z) * direction_inv.z,
  };

  vector3_t t_delta = {
    fabsf(voxel_size * direction_inv.x),
    fabsf(voxel_size * direction_inv.y),
    fabsf(voxel_size * direction_inv.z),
  };

  while (t < max_distance) {

    if (index.x < 0 || index.y < 0 || index.z < 0 ||
        index.x >= voxel_count ||
        index.y >= voxel_count ||
        index.z >= voxel_count) {

      break;
    }

    int8_t solid = vdb_voxel_is_solid(brick_index, lod, index);

    if (solid && lod > 0) {

      lod--;

      float advance = fminf(t_max.x, fminf(t_max.y, t_max.z));

      ray_origin = vector3_add(ray_origin, vector3_muls(ray_direction, advance + EPSILON_6));

      t += advance + EPSILON_6;

      voxel_size = (float)vdb_voxel_size(lod);
      voxel_count = vdb_total_voxel_count(lod);

      index = (ivector3_t){
        (int32_t)floorf(ray_origin.x / voxel_size),
        (int32_t)floorf(ray_origin.y / voxel_size),
        (int32_t)floorf(ray_origin.z / voxel_size),
      };

      step = (ivector3_t){
        signf(ray_direction.x),
        signf(ray_direction.y),
        signf(ray_direction.z),
      };

      direction_inv = (vector3_t){
        ray_direction.x != 0.0F ? 1.0F / ray_direction.x : FLT_MAX,
        ray_direction.y != 0.0F ? 1.0F / ray_direction.y : FLT_MAX,
        ray_direction.z != 0.0F ? 1.0F / ray_direction.z : FLT_MAX,
      };

      next_boundary = (vector3_t){
        (step.x > 0 ? (float)(index.x + 1) : (float)(index.x)) * voxel_size,
        (step.y > 0 ? (float)(index.y + 1) : (float)(index.y)) * voxel_size,
        (step.z > 0 ? (float)(index.z + 1) : (float)(index.z)) * voxel_size,
      };

      t_max = (vector3_t){
        (next_boundary.x - ray_origin.x) * direction_inv.x,
        (next_boundary.y - ray_origin.y) * direction_inv.y,
        (next_boundary.z - ray_origin.z) * direction_inv.z,
      };

      t_delta = (vector3_t){
        fabsf(voxel_size * direction_inv.x),
        fabsf(voxel_size * direction_inv.y),
        fabsf(voxel_size * direction_inv.z),
      };

      continue;
    }

    if (solid && lod == 0) {

      vector3_t hit_position = vector3_add(ray_origin, vector3_muls(ray_direction, t));

      hit.intersect = 1;
      hit.index = index;
      hit.hit_position = hit_position;

      return hit;
    }

    float t_next = fminf(t_max.x, fminf(t_max.y, t_max.z));

    int32_t next_axis = -1;

    if (t_max.x <= t_max.y && t_max.x <= t_max.z) {
      next_axis = 0;
    } else if (t_max.y <= t_max.z) {
      next_axis = 1;
    } else {
      next_axis = 2;
    }

    int32_t step_x = fabsf(t_max.x - t_next) < EPSILON_6;
    int32_t step_y = fabsf(t_max.y - t_next) < EPSILON_6;
    int32_t step_z = fabsf(t_max.z - t_next) < EPSILON_6;

    if (step_x) {
      index.x += step.x;
      t_max.x += t_delta.x;
    }

    if (step_y) {
      index.y += step.y;
      t_max.y += t_delta.y;
    }

    if (step_z) {
      index.z += step.z;
      t_max.z += t_delta.z;
    }

    hit_axis = next_axis;

    t = t_next + EPSILON_6;
  }

  return hit;
}
void vdb_destroy(void) {
  int32_t brick_index = 0;
  int32_t brick_count = g_vdb.brick_count;

  while (brick_index < brick_count) {

    buffer_destroy(&g_vdb.bricks[brick_index].mask_buffer);

    brick_index++;
  }

  HEAP_FREE(g_vdb.bricks);
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

  uint32_t *mask = (uint32_t *)g_vdb.bricks[brick_index].mask_buffer.mapped_memory;

  return (mask[word] & bit) != 0u;
}

void vdb_voxel_set(int32_t brick_index, int8_t lod, ivector3_t index) {
  int32_t i = vdb_voxel_index(lod, index);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t bit = 1U << (i & 31);

  uint32_t *mask = (uint32_t *)g_vdb.bricks[brick_index].mask_buffer.mapped_memory;

  mask[word] |= bit;
}
void vdb_voxel_clr(int32_t brick_index, int8_t lod, ivector3_t index) {
  int32_t i = vdb_voxel_index(lod, index);
  int32_t word = s_vdb_word_offset[lod] + vdb_word_index(i);

  uint32_t bit = ~(1U << (i & 31));

  uint32_t *mask = (uint32_t *)g_vdb.bricks[brick_index].mask_buffer.mapped_memory;

  mask[word] &= bit;
}

static void vdb_debug_lod(int32_t brick_index, int8_t lod) {
  ivector3_t brick_position = g_vdb.bricks[brick_index].position;

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
