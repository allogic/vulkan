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

extern vdb_t g_vdb;

void vdb_create(ivector3_t cluster_dim, float radius_lod0);
vdb_brick_t *vdb_brick(ivector3_t position);
void vdb_debug(void);
void vdb_destroy(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VDB_H
