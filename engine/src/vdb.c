#include <pch.h>

static void vdb_grow(vdb_t *vdb);
static float vdb_load_factor(vdb_t *vdb);

static float vdb_dummy_surface_density(ivector3_t position, int8_t lod);

static int32_t vdb_position_hash(ivector3_t position, int32_t modulus);

vdb_t vdb_create(void) {
  return (vdb_t){
    .table = (vdb_record_t **)HEAP_ALLOC(VDB_INITIAL_TABLE_COUNT * sizeof(vdb_record_t *), 1, 0),
    .table_size = VDB_INITIAL_TABLE_COUNT * sizeof(vdb_record_t *),
    .table_count = VDB_INITIAL_TABLE_COUNT,
    .record_count = 0,
  };
}
void vdb_insert(vdb_t *vdb, ivector3_t position) {
  float load_factor = vdb_load_factor(vdb);

  if (load_factor > VDB_LOAD_FACTOR) {
    vdb_grow(vdb);
  }

  int8_t position_exists = 0;
  int32_t position_hash = vdb_position_hash(position, vdb->table_count);

  vdb_record_t *record = vdb->table[position_hash];

  while (record) {

    if (record->position.x == position.x &&
        record->position.y == position.y &&
        record->position.z == position.z) {

      position_exists = 1;

      break;
    }

    record = record->next;
  }

  if (position_exists == 0) {

    record = (vdb_record_t *)HEAP_ALLOC(sizeof(vdb_record_t), 1, 0);

    record->next = vdb->table[position_hash];
    record->position = position;
    record->brick = vdb_brick_create();

    vdb->table[position_hash] = record;
    vdb->record_count++;
  }
}
void vdb_remove(vdb_t *vdb, ivector3_t position) {
  int32_t position_hash = vdb_position_hash(position, vdb->table_count);

  vdb_record_t *curr_record = vdb->table[position_hash];
  vdb_record_t *prev_record = 0;

  while (curr_record) {

    if (curr_record->position.x == position.x &&
        curr_record->position.y == position.y &&
        curr_record->position.z == position.z) {

      if (prev_record) {
        prev_record->next = curr_record->next;
      } else {
        vdb->table[position_hash] = curr_record->next;
      }

      HEAP_FREE(curr_record);

      vdb->record_count--;

      break;
    }

    prev_record = curr_record;
    curr_record = curr_record->next;
  }
}
void vdb_draw(vdb_t *vdb, vector3_t ray_origin, vector3_t ray_direction) {
  int32_t table_index = 0;
  int32_t table_count = vdb->table_count;

  while (table_index < table_count) {

    vdb_record_t *record = vdb->table[table_index];

    while (record) {

      vdb_brick_t *brick = &record->brick;

      vdb_brick_draw(brick, 0, record->position, 0.1F);
      // vdb_brick_draw(brick, 1, record->position, 0.1F);
      // vdb_brick_draw(brick, 2, record->position, 0.15F);
      // vdb_brick_draw(brick, 3, record->position, 0.2F);

      vdb_brick_hdda_raymarch(brick, ray_origin, ray_direction, 1000.0F);

      // TODO: remove this
      for (float i = 0.0F; i < 48.0F; i += 0.2F) {
        vector3_t ray_orig = {i - 8.0F, i / 2.0F, -10.0F};
        vector3_t ray_dir = {0.5F, 0.0F, 0.8F};

        // vdb_brick_hdda_raymarch(brick, ray_orig, ray_dir, 1000.0F);
      }

      record = record->next;
    }

    table_index++;
  }
}
void vdb_destroy(vdb_t *vdb) {
  int32_t table_index = 0;
  int32_t table_count = vdb->table_count;

  while (table_index < table_count) {

    vdb_record_t *record = vdb->table[table_index];

    while (record) {

      vdb_record_t *next = record->next;

      HEAP_FREE(record);

      record = next;
    }

    table_index++;
  }

  HEAP_FREE(vdb->table);
}

vdb_brick_t vdb_brick_create(void) {
  float *colors = (float *)HEAP_ALLOC(sizeof(float) * 10000, 1, 0);

  for (int i = 0; i < 1000; i++) {
    colors[i] = (float)rand() / (float)RAND_MAX;
  }

  vdb_brick_t brick = {0};
  brick.colors = colors;
  brick.box_min = (ivector3_t){0, 0, 0};
  brick.box_max = (ivector3_t){32, 32, 32};

  {
    int8_t lod = 0;

    int32_t cell_count = vdb_brick_cell_count(lod);
    int32_t cell_size = vdb_brick_cell_size(lod);

    for (int32_t x = 0; x < cell_count; x++) {
      for (int32_t y = 0; y < cell_count; y++) {
        for (int32_t z = 0; z < cell_count; z++) {

          // float d = ((float)y + 0.5F) / cell_count;
          float d = vdb_dummy_surface_density((ivector3_t){x, y, z}, lod);

          if (d < 0.0F) {
            vdb_brick_set(&brick, lod, x, y, z);
          }
        }
      }
    }
  }

  {
    int8_t lod = 1;

    int32_t cell_count = vdb_brick_cell_count(lod);
    int32_t cell_size = vdb_brick_cell_size(lod);

    for (int32_t x = 0; x < cell_count; x++) {
      for (int32_t y = 0; y < cell_count; y++) {
        for (int32_t z = 0; z < cell_count; z++) {

          // float d = ((float)y + 0.5F) / cell_count;
          float d = vdb_dummy_surface_density((ivector3_t){x, y, z}, lod);

          if (d < 0.0F) {
            vdb_brick_set(&brick, lod, x, y, z);
          }
        }
      }
    }
  }

  {
    int8_t lod = 2;

    int32_t cell_count = vdb_brick_cell_count(lod);
    int32_t cell_size = vdb_brick_cell_size(lod);

    for (int32_t x = 0; x < cell_count; x++) {
      for (int32_t y = 0; y < cell_count; y++) {
        for (int32_t z = 0; z < cell_count; z++) {

          // float d = ((float)y + 0.5F) / cell_count;
          float d = vdb_dummy_surface_density((ivector3_t){x, y, z}, lod);

          if (d < 0.0F) {
            vdb_brick_set(&brick, lod, x, y, z);
          }
        }
      }
    }
  }

  {
    int8_t lod = 3;

    int32_t cell_count = vdb_brick_cell_count(lod);
    int32_t cell_size = vdb_brick_cell_size(lod);

    for (int32_t x = 0; x < cell_count; x++) {
      for (int32_t y = 0; y < cell_count; y++) {
        for (int32_t z = 0; z < cell_count; z++) {

          // float d = ((float)y + 0.5F) / cell_count;
          float d = vdb_dummy_surface_density((ivector3_t){x, y, z}, lod);

          if (d < 0.0F) {
            vdb_brick_set(&brick, lod, x, y, z);
          }
        }
      }
    }
  }

  return brick;
}
vdb_hit_t vdb_brick_hdda_raymarch(vdb_brick_t *brick, vector3_t ray_origin, vector3_t ray_direction, float max_distance) {
  vdb_hit_t hit = {0};

  float t_enter = 0.0F;
  float t_exit = 0.0F;

  if (ray_aabb_intersect(
        ray_origin,
        ray_direction,
        (vector3_t){(float)brick->box_min.x, (float)brick->box_min.y, (float)brick->box_min.z},
        (vector3_t){(float)brick->box_max.x, (float)brick->box_max.y, (float)brick->box_max.z},
        &t_enter,
        &t_exit) == 0) {

    return hit;
  }

  float t0 = fmaxf(t_enter, 0.0f);

  t0 += EPSILON_4;

  if (t0 > max_distance)
    return hit;

  vector3_t prev_position = ray_origin;

  ray_origin.x += ray_direction.x * t0;
  ray_origin.y += ray_direction.y * t0;
  ray_origin.z += ray_direction.z * t0;

  max_distance -= t0;

  renderer_draw_debug_line(
    prev_position,
    ray_origin,
    (vector4_t){brick->colors[0], brick->colors[3], brick->colors[6], 1.0F});

  // Start at the coarsest LOD
  int8_t lod = VDB_BRICK_MAX_LOD_LEVEL;

  // We'll maintain a stack of LOD states for when we descend
  typedef struct {
    int8_t lod;
    int32_t vx, vy, vz;
    float cell_size;
    int stepX, stepY, stepZ;
    float tMaxX, tMaxY, tMaxZ;
    float tDeltaX, tDeltaY, tDeltaZ;
  } dda_state_t;

#define MAX_LOD_STACK 4
  dda_state_t stack[MAX_LOD_STACK];
  int stack_top = 0;

  // Initialize DDA at current LOD
  float cell_size = (float)vdb_brick_cell_size(lod);
  int cell_count = vdb_brick_cell_count(lod);

  int vx = (int)floorf(ray_origin.x / cell_size);
  int vy = (int)floorf(ray_origin.y / cell_size);
  int vz = (int)floorf(ray_origin.z / cell_size);

  int stepX = signum(ray_direction.x);
  int stepY = signum(ray_direction.y);
  int stepZ = signum(ray_direction.z);

  float nextX = (stepX > 0 ? (vx + 1) * cell_size : vx * cell_size);
  float nextY = (stepY > 0 ? (vy + 1) * cell_size : vy * cell_size);
  float nextZ = (stepZ > 0 ? (vz + 1) * cell_size : vz * cell_size);

  float tMaxX = (ray_direction.x != 0.0f) ? (nextX - ray_origin.x) / ray_direction.x : FLT_MAX;
  float tMaxY = (ray_direction.y != 0.0f) ? (nextY - ray_origin.y) / ray_direction.y : FLT_MAX;
  float tMaxZ = (ray_direction.z != 0.0f) ? (nextZ - ray_origin.z) / ray_direction.z : FLT_MAX;

  float tDeltaX = (ray_direction.x != 0.0f) ? cell_size / fabsf(ray_direction.x) : FLT_MAX;
  float tDeltaY = (ray_direction.y != 0.0f) ? cell_size / fabsf(ray_direction.y) : FLT_MAX;
  float tDeltaZ = (ray_direction.z != 0.0f) ? cell_size / fabsf(ray_direction.z) : FLT_MAX;

  int32_t it = 0;

  float t = 0.0F;

  while (t <= max_distance) {

    // Bounds check
    if (vx < 0 || vy < 0 || vz < 0 || vx >= cell_count || vy >= cell_count || vz >= cell_count) {

      // Pop previous LOD state if any
      if (stack_top == 0)
        break;

      stack_top--;
      dda_state_t s = stack[stack_top];

      lod = s.lod;
      vx = s.vx;
      vy = s.vy;
      vz = s.vz;
      cell_size = s.cell_size;
      stepX = s.stepX;
      stepY = s.stepY;
      stepZ = s.stepZ;
      tMaxX = s.tMaxX;
      tMaxY = s.tMaxY;
      tMaxZ = s.tMaxZ;
      tDeltaX = s.tDeltaX;
      tDeltaY = s.tDeltaY;
      tDeltaZ = s.tDeltaZ;
      cell_count = vdb_brick_cell_count(lod);

      it = 0;

      continue;
    }

    // Sample voxel
    int8_t v = vdb_brick_get(brick, lod, vx, vy, vz);

    if (v != 0 && lod > 0) {
      // Descend to finer LOD within current voxel bounds

      if (stack_top >= MAX_LOD_STACK) {

        // Safety fallback
        lod = 0;

      } else {

        // Push current state
        stack[stack_top++] = (dda_state_t){
          lod, vx, vy, vz, cell_size,
          stepX, stepY, stepZ,
          tMaxX, tMaxY, tMaxZ,
          tDeltaX, tDeltaY, tDeltaZ};

        // Go to next finer LOD
        lod--;
        float new_cell_size = (float)vdb_brick_cell_size(lod);
        cell_size = new_cell_size;
        cell_count = vdb_brick_cell_count(lod);

        // Map coarse voxel to fine voxel grid
        // vx = vx * 2; // assume 2x finer per LOD step
        // vy = vy * 2;
        // vz = vz * 2;

        vector3_t pos = vector3_add(ray_origin, vector3_muls(ray_direction, t));

        vx = (int)floorf(pos.x / cell_size);
        vy = (int)floorf(pos.y / cell_size);
        vz = (int)floorf(pos.z / cell_size);

        stepX = signum(ray_direction.x);
        stepY = signum(ray_direction.y);
        stepZ = signum(ray_direction.z);

        nextX = (stepX > 0 ? (vx + 1) * cell_size : vx * cell_size);
        nextY = (stepY > 0 ? (vy + 1) * cell_size : vy * cell_size);
        nextZ = (stepZ > 0 ? (vz + 1) * cell_size : vz * cell_size);

        tMaxX = (ray_direction.x != 0.0f) ? (nextX - ray_origin.x) / ray_direction.x : FLT_MAX;
        tMaxY = (ray_direction.y != 0.0f) ? (nextY - ray_origin.y) / ray_direction.y : FLT_MAX;
        tMaxZ = (ray_direction.z != 0.0f) ? (nextZ - ray_origin.z) / ray_direction.z : FLT_MAX;

        tDeltaX = (ray_direction.x != 0.0f) ? cell_size / fabsf(ray_direction.x) : FLT_MAX;
        tDeltaY = (ray_direction.y != 0.0f) ? cell_size / fabsf(ray_direction.y) : FLT_MAX;
        tDeltaZ = (ray_direction.z != 0.0f) ? cell_size / fabsf(ray_direction.z) : FLT_MAX;

        continue;
      }
    }

    // Hit at finest LOD
    if (v != 0 && lod == 0) {

      hit.hit = 1;
      hit.position = (ivector3_t){vx, vy, vz};

      renderer_draw_debug_box(
        (vector3_t){(float)vx, (float)vy, (float)vz},
        (vector3_t){1.0F, 1.0F, 1.0F},
        (vector4_t){1.0F, 1.0F, 0.0F, 1.0F});

      return hit;
    }

    float t_step = 0.0F;

    // Step to next voxel boundary (classic 3D DDA)
    if (tMaxX < tMaxY) {
      if (tMaxX < tMaxZ) {
        vx += stepX;
        t_step = tMaxX;
        tMaxX += tDeltaX;
      } else {
        vz += stepZ;
        t_step = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    } else {
      if (tMaxY < tMaxZ) {
        vy += stepY;
        t_step = tMaxY;
        tMaxY += tDeltaY;
      } else {
        vz += stepZ;
        t_step = tMaxZ;
        tMaxZ += tDeltaZ;
      }
    }

    t = t_step;

    vector3_t position = vector3_add(ray_origin, vector3_muls(ray_direction, t));

    renderer_draw_debug_line(
      prev_position,
      position,
      (vector4_t){brick->colors[it], brick->colors[it + 3], brick->colors[it + 6], 1.0F});

    prev_position = position;

    it++;
  }

  return hit;
}
void vdb_brick_draw(vdb_brick_t *brick, int8_t lod, ivector3_t brick_position, float alpha) {
  int32_t cell_count = vdb_brick_cell_count(lod);
  int32_t cell_size = vdb_brick_cell_size(lod);

  for (int32_t x = 0; x < cell_count; x++) {
    for (int32_t y = 0; y < cell_count; y++) {
      for (int32_t z = 0; z < cell_count; z++) {

        if (vdb_brick_get(brick, lod, x, y, z)) {

          ivector3_t cell_position = ivector3_add(ivector3_muls((ivector3_t){x, y, z}, cell_size), brick_position);

          renderer_draw_debug_box(
            (vector3_t){(float)cell_position.x, (float)cell_position.y, (float)cell_position.z},
            (vector3_t){(float)cell_size, (float)cell_size, (float)cell_size},
            (vector4_t){brick->colors[lod + 1], brick->colors[lod + 2 * 3], brick->colors[lod + 3 * 6], alpha});
        }
      }
    }
  }
}
void vdb_brick_destroy(vdb_brick_t *brick) {
  HEAP_FREE(brick->colors);
}

static void vdb_grow(vdb_t *vdb) {
  int32_t next_table_count = (int32_t)ceilf((float)vdb->table_count * VDB_GROWTH_FACTOR);
  int32_t next_table_size = next_table_count * sizeof(vdb_record_t *);

  vdb_record_t **table = (vdb_record_t **)HEAP_ALLOC(next_table_size, 1, 0);

  int32_t table_index = 0;
  int32_t table_count = vdb->table_count;

  while (table_index < table_count) {

    vdb_record_t *record = vdb->table[table_index];

    while (record) {

      vdb_record_t *next = record->next;

      int32_t position_hash = vdb_position_hash(record->position, next_table_count);

      record->next = table[position_hash];
      table[position_hash] = record;

      record = next;
    }

    table_index++;
  }

  HEAP_FREE(vdb->table);

  vdb->table = table;
  vdb->table_size = next_table_size;
  vdb->table_count = next_table_count;
}
static float vdb_load_factor(vdb_t *vdb) {
  return (((float)vdb->record_count + 1.0F) / (float)vdb->table_count) * 100.0F;
}

static float vdb_dummy_surface_density(ivector3_t position, int8_t lod) {
  int32_t cell_count = vdb_brick_cell_count(lod);
  int32_t cell_size = vdb_brick_cell_size(lod);

  float fx = ((float)position.x + 0.5F) / cell_count * 2.0F - 1.0F;
  float fy = ((float)position.y + 0.5F) / cell_count * 2.0F - 1.0F;
  float fz = ((float)position.z + 0.5F) / cell_count * 2.0F - 1.0F;

  float nx = 0.7071F;
  float ny = 0.4082F;
  float nz = 0.5773F;

  float density = fx * nx + fy * ny + fz * nz;

  return density;
}

static int32_t vdb_position_hash(ivector3_t position, int32_t modulus) {
  return (((1 << 20) - 1) & ((int32_t)(position.x) * 73856093 ^
                             (int32_t)(position.y) * 19349669 ^
                             (int32_t)(position.z) * 83492791)) %
         modulus;
}
