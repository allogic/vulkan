#ifndef VDB_COMMON_H
#define VDB_COMMON_H

#define VDB_LOD_COUNT (4)
#define VDB_LOD_COUNT_PLUS_ONE (VDB_LOD_COUNT + 1)
#define VDB_BRICK_SIZE (32)
#define VDB_CLUSTER_DIM_X (3)
#define VDB_CLUSTER_DIM_Y (1)
#define VDB_CLUSTER_DIM_Z (3)
#define VDB_BRICK_COUNT (9)

#define VDB_VOXEL_IS_SOLID_BIT (0x1)

#define VDB_EMPTY_VOXEL (0)

#define VDB_VOXEL_IS_SOLID(VOXEL) \
	((VOXEL & VDB_VOXEL_IS_SOLID_BIT) == VDB_VOXEL_IS_SOLID_BIT)

#define VDB_VOXEL_GET_MATERIAL(VOXEL) \
	((VOXEL >> 8) & 0xFF)

#define VDB_VOXEL_SET_SOLID(VOXEL) \
	(VOXEL | VDB_VOXEL_IS_SOLID_BIT)

struct vdb_brick_info_t {
	ivec3 brick_position;
	int lod;
	vec3 aabb_min;
	int reserved0;
	vec3 aabb_max;
	int reserved1;
};

struct vdb_shared_data_t {
	ivec3 brick_position;
	int brick_index;
};

#endif // VDB_COMMON_H