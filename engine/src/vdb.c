#include <pch.h>

static void vdb_create_terrain_layer_buffer(void);
static void vdb_create_cluster_info_buffer(void);
static void vdb_create_chunk_info_buffer(void);
static void vdb_create_chunk_mask_buffer(void);

vdb_t g_vdb = {0};

void vdb_create(void) {
  vdb_create_terrain_layer_buffer();
  vdb_create_cluster_info_buffer();
  vdb_create_chunk_info_buffer();
  vdb_create_chunk_mask_buffer();
}
void vdb_destroy(void) {
  buffer_destroy(&g_vdb.terrain_layer_buffer);
  buffer_destroy(&g_vdb.cluster_info_buffer);
  buffer_destroy(&g_vdb.chunk_info_buffer);
  buffer_destroy(&g_vdb.chunk_mask_buffer);
}

int32_t vdb_chunk_position_to_index(ivector3_t chunk_position) {
  return (chunk_position.x) +
         (chunk_position.y * VDB_CLUSTER_DIM_X) +
         (chunk_position.z * VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y);
}
ivector3_t vdb_chunk_index_to_position(int32_t chunk_index) {
  return (ivector3_t){
    chunk_index % VDB_CLUSTER_DIM_X,
    (chunk_index / VDB_CLUSTER_DIM_X) % VDB_CLUSTER_DIM_Y,
    chunk_index / (VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y),
  };
}

static void vdb_create_terrain_layer_buffer(void) {
  g_vdb.terrain_layer_buffer = buffer_create_uniform(0, sizeof(vdb_terrain_layer_t) * VDB_TERRAIN_LAYER_COUNT);
}
static void vdb_create_cluster_info_buffer(void) {
  vdb_cluster_info_t vdb_cluster_info = {
    .cluster_dim = {
      .x = VDB_CLUSTER_DIM_X,
      .y = VDB_CLUSTER_DIM_Y,
      .z = VDB_CLUSTER_DIM_Z,
    },
  };

  g_vdb.cluster_info_buffer = buffer_create_uniform(&vdb_cluster_info, sizeof(vdb_cluster_info_t));
}
static void vdb_create_chunk_info_buffer(void) {
  vdb_chunk_info_t *chunk_info = (vdb_chunk_info_t *)HEAP_ALLOC(sizeof(vdb_chunk_info_t) * VDB_CHUNK_COUNT, 1, 0);

  int32_t chunk_index = 0;
  int32_t chunk_count = VDB_CHUNK_COUNT;

  while (chunk_index < chunk_count) {

    ivector3_t chunk_position = vdb_chunk_index_to_position(chunk_index);

    vector3_t aabb_min = {
      (float)chunk_position.x * VDB_CHUNK_SIZE,
      (float)chunk_position.y * VDB_CHUNK_SIZE,
      (float)chunk_position.z * VDB_CHUNK_SIZE,
    };
    vector3_t aabb_max = {
      aabb_min.x + VDB_CHUNK_SIZE,
      aabb_min.y + VDB_CHUNK_SIZE,
      aabb_min.z + VDB_CHUNK_SIZE,
    };

    chunk_info[chunk_index].chunk_position = chunk_position;
    chunk_info[chunk_index].lod = 0;
    chunk_info[chunk_index].aabb_min = aabb_min;
    chunk_info[chunk_index].aabb_max = aabb_max;

    chunk_index++;
  }

  g_vdb.chunk_info_buffer = buffer_create_storage(chunk_info, sizeof(vdb_chunk_info_t) * VDB_CHUNK_COUNT);

  HEAP_FREE(chunk_info);
}
static void vdb_create_chunk_mask_buffer(void) {
  g_vdb.chunk_mask_buffer = buffer_create_storage(0, sizeof(vdb_chunk_mask_t) * VDB_CHUNK_COUNT);
}
