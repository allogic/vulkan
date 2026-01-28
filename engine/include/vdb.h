#ifndef VDB_H
#define VDB_H

#define VDB_MAX_LOD_LEVEL (0x6)
#define VDB_BASE_RES (0x40)
#define VDB_BITS_PER_WORD (0x20)
#define VDB_MAX_TERRAIN_MODIFIER (0x10)

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

typedef struct terrain_layer_t {
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
} terrain_layer_t;

STATIC_ASSERT(sizeof(terrain_layer_t) % 4 == 0);

typedef struct vdb_info_t {
  ivector3_t dimension;
  int32_t reserved0;
} vdb_info_t;

STATIC_ASSERT(sizeof(vdb_info_t) % 4 == 0);

typedef struct vdb_brick_t {
  ivector3_t position;
  buffer_t mask_buffer;
} vdb_brick_t;

typedef struct vdb_t {
  struct vdb_brick_t *brick;
  int32_t brick_count;
  ivector3_t dimension;
  buffer_t info_buffer;
  buffer_t terrain_layer_buffer;
} vdb_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern vdb_t g_vdb;

void vdb_create(ivector3_t dimension);
void vdb_destroy(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VDB_H
