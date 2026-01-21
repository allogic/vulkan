#include <pch.h>

static void vdb_grow(vdb_t *vdb);
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
  int32_t lod6_words = (int32_t)ceilf((VDB_BRICK_CELL_COUNT_LOD6 * VDB_BRICK_CELL_COUNT_LOD6 * VDB_BRICK_CELL_COUNT_LOD6) / 0x40);
  int32_t lod5_words = (int32_t)ceilf((VDB_BRICK_CELL_COUNT_LOD5 * VDB_BRICK_CELL_COUNT_LOD5 * VDB_BRICK_CELL_COUNT_LOD5) / 0x40);
  int32_t lod4_words = (int32_t)ceilf((VDB_BRICK_CELL_COUNT_LOD4 * VDB_BRICK_CELL_COUNT_LOD4 * VDB_BRICK_CELL_COUNT_LOD4) / 0x40);
  int32_t lod3_words = (int32_t)ceilf((VDB_BRICK_CELL_COUNT_LOD3 * VDB_BRICK_CELL_COUNT_LOD3 * VDB_BRICK_CELL_COUNT_LOD3) / 0x40);
  int32_t lod2_words = (int32_t)ceilf((VDB_BRICK_CELL_COUNT_LOD2 * VDB_BRICK_CELL_COUNT_LOD2 * VDB_BRICK_CELL_COUNT_LOD2) / 0x40);
  int32_t lod1_words = (int32_t)ceilf((VDB_BRICK_CELL_COUNT_LOD1 * VDB_BRICK_CELL_COUNT_LOD1 * VDB_BRICK_CELL_COUNT_LOD1) / 0x40);
  int32_t lod0_words = (int32_t)ceilf((VDB_BRICK_CELL_COUNT_LOD0 * VDB_BRICK_CELL_COUNT_LOD0 * VDB_BRICK_CELL_COUNT_LOD0) / 0x40);

  vdb_brick_t brick = {
    .box_min = (ivector3_t){0, 0, 0},
    .box_max = (ivector3_t){VDB_BRICK_SIZE, VDB_BRICK_SIZE, VDB_BRICK_SIZE},
    .mask_lod6 = (uint64_t *)HEAP_ALLOC(sizeof(uint64_t) * lod6_words, 1, 0),
    .mask_lod5 = (uint64_t *)HEAP_ALLOC(sizeof(uint64_t) * lod5_words, 1, 0),
    .mask_lod4 = (uint64_t *)HEAP_ALLOC(sizeof(uint64_t) * lod4_words, 1, 0),
    .mask_lod3 = (uint64_t *)HEAP_ALLOC(sizeof(uint64_t) * lod3_words, 1, 0),
    .mask_lod2 = (uint64_t *)HEAP_ALLOC(sizeof(uint64_t) * lod2_words, 1, 0),
    .mask_lod1 = (uint64_t *)HEAP_ALLOC(sizeof(uint64_t) * lod1_words, 1, 0),
    .mask_lod0 = (uint64_t *)HEAP_ALLOC(sizeof(uint64_t) * lod0_words, 1, 0),
  };

  return brick;
}
void vdb_brick_build_lod(vdb_brick_t *brick, int8_t lod) {
  int32_t cell_count = vdb_brick_cell_count(lod);

  for (int32_t x = 0; x < cell_count; x++) {
    for (int32_t y = 0; y < cell_count; y++) {
      for (int32_t z = 0; z < cell_count; z++) {

        int8_t occupied = 0;

        for (int32_t dx = 0; dx < 2; dx++) {
          for (int32_t dy = 0; dy < 2; dy++) {
            for (int32_t dz = 0; dz < 2; dz++) {

              if (vdb_brick_get(
                    brick,
                    lod - 1,
                    x * 2 + dx,
                    y * 2 + dy,
                    z * 2 + dz)) {

                occupied = 1;
              }
            }
          }
        }

        if (occupied) {
          vdb_brick_set(brick, lod, x, y, z);
        }
      }
    }
  }
}
vdb_hit_t vdb_brick_hdda_raymarch(vdb_brick_t *brick, vector3_t ray_origin, vector3_t ray_direction, float max_distance) {
  vdb_hit_t hit = {0};

  float t_enter = 0.0F;
  float t_exit = 0.0F;

  if (!ray_aabb_intersect(
        ray_origin,
        ray_direction,
        (vector3_t){(float)brick->box_min.x, (float)brick->box_min.y, (float)brick->box_min.z},
        (vector3_t){(float)brick->box_max.x, (float)brick->box_max.y, (float)brick->box_max.z},
        &t_enter,
        &t_exit)) {
    return hit;
  }

  float t0 = fmaxf(t_enter, 0.0F);

  if (t0 > max_distance) {
    return hit;
  }

  float t = 0.0F;
  float t_max = fminf(t_exit - t0, max_distance - t0);

  int32_t lod = VDB_BRICK_MAX_LOD_LEVEL;

  float cell_size = (float)vdb_brick_cell_size(lod);
  int32_t cell_count = vdb_brick_cell_count(lod);

  ray_origin = vector3_add(ray_origin, vector3_muls(ray_direction, t0));

  int32_t vx = (int32_t)((ray_origin.x - (ray_direction.x < 0.0F ? EPSILON_4 : 0.0F)) / cell_size);
  int32_t vy = (int32_t)((ray_origin.y - (ray_direction.y < 0.0F ? EPSILON_4 : 0.0F)) / cell_size);
  int32_t vz = (int32_t)((ray_origin.z - (ray_direction.z < 0.0F ? EPSILON_4 : 0.0F)) / cell_size);

  int32_t step_x = signum(ray_direction.x);
  int32_t step_y = signum(ray_direction.y);
  int32_t step_z = signum(ray_direction.z);

  float t_delta_x = (ray_direction.x != 0.0F) ? cell_size / fabsf(ray_direction.x) : FLT_MAX;
  float t_delta_y = (ray_direction.y != 0.0F) ? cell_size / fabsf(ray_direction.y) : FLT_MAX;
  float t_delta_z = (ray_direction.z != 0.0F) ? cell_size / fabsf(ray_direction.z) : FLT_MAX;

  float voxel_boundary_x = (step_x > 0 ? (vx + 1) : vx) * cell_size;
  float voxel_boundary_y = (step_y > 0 ? (vy + 1) : vy) * cell_size;
  float voxel_boundary_z = (step_z > 0 ? (vz + 1) : vz) * cell_size;

  float t_max_x = (voxel_boundary_x - ray_origin.x) / ray_direction.x;
  float t_max_y = (voxel_boundary_y - ray_origin.y) / ray_direction.y;
  float t_max_z = (voxel_boundary_z - ray_origin.z) / ray_direction.z;

  t_max_x = (t_max_x < 0.0F) ? 0.0F : t_max_x;
  t_max_y = (t_max_y < 0.0F) ? 0.0F : t_max_y;
  t_max_z = (t_max_z < 0.0f) ? 0.0F : t_max_z;

  while (t < t_max) {

    if (vx < 0 || vy < 0 || vz < 0 || vx >= cell_count || vy >= cell_count || vz >= cell_count) {
      break;
    }

    int32_t v = vdb_brick_get(brick, lod, vx, vy, vz);

    if (v && lod > 0) {

      lod--;

      cell_size = (float)vdb_brick_cell_size(lod);
      cell_count = vdb_brick_cell_count(lod);

      vx = (int32_t)((ray_origin.x - (ray_direction.x < 0.0F ? EPSILON_4 : 0.0F)) / cell_size);
      vy = (int32_t)((ray_origin.y - (ray_direction.y < 0.0F ? EPSILON_4 : 0.0F)) / cell_size);
      vz = (int32_t)((ray_origin.z - (ray_direction.z < 0.0F ? EPSILON_4 : 0.0F)) / cell_size);

      step_x = signum(ray_direction.x);
      step_y = signum(ray_direction.y);
      step_z = signum(ray_direction.z);

      t_delta_x = (ray_direction.x != 0.0F) ? cell_size / fabsf(ray_direction.x) : FLT_MAX;
      t_delta_y = (ray_direction.y != 0.0F) ? cell_size / fabsf(ray_direction.y) : FLT_MAX;
      t_delta_z = (ray_direction.z != 0.0F) ? cell_size / fabsf(ray_direction.z) : FLT_MAX;

      voxel_boundary_x = (step_x > 0 ? (vx + 1) : vx) * cell_size;
      voxel_boundary_y = (step_y > 0 ? (vy + 1) : vy) * cell_size;
      voxel_boundary_z = (step_z > 0 ? (vz + 1) : vz) * cell_size;

      vector3_t next_origin = vector3_add(ray_origin, vector3_muls(ray_direction, t));

      t_max_x = (voxel_boundary_x - next_origin.x) / ray_direction.x;
      t_max_y = (voxel_boundary_y - next_origin.y) / ray_direction.y;
      t_max_z = (voxel_boundary_z - next_origin.z) / ray_direction.z;

      t_max_x = (t_max_x < 0.0F) ? 0.0F : t_max_x;
      t_max_y = (t_max_y < 0.0F) ? 0.0F : t_max_y;
      t_max_z = (t_max_z < 0.0f) ? 0.0F : t_max_z;

      continue;
    }

    if (v && lod == 0) {

      hit.hit = 1;
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
  }

  return hit;
}
void vdb_brick_destroy(vdb_brick_t *brick) {
  HEAP_FREE(brick->mask_lod6);
  HEAP_FREE(brick->mask_lod5);
  HEAP_FREE(brick->mask_lod4);
  HEAP_FREE(brick->mask_lod3);
  HEAP_FREE(brick->mask_lod2);
  HEAP_FREE(brick->mask_lod1);
  HEAP_FREE(brick->mask_lod0);
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

static int32_t vdb_position_hash(ivector3_t position, int32_t modulus) {
  return (((1 << 20) - 1) & ((int32_t)(position.x) * 73856093 ^
                             (int32_t)(position.y) * 19349669 ^
                             (int32_t)(position.z) * 83492791)) %
         modulus;
}
