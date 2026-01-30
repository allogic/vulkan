#ifndef VDB_COMMON_H
#define VDB_COMMON_H

// TODO: fix off by one LOD bug! (VDB_MAX_LOD -> VDB_LOD_COUNT)

#define VDB_INFINITE (3.402823466E+38)
#define VDB_EPSILON_4 (1E-4)
#define VDB_EPSILON_5 (1E-5)
#define VDB_EPSILON_6 (1E-6)

#define VDB_MAX_LOD (0x06)
#define VDB_BASE_RES (0x40)
#define VDB_CLUSTER_DIM_X (20)
#define VDB_CLUSTER_DIM_Y (5)
#define VDB_CLUSTER_DIM_Z (20)
#define VDB_BRICK_COUNT (2000)

#define VDB_VOXEL_IS_SOLID_BIT (0x80000000)

#define VDB_EMPTY_VOXEL (0)

#define VDB_VOXELS_PER_AXIS(LOD) \
	(VDB_BASE_RES >> LOD)

#define VDB_VOXEL_SIZE(LOD) \
	(1 << LOD)

#define VDB_VOXEL_COUNT(LOD) \
	(vdb_voxel_count_per_lod[LOD])

#define VDB_BRICK_INDEX(LOD, X, Y, Z) \
	(LOD + ((X + (Y * VDB_CLUSTER_DIM_X) + (Z * VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y)) * VDB_MAX_LOD))

#define VDB_VOXEL_IS_SOLID(VOXEL) \
	((VOXEL & VDB_VOXEL_IS_SOLID_BIT) == VDB_VOXEL_IS_SOLID_BIT)

#define VDB_VOXEL_SET_SOLID(VOXEL) \
	(VOXEL | VDB_VOXEL_IS_SOLID_BIT)

const int vdb_voxel_count_per_lod[VDB_MAX_LOD + 1] = int[VDB_MAX_LOD + 1](
	1, 8, 64, 512, 4096, 32768, 262144
);

#endif // VDB_COMMON_H