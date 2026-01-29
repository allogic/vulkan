#ifndef VDB_FWD_H
#define VDB_FWD_H

struct vdb_voxel_t {
  uint value;
};
struct vdb_lod_t {
  int lod;
  vec3 min_bounds;
  vec3 max_bounds;
};
struct vdb_hit_t {
  bool intersect;
  ivec3 brick_position;
  ivec3 voxel_position;
  vec3 hit_position;
  vec3 normal;
  // vec3 face_position;
  // vec2 uv;
  // float ao;
};

#endif // VDB_FWD_H