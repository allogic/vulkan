#include <pch.h>

static void vdb_expand(vdb_t *vdb);
static float vdb_load_factor(vdb_t *vdb);

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
    vdb_expand(vdb);
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
void vdb_print(vdb_t *vdb) {
  int32_t table_index = 0;
  int32_t table_count = vdb->table_count;

  while (table_index < table_count) {

    vdb_record_t *record = vdb->table[table_index];

    if (record) {

      printf("Bucket (%d) %p\n", table_index, record);

      record = record->next;

      while (record) {

        printf("\t%p\n", record);

        record = record->next;
      }

    } else {

      printf("Bucket (%d) Empty\n", table_index);
    }

    table_index++;
  }
}
void vdb_destroy(vdb_t *vdb) {
  HEAP_FREE(vdb->table);
}

vdb_brick_t vdb_brick_create(void) {
  return (vdb_brick_t){
    .grid_size = 0x20,
  };
}
vdb_hit_t vdb_brick_raymarch(vdb_brick_t *brick, vector3_t ray_origin, vector3_t ray_direction, float max_distance) {
  vdb_hit_t hit = {0};

  vector3_t inv_direction = vector3_inv(ray_direction);

  int8_t lod = VDB_MAX_BRICK_LOD - 1;

  float t = 0.0F;

  while (t < max_distance) {

    float cell_size = (float)(1 << lod);

    int32_t vx = (int32_t)floorf(ray_origin.x / cell_size);
    int32_t vy = (int32_t)floorf(ray_origin.y / cell_size);
    int32_t vz = (int32_t)floorf(ray_origin.z / cell_size);

    if (vx < 0 || vy < 0 || vz < 0 ||
        vx >= brick->grid_size ||
        vy >= brick->grid_size ||
        vz >= brick->grid_size) {

      break;
    }

    int32_t v = vdb_brick_get(brick, vx, vy, vz);

    if (v == 0) {

      vector3_t next_boundary = {
        (vx + (ray_direction.x > 0.0F)) * cell_size,
        (vy + (ray_direction.y > 0.0F)) * cell_size,
        (vz + (ray_direction.z > 0.0F)) * cell_size,
      };

      float tx = (next_boundary.x - ray_origin.x) * inv_direction.x;
      float ty = (next_boundary.y - ray_origin.y) * inv_direction.y;
      float tz = (next_boundary.z - ray_origin.z) * inv_direction.z;

      float step_t = fminf(tx, fminf(ty, tz));

      t += step_t + EPSILON_4;

      ray_origin.x += ray_direction.x * (step_t + EPSILON_4);
      ray_origin.y += ray_direction.y * (step_t + EPSILON_4);
      ray_origin.z += ray_direction.z * (step_t + EPSILON_4);

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
void vdb_brick_debug(vdb_brick_t *brick, ivector3_t position) {
  for (int32_t x = 0; x < brick->grid_size; x++) {
    for (int32_t y = 0; y < brick->grid_size; y++) {
      for (int32_t z = 0; z < brick->grid_size; z++) {

        renderer_draw_debug_box(
          (vector3_t){(float)(position.x + x), (float)(position.y + y), (float)(position.z + z)},
          (vector3_t){1.0F, 1.0F, 1.0F},
          (vector4_t){1.0F, 1.0F, 0.0F, 1.0F});
      }
    }
  }
}
void vdb_brick_destroy(vdb_brick_t *brick) {
  // TODO
}

static void vdb_expand(vdb_t *vdb) {
  // int32_t next_table_size = (int32_t)ceilf((float)vdb->table_size * VDB_EXPANSION_FACTOR);
  int32_t next_table_count = (int32_t)ceilf((float)vdb->table_count * VDB_EXPANSION_FACTOR);
  int32_t next_table_size = next_table_count * sizeof(vdb_record_t *);

  // next_table_size = ALIGN_UP_BY(next_table_size, 8 * sizeof(vdb_record_t *));

  vdb_record_t **table = (vdb_record_t **)HEAP_ALLOC(next_table_size, 1, 0);

  int32_t table_index = 0;
  int32_t table_count = vdb->table_count;

  while (table_index < table_count) {

    vdb_record_t *record = vdb->table[table_index];

    while (record) {

      int32_t position_hash = vdb_position_hash(record->position, next_table_count);

      record->next = table[position_hash];
      table[position_hash] = record;

      record = record->next;
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

static int32_t vdb_position_hash(ivector3_t position, int32_t modulus) {
  return (((1 << 20) - 1) & ((int32_t)(position.x) * 73856093 ^
                             (int32_t)(position.y) * 19349669 ^
                             (int32_t)(position.z) * 83492791)) %
         modulus;
}
