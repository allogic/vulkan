#ifndef VDB_H
#define VDB_H

typedef struct vdb_hit_t {
  int8_t hit;
  int8_t lod;
  ivector3_t position;
} vdb_hit_t;

typedef struct vdb_brick_t {
  int32_t grid_size;
  uint64_t mask[VDB_COMPUTE_BRICK_WORDS(0x20)];
} vdb_brick_t;

typedef struct vdb_record_t {
  struct vdb_record_t *next;
  ivector3_t position;
  vdb_brick_t brick;
} vdb_record_t;

typedef struct vdb_bucket_t {
  struct vdb_bucket_t *prev;
  struct vdb_bucket_t *next;
} vdb_bucket_t;

typedef struct vdb_t {
  struct vdb_bucket_t **bucket;
  int32_t bucket_size;
  int32_t bucket_count;
  int32_t record_count;
} vdb_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

vdb_t vdb_create(void);
void vdb_insert(vdb_t *vdb, ivector3_t position);
void vdb_remove(vdb_t *vdb, ivector3_t position);
void vdb_print(vdb_t *vdb);
void vdb_destroy(vdb_t *vdb);

vdb_brick_t vdb_brick_create(void);
vdb_hit_t vdb_brick_raymarch(vdb_brick_t *brick, vector3_t ray_origin, vector3_t ray_direction, float max_distance);
void vdb_brick_debug(vdb_brick_t *brick, ivector3_t position);
void vdb_brick_destroy(vdb_brick_t *brick);

__forceinline int32_t vdb_brick_index(int32_t x, int32_t y, int32_t z);
__forceinline int8_t vdb_brick_get(vdb_brick_t *brick, int32_t x, int32_t y, int32_t z);
__forceinline void vdb_brick_set(vdb_brick_t *brick, int32_t x, int32_t y, int32_t z);
__forceinline void vdb_brick_clear(vdb_brick_t *brick, int32_t x, int32_t y, int32_t z);

#ifdef __cplusplus
}
#endif // __cplusplus

#include <vdb.inl>

#endif // VDB_
