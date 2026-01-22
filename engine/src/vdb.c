#include <pch.h>

static void vdb_grow(vdb_t *vdb);
static float vdb_load(vdb_t *vdb);
static int32_t vdb_hash(ivector3_t position, int32_t modulus);

vdb_t vdb_create(void) {
  return (vdb_t){
    .table = (vdb_record_t **)HEAP_ALLOC(VDB_INITIAL_TABLE_COUNT * sizeof(vdb_record_t *), 1, 0),
    .table_size = VDB_INITIAL_TABLE_COUNT * sizeof(vdb_record_t *),
    .table_count = VDB_INITIAL_TABLE_COUNT,
    .record_count = 0,
  };
}
void vdb_insert(vdb_t *vdb, ivector3_t position) {
  float load_factor = vdb_load(vdb);

  if (load_factor > VDB_LOAD_FACTOR) {
    vdb_grow(vdb);
  }

  int8_t position_exists = 0;
  int32_t position_hash = vdb_hash(position, vdb->table_count);

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
  int32_t position_hash = vdb_hash(position, vdb->table_count);

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
vdb_brick_t *vdb_brick(vdb_t *vdb, ivector3_t position) {
  int32_t position_hash = vdb_hash(position, vdb->table_count);

  vdb_record_t *record = vdb->table[position_hash];

  while (record) {

    if (record->position.x == position.x &&
        record->position.y == position.y &&
        record->position.z == position.z) {

      return &record->brick;
    }

    record = record->next;
  }

  return 0;
}
void vdb_destroy(vdb_t *vdb) {
  int32_t table_index = 0;
  int32_t table_count = vdb->table_count;

  while (table_index < table_count) {

    vdb_record_t *record = vdb->table[table_index];

    while (record) {

      vdb_record_t *next = record->next;

      vdb_brick_destroy(&record->brick);

      HEAP_FREE(record);

      record = next;
    }

    table_index++;
  }

  HEAP_FREE(vdb->table);
}

vdb_brick_t vdb_brick_create(void) {
  uint64_t mask_buffer_size_lod6 = 1;
  uint64_t mask_buffer_size_lod5 = 1;
  uint64_t mask_buffer_size_lod4 = 1;
  uint64_t mask_buffer_size_lod3 = 8;
  uint64_t mask_buffer_size_lod2 = 64;
  uint64_t mask_buffer_size_lod1 = 512;
  uint64_t mask_buffer_size_lod0 = 4096;

  uint64_t mask_buffer_size =
    mask_buffer_size_lod6 +
    mask_buffer_size_lod5 +
    mask_buffer_size_lod4 +
    mask_buffer_size_lod3 +
    mask_buffer_size_lod2 +
    mask_buffer_size_lod1 +
    mask_buffer_size_lod0;

  return (vdb_brick_t){
    .mask_buffer = buffer_create_storage(0, sizeof(uint64_t) * mask_buffer_size),
  };
}
void vdb_brick_destroy(vdb_brick_t *brick) {
  buffer_destroy(&brick->mask_buffer);
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

      int32_t position_hash = vdb_hash(record->position, next_table_count);

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
static float vdb_load(vdb_t *vdb) {
  return (((float)vdb->record_count + 1.0F) / (float)vdb->table_count) * 100.0F;
}
static int32_t vdb_hash(ivector3_t position, int32_t modulus) {
  return (((1 << 20) - 1) & ((int32_t)(position.x) * 73856093 ^
                             (int32_t)(position.y) * 19349669 ^
                             (int32_t)(position.z) * 83492791)) %
         modulus;
}
