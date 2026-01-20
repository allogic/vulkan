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

  vector3_t inv_direction = vector3_inv(ray_direction);
  vector3_t prev_ray_origin = ray_origin;

  int8_t lod = VDB_BRICK_MAX_LOD_LEVEL;
  int32_t iter = 0;

  float t = 0.0F;

  while (t < max_distance) {

    int32_t cell_count = vdb_brick_cell_count(lod);
    int32_t cell_size = vdb_brick_cell_size(lod);

    int32_t vx = (int32_t)floorf(ray_origin.x / cell_size);
    int32_t vy = (int32_t)floorf(ray_origin.y / cell_size);
    int32_t vz = (int32_t)floorf(ray_origin.z / cell_size);

    if (vx < 0 || vy < 0 || vz < 0 || vx >= cell_count || vy >= cell_count || vz >= cell_count) {

      // break;
    }

    iter++;

    int8_t v = vdb_brick_get(brick, lod, vx, vy, vz);

    if (v == 0) {

      vector3_t next_boundary = {
        ((float)vx + (ray_direction.x > 0.0F)) * cell_size,
        ((float)vy + (ray_direction.y > 0.0F)) * cell_size,
        ((float)vz + (ray_direction.z > 0.0F)) * cell_size,
      };

      float tx = (next_boundary.x - ray_origin.x) * inv_direction.x;
      float ty = (next_boundary.y - ray_origin.y) * inv_direction.y;
      float tz = (next_boundary.z - ray_origin.z) * inv_direction.z;

      float step_t = fminf(tx, fminf(ty, tz));

      t += step_t + EPSILON_4;

      ray_origin.x += ray_direction.x * (step_t + EPSILON_4);
      ray_origin.y += ray_direction.y * (step_t + EPSILON_4);
      ray_origin.z += ray_direction.z * (step_t + EPSILON_4);

      renderer_draw_debug_line(
        prev_ray_origin,
        ray_origin,
        (vector4_t){brick->colors[iter], brick->colors[iter + 3], brick->colors[iter + 6], 1.0F});

      prev_ray_origin = ray_origin;

      continue;
    }

    if (lod > 0) {

      lod--;

      continue;
    }

    hit.hit = 1;
    hit.lod = lod;
    hit.position = (ivector3_t){vx, vy, vz};

    return hit;
  }

  return hit;
}
void vdb_brick_draw(vdb_brick_t *brick, int8_t lod, ivector3_t brick_position) {
  int32_t cell_count = vdb_brick_cell_count(lod);
  int32_t cell_size = vdb_brick_cell_size(lod);

  for (int32_t x = 0; x < cell_count; x++) {
    for (int32_t y = 0; y < cell_count; y++) {
      for (int32_t z = 0; z < cell_count; z++) {

        int8_t v = vdb_brick_get(brick, lod, x, y, z);

        if (v) {

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
