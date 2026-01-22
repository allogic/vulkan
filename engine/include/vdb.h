#ifndef VDB_H
#define VDB_H

typedef struct vdb_hit_t {
  int8_t hit;
  ivector3_t position;
} vdb_hit_t;

typedef struct vdb_brick_t {
  buffer_t mask_buffer;
} vdb_brick_t;

typedef struct vdb_record_t {
  struct vdb_record_t *next;
  ivector3_t position;
  vdb_brick_t brick;
} vdb_record_t;

typedef struct vdb_t {
  struct vdb_record_t **table;
  int32_t table_size;
  int32_t table_count;
  int32_t record_count;
} vdb_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

vdb_t vdb_create(void);
void vdb_insert(vdb_t *vdb, ivector3_t position);
void vdb_remove(vdb_t *vdb, ivector3_t position);
vdb_brick_t *vdb_brick(vdb_t *vdb, ivector3_t position);
void vdb_destroy(vdb_t *vdb);

vdb_brick_t vdb_brick_create(void);
void vdb_brick_destroy(vdb_brick_t *brick);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VDB_H
