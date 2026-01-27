#include <pch.h>

vdb_t g_vdb = {0};

static int32_t const s_vdb_mask_buffer_size = 9364;

void vdb_create(ivector3_t dimension) {
  vdb_info_t vdb_info = {
    .dimension = dimension,
  };

  int32_t brick_count = dimension.x * dimension.y * dimension.z;

  g_vdb.brick = (vdb_brick_t *)HEAP_ALLOC(brick_count * sizeof(vdb_brick_t), 1, 0);
  g_vdb.brick_count = brick_count;
  g_vdb.dimension = dimension;
  g_vdb.info_buffer = buffer_create_uniform(&vdb_info, sizeof(vdb_info_t));
  g_vdb.terrain_layer_buffer = buffer_create_uniform_coherent(0, sizeof(terrain_mod_t) * VDB_MAX_TERRAIN_MODIFIER);

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
void vdb_destroy(void) {
  int32_t brick_index = 0;
  int32_t brick_count = g_vdb.brick_count;

  while (brick_index < brick_count) {

    buffer_destroy(&g_vdb.brick[brick_index].mask_buffer);

    brick_index++;
  }

  buffer_destroy(&g_vdb.info_buffer);
  buffer_destroy(&g_vdb.terrain_layer_buffer);

  HEAP_FREE(g_vdb.brick);
}
