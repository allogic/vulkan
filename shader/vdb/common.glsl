#ifndef VDB_COMMON_H
#define VDB_COMMON_H

#define VDB_INFINITE (3.402823466E+38)
#define VDB_EPSILON_4 (1E-4)
#define VDB_EPSILON_5 (1E-5)
#define VDB_EPSILON_6 (1E-6)

#define VDB_LOD_COUNT (5)
#define VDB_LOD_COUNT_PLUS_ONE (VDB_LOD_COUNT + 1)
#define VDB_BRICK_SIZE (32)
#define VDB_CLUSTER_DIM_X (16)
#define VDB_CLUSTER_DIM_Y (8)
#define VDB_CLUSTER_DIM_Z (16)
#define VDB_BRICK_COUNT (VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y * VDB_CLUSTER_DIM_Z)

#define VDB_VOXEL_IS_SOLID_BIT (0x1)

#define VDB_EMPTY_VOXEL (0)

#define VDB_VOXELS_PER_AXIS(LOD) \
	(vdb_axis_voxels_per_lod[LOD])

#define VDB_VOXEL_SIZE(LOD) \
	(1 << LOD)

#define VDB_VOXEL_COUNT(LOD) \
	(vdb_voxel_count_per_lod[LOD])

// TODO: optimize LOD away, just use addition once all multiplications are done!
#define VDB_BRICK_INDEX(LOD, X, Y, Z) \
	(LOD + ((X + (Y * VDB_CLUSTER_DIM_X) + (Z * VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y)) * VDB_LOD_COUNT_PLUS_ONE))

#define VDB_VOXEL_IS_SOLID(VOXEL) \
	((VOXEL & VDB_VOXEL_IS_SOLID_BIT) == VDB_VOXEL_IS_SOLID_BIT)

#define VDB_VOXEL_GET_MATERIAL(VOXEL) \
	((VOXEL >> 8) & 0xFF)

#define VDB_VOXEL_SET_SOLID(VOXEL) \
	(VOXEL | VDB_VOXEL_IS_SOLID_BIT)

const int vdb_axis_voxels_per_lod[VDB_LOD_COUNT_PLUS_ONE] = int[VDB_LOD_COUNT_PLUS_ONE](
	// 256, // 256 >> 0
	// 128, // 256 >> 1
	// 64,  // 256 >> 2
	32,  // 256 >> 3
	16,  // 256 >> 4
	8,   // 256 >> 5
	4,   // 256 >> 6
	2,   // 256 >> 7
	1    // 256 >> 8
);

const int vdb_voxel_count_per_lod[VDB_LOD_COUNT_PLUS_ONE] = int[VDB_LOD_COUNT_PLUS_ONE](
	// 16777216, // 256x256x256
	// 2097152,  // 128x128x128
	// 262144,   // 64x64x64
	32768,    // 32x32x32
	4096,     // 16x16x16
	512,      // 8x8x8
	64,       // 4x4x4
	8,        // 2x2x2
	1         // 1x1x1
);

struct vdb_brick_info_t {
	vec3 aabb_min;
	int reserved0;
	vec3 aabb_max;
	int reserved1;
	int lod;
	int meshlet_offset;
	int meshlet_count;
	int reserved2;
};

struct shared_data_t {
	vec2 position;
	vec2 offsets[2 * 2];
	vec2 size;
};

#endif // VDB_COMMON_H