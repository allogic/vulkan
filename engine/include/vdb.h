#ifndef VDB_H
#define VDB_H

#define VDB_LOD_COUNT (8)
#define VDB_LOD_COUNT_PLUS_ONE (VDB_LOD_COUNT + 1)
#define VDB_BASE_RES (256)
#define VDB_CLUSTER_DIM_X (5)
#define VDB_CLUSTER_DIM_Y (5)
#define VDB_CLUSTER_DIM_Z (5)
#define VDB_BRICK_COUNT (VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y * VDB_CLUSTER_DIM_Z)

#define VDB_VOXELS_PER_AXIS(LOD) \
  (g_vdb_axis_voxels_per_lod[LOD])

#define VDB_MAX_LAYER (16)

#define VDB_NOISE_TYPE_CELLULAR (0x0)

#define VDB_CELLULAR_TYPE_0 (0x0)
#define VDB_CELLULAR_TYPE_1 (0x1)

typedef struct cellular_noise_args_t {
  vector4_t offset;
  int32_t type;
  float u;
  float v;
  int32_t reserved0;
} cellular_noise_args_t;
typedef struct curl_noise_args_t {
  vector4_t offset;
  int32_t type;
  int32_t reserved0;
  int32_t reserved1;
  int32_t reserved2;
} curl_noise_args_t;
typedef struct fbm_noise_args_t {
  vector4_t offset;
  int32_t type;
  float scale;
  float tile_length;
  float amplitude;
  float lacunarity;
  int32_t octaves;
  int32_t reserved0;
  int32_t reserved1;
} fbm_noise_args_t;
typedef struct gradient_noise_args_t {
  vector4_t offset;
  int32_t type;
  float tile_length;
  int32_t reserved0;
  int32_t reserved1;
} gradient_noise_args_t;
typedef struct perlin_noise_args_t {
  vector4_t offset;
  int32_t type;
  int32_t reserved0;
  int32_t reserved1;
  int32_t reserved2;
} perlin_noise_args_t;
typedef struct simplex_noise_args_t {
  vector4_t offset;
  int32_t type;
  int32_t reserved0;
  int32_t reserved1;
  int32_t reserved2;
} simplex_noise_args_t;

STATIC_ASSERT(sizeof(cellular_noise_args_t) % 4 == 0);
STATIC_ASSERT(sizeof(curl_noise_args_t) % 4 == 0);
STATIC_ASSERT(sizeof(fbm_noise_args_t) % 4 == 0);
STATIC_ASSERT(sizeof(gradient_noise_args_t) % 4 == 0);
STATIC_ASSERT(sizeof(perlin_noise_args_t) % 4 == 0);
STATIC_ASSERT(sizeof(simplex_noise_args_t) % 4 == 0);

typedef struct vdb_layer_t {
  cellular_noise_args_t cellular_noise_args;
  curl_noise_args_t curl_noise_args;
  fbm_noise_args_t fbm_noise_args;
  gradient_noise_args_t gradient_noise_args;
  perlin_noise_args_t perlin_noise_args;
  simplex_noise_args_t simplex_noise_args;
  int32_t noise_type;
  float scale;
  int32_t reserved0;
  int32_t reserved1;
} vdb_layer_t;
typedef struct vdb_info_t {
  ivector3_t cluster_dim;
  int32_t reserved0;
} vdb_info_t;

STATIC_ASSERT(sizeof(vdb_layer_t) % 4 == 0);
STATIC_ASSERT(sizeof(vdb_info_t) % 4 == 0);

typedef struct vdb_brick_t {
  VkImage image;
  VkDeviceMemory device_memory;
  VkImageView image_view[VDB_LOD_COUNT_PLUS_ONE];
} vdb_brick_t;

typedef struct vdb_t {
  vdb_brick_t *brick;
  VkSampler brick_sampler;
  buffer_t info_buffer;
  buffer_t layer_buffer;
} vdb_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern vdb_t g_vdb;

extern int32_t g_vdb_axis_voxels_per_lod[VDB_LOD_COUNT_PLUS_ONE];

void vdb_create(void);
void vdb_destroy(void);

int32_t vdb_brick_position_to_index(ivector3_t brick_position);
ivector3_t vdb_brick_index_to_position(int32_t brick_index);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VDB_H
