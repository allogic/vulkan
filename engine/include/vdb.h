#ifndef VDB_H
#define VDB_H

typedef struct vdb_coord_t {
  int32_t x;
  int32_t y;
  int32_t z;
} vdb_coord_t;

typedef struct vdb_voxel_t {
  void *dummy;
} vdb_voxel_t;

typedef struct vdb_brick_t {
  vdb_voxel_t *voxels; // TODO: make linear allocator..
} vdb_brick_t;

typedef struct vdb_node_t {
  vdb_coord_t coord;
  vdb_brick_t brick;
  struct vdb_node_t *next;
} vdb_node_t;

typedef struct vdb_t {
  void *dummy;
} vdb_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

vdb_t vdb_create(int32_t layer_count);
void vdb_destroy(vdb_t *vdb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // VDB_
