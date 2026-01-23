#ifndef VDB_H
#define VDB_H

typedef struct vdb_brick_t {
  struct vdb_brick_t *next;
  ivector3_t position;
  buffer_t mask_buffer;
} vdb_brick_t;

typedef struct vdb_t {
  struct vdb_brick_t **table;
  int32_t table_size;
  int32_t table_count;
  int32_t brick_count;
  ivector3_t cluster_dim;
} vdb_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

vdb_t vdb_create(ivector3_t cluster_dim, float radius_lod0);
vdb_brick_t *vdb_brick(vdb_t *vdb, ivector3_t position);
void vdb_debug(vdb_t *vdb);
void vdb_destroy(vdb_t *vdb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VDB_H
