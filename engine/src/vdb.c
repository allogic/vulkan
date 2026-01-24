#include <pch.h>

static void vdb_insert(ivector3_t position);
static void vdb_remove(ivector3_t position);
static void vdb_grow(void);
static float vdb_load(void);

static vdb_brick_t *vdb_brick_create(ivector3_t position);
static void vdb_brick_debug(vdb_brick_t *brick);
static void vdb_brick_destroy(vdb_brick_t *brick);

static int32_t vdb_position_hash(ivector3_t position, int32_t modulus);

vdb_t g_vdb = {0};

void vdb_create(ivector3_t cluster_dim, float radius_lod0) {
  g_vdb.table = (vdb_brick_t **)HEAP_ALLOC(VDB_INITIAL_TABLE_COUNT * sizeof(vdb_brick_t *), 1, 0);
  g_vdb.table_size = VDB_INITIAL_TABLE_COUNT * sizeof(vdb_brick_t *);
  g_vdb.table_count = VDB_INITIAL_TABLE_COUNT;
  g_vdb.brick_count = 0;
  g_vdb.cluster_dim = cluster_dim;

  for (int32_t x = 0; x < cluster_dim.x; x++) {
    for (int32_t y = 0; y < cluster_dim.y; y++) {
      for (int32_t z = 0; z < cluster_dim.z; z++) {

        vdb_insert((ivector3_t){x, y, z});
      }
    }
  }
}
vdb_brick_t *vdb_brick(ivector3_t position) {
  int32_t position_hash = vdb_position_hash(position, g_vdb.table_count);

  vdb_brick_t *brick = g_vdb.table[position_hash];

  while (brick) {

    if (brick->position.x == position.x &&
        brick->position.y == position.y &&
        brick->position.z == position.z) {

      return brick;
    }

    brick = brick->next;
  }

  return 0;
}
void vdb_debug(void) {
  int32_t table_index = 0;
  int32_t table_count = g_vdb.table_count;

  while (table_index < table_count) {

    vdb_brick_t *brick = g_vdb.table[table_index];

    while (brick) {

      vdb_brick_debug(brick);

      brick = brick->next;
    }

    table_index++;
  }
}
void vdb_destroy(void) {
  int32_t table_index = 0;
  int32_t table_count = g_vdb.table_count;

  while (table_index < table_count) {

    vdb_brick_t *brick = g_vdb.table[table_index];

    while (brick) {

      vdb_brick_t *next = brick->next;

      vdb_brick_destroy(brick);

      brick = next;
    }

    table_index++;
  }

  HEAP_FREE(g_vdb.table);
}

static void vdb_insert(ivector3_t position) {
  float load_factor = vdb_load();

  if (load_factor > VDB_LOAD_FACTOR) {
    vdb_grow();
  }

  int8_t position_exists = 0;
  int32_t position_hash = vdb_position_hash(position, g_vdb.table_count);

  vdb_brick_t *brick = g_vdb.table[position_hash];

  while (brick) {

    if (brick->position.x == position.x &&
        brick->position.y == position.y &&
        brick->position.z == position.z) {

      position_exists = 1;

      break;
    }

    brick = brick->next;
  }

  if (position_exists == 0) {

    brick = vdb_brick_create(position);

    brick->next = g_vdb.table[position_hash];

    g_vdb.table[position_hash] = brick;
    g_vdb.brick_count++;
  }
}
static void vdb_remove(ivector3_t position) {
  int32_t position_hash = vdb_position_hash(position, g_vdb.table_count);

  vdb_brick_t *curr_brick = g_vdb.table[position_hash];
  vdb_brick_t *prev_brick = 0;

  while (curr_brick) {

    if (curr_brick->position.x == position.x &&
        curr_brick->position.y == position.y &&
        curr_brick->position.z == position.z) {

      if (prev_brick) {
        prev_brick->next = curr_brick->next;
      } else {
        g_vdb.table[position_hash] = curr_brick->next;
      }

      vdb_brick_destroy(curr_brick);

      g_vdb.brick_count--;

      break;
    }

    prev_brick = curr_brick;
    curr_brick = curr_brick->next;
  }
}
static void vdb_grow(void) {
  int32_t next_table_count = (int32_t)ceilf((float)g_vdb.table_count * VDB_GROWTH_FACTOR);
  int32_t next_table_size = next_table_count * sizeof(vdb_brick_t *);

  vdb_brick_t **table = (vdb_brick_t **)HEAP_ALLOC(next_table_size, 1, 0);

  int32_t table_index = 0;
  int32_t table_count = g_vdb.table_count;

  while (table_index < table_count) {

    vdb_brick_t *brick = g_vdb.table[table_index];

    while (brick) {

      vdb_brick_t *next = brick->next;

      int32_t position_hash = vdb_position_hash(brick->position, next_table_count);

      brick->next = table[position_hash];
      table[position_hash] = brick;

      brick = next;
    }

    table_index++;
  }

  HEAP_FREE(g_vdb.table);

  g_vdb.table = table;
  g_vdb.table_size = next_table_size;
  g_vdb.table_count = next_table_count;
}
static float vdb_load(void) {
  return (((float)g_vdb.brick_count + 1.0F) / (float)g_vdb.table_count) * 100.0F;
}

static vdb_brick_t *vdb_brick_create(ivector3_t position) {
  vdb_brick_t *brick = (vdb_brick_t *)HEAP_ALLOC(sizeof(vdb_brick_t), 1, 0);

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

  brick->position = position;
  brick->mask_buffer = buffer_create_storage(0, sizeof(uint32_t) * mask_buffer_size);

  return brick;
}
static void vdb_brick_debug(vdb_brick_t *brick) {
  renderer_draw_debug_box(
    (vector3_t){(float)brick->position.x * VDB_BASE_RES, (float)brick->position.y * VDB_BASE_RES, (float)brick->position.z * VDB_BASE_RES},
    (vector3_t){(float)VDB_BASE_RES, (float)VDB_BASE_RES, (float)VDB_BASE_RES},
    (vector4_t){1.0F, 1.0F, 0.0F, 1.0F});
}
static void vdb_brick_destroy(vdb_brick_t *brick) {
  buffer_destroy(&brick->mask_buffer);

  HEAP_FREE(brick);
}

static int32_t vdb_position_hash(ivector3_t position, int32_t modulus) {
  return (((1 << 20) - 1) & ((int32_t)(position.x) * 73856093 ^
                             (int32_t)(position.y) * 19349669 ^
                             (int32_t)(position.z) * 83492791)) %
         modulus;
}
