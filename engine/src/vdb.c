#include <pch.h>

vdb_t g_vdb = {0};

void vdb_create(void) {
  vdb_info_t vdb_info = {
    .cluster_dim = {
      .x = VDB_CLUSTER_DIM_X,
      .y = VDB_CLUSTER_DIM_Y,
      .z = VDB_CLUSTER_DIM_Z,
    },
  };

  g_vdb.voxel_buffer = buffer_create_storage(0, sizeof(vdb_voxel_t) * VDB_BRICK_COUNT * VDB_BRICK_SIZE);
  g_vdb.info_buffer = buffer_create_uniform(&vdb_info, sizeof(vdb_info_t));
  g_vdb.layer_buffer = buffer_create_uniform_coherent(0, sizeof(vdb_layer_t) * VDB_MAX_LAYER);
}
void vdb_destroy(void) {
  buffer_destroy(&g_vdb.voxel_buffer);
  buffer_destroy(&g_vdb.info_buffer);
  buffer_destroy(&g_vdb.layer_buffer);
}
