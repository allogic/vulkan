#ifndef VDB_H
#define VDB_H

typedef struct vdb_lod_t {
  int32_t lod;
  vector3_t min_bounds;
  vector3_t max_bounds;
} vdb_lod_t;

typedef struct vdb_hit_t {
  uint8_t intersect;
  ivector3_t brick_position;
  ivector3_t voxel_position;
  vector3_t hit_position;
  // vector3_t face_position;
  // vector3_t normal;
  // vector2_t uv;
  // float ao;
} vdb_hit_t;

typedef struct vdb_brick_t {
  ivector3_t position;
  buffer_t mask_buffer;
} vdb_brick_t;

typedef struct vdb_t {
  struct vdb_brick_t *brick;
  int32_t brick_count;
  ivector3_t dimension;
} vdb_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern vdb_t g_vdb;

void vdb_create(ivector3_t dimension);
void vdb_debug(vector3_t ray_origin, vector3_t ray_direction);
vdb_hit_t vdb_raymarch(vector3_t ray_origin, vector3_t ray_direction, float max_distance, int32_t max_iteration);
void vdb_destroy(void);

int32_t vdb_voxels_per_axis(int8_t lod);
int32_t vdb_total_voxel_count(int8_t lod);
int32_t vdb_word_count(int8_t lod);
int32_t vdb_voxel_index(int8_t lod, ivector3_t voxel_position);
int32_t vdb_voxel_size(int8_t lod);
int32_t vdb_word_index(int32_t voxel_index);

int8_t vdb_voxel_is_solid(int32_t brick_index, int8_t lod, ivector3_t voxel_position);

void vdb_voxel_set(int32_t brick_index, int8_t lod, ivector3_t voxel_position);
void vdb_voxel_clr(int32_t brick_index, int8_t lod, ivector3_t voxel_position);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VDB_H
