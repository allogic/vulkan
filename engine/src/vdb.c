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

      vdb_brick_draw(brick, 0, record->position);
      vdb_brick_draw(brick, 1, record->position);
      vdb_brick_draw(brick, 2, record->position);
      vdb_brick_draw(brick, 3, record->position);

      vdb_brick_raymarch(brick, ray_origin, ray_direction, 1000.0F);

      // TODO: remove this
      for (float i = 0.0F; i < 48.0F; i += 0.2F) {
        vector3_t ray_orig = {i - 8.0F, i / 2.0F, -10.0F};
        vector3_t ray_dir = {0.5F, 0.0F, 0.8F};

        vdb_brick_raymarch(brick, ray_orig, ray_dir, 1000.0F);
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

          float d = ((float)y + 0.5F) / cell_count;

          if (d < 0.5F) {
            vdb_brick_set(&brick, lod, x, y, z);
          }
        }
      }
    }
  }

  return brick;
}
vdb_hit_t vdb_brick_raymarch(vdb_brick_t *brick, vector3_t ray_origin, vector3_t ray_direction, float max_distance) {
  vdb_hit_t hit = {0};

  int32_t lod = VDB_BRICK_MAX_LOD_LEVEL;

  int32_t cell_count = vdb_brick_cell_count(lod);
  float cell_size = (float)vdb_brick_cell_size(lod);

  int32_t vx = (int32_t)floorf(ray_origin.x / cell_size);
  int32_t vy = (int32_t)floorf(ray_origin.y / cell_size);
  int32_t vz = (int32_t)floorf(ray_origin.z / cell_size);

  int32_t step_x = signum(ray_direction.x);
  int32_t step_y = signum(ray_direction.y);
  int32_t step_z = signum(ray_direction.z);

  float next_boundary_x = (step_x > 0 ? (vx + 1) * cell_size : vx * cell_size);
  float next_boundary_y = (step_y > 0 ? (vy + 1) * cell_size : vy * cell_size);
  float next_boundary_z = (step_z > 0 ? (vz + 1) * cell_size : vz * cell_size);

  float t_max_x = (ray_direction.x != 0.0F)
                    ? (next_boundary_x - ray_origin.x) / ray_direction.x
                    : FLT_MAX;

  float t_max_y = (ray_direction.y != 0.0F)
                    ? (next_boundary_y - ray_origin.y) / ray_direction.y
                    : FLT_MAX;

  float t_max_z = (ray_direction.z != 0.0F)
                    ? (next_boundary_z - ray_origin.z) / ray_direction.z
                    : FLT_MAX;

  float t_delta_x = (ray_direction.x != 0.0F)
                      ? cell_size / fabsf(ray_direction.x)
                      : FLT_MAX;

  float t_delta_y = (ray_direction.y != 0.0F)
                      ? cell_size / fabsf(ray_direction.y)
                      : FLT_MAX;

  float t_delta_z = (ray_direction.z != 0.0F)
                      ? cell_size / fabsf(ray_direction.z)
                      : FLT_MAX;

  vector3_t prev_position = ray_origin;

  int32_t it = 0;

  float t = 0.0F;

  while (t <= max_distance) {

    // if (vx < 0 || vy < 0 || vz < 0 || vx >= cell_count || vy >= cell_count || vz >= cell_count) {
    //   break;
    // }

    if (vdb_brick_get(brick, lod, vx, vy, vz)) {

      if (lod > 0) {

        float old_cell_size = (float)vdb_brick_cell_size(lod);
        lod--;
        float new_cell_size = (float)vdb_brick_cell_size(lod);

        // Convert current voxel position to finer LOD
        vx = (int)floorf(vx * old_cell_size / new_cell_size);
        vy = (int)floorf(vy * old_cell_size / new_cell_size);
        vz = (int)floorf(vz * old_cell_size / new_cell_size);

        // Also scale tMax and tDelta along each axis
        t_max_x = (vx * new_cell_size - ray_origin.x) / ray_direction.x;
        t_max_y = (vy * new_cell_size - ray_origin.y) / ray_direction.y;
        t_max_z = (vz * new_cell_size - ray_origin.z) / ray_direction.z;

        t_delta_x = new_cell_size / fabsf(ray_direction.x);
        t_delta_y = new_cell_size / fabsf(ray_direction.y);
        t_delta_z = new_cell_size / fabsf(ray_direction.z);

        // OLD IMPLEMENTATION

        // lod--;

        // cell_size = (float)vdb_brick_cell_size(lod);
        // cell_count = vdb_brick_cell_count(lod);

        // vx = (int32_t)floorf(ray_origin.x / cell_size);
        // vy = (int32_t)floorf(ray_origin.y / cell_size);
        // vz = (int32_t)floorf(ray_origin.z / cell_size);

        continue;
      }

      hit.hit = 1;
      hit.lod = lod;
      hit.position = (ivector3_t){vx, vy, vz};

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
void vdb_brick_draw(vdb_brick_t *brick, int8_t lod, ivector3_t brick_position) {
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
            (vector4_t){brick->colors[lod + 1], brick->colors[lod + 2 * 3], brick->colors[lod + 3 * 6], 0.2F});
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
