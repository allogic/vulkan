#ifndef VDB_H
#define VDB_H

#include "fwd.glsl"

#define VDB_INFINITE  (3.402823466E+38)
#define VDB_EPSILON_4 (1E-4)
#define VDB_EPSILON_5 (1E-5)
#define VDB_EPSILON_6 (1E-6)

#define VDB_MAX_LOD       (0x06)
#define VDB_BASE_RES      (0x40)
#define VDB_CLUSTER_DIM_X (30)
#define VDB_CLUSTER_DIM_Y (2)
#define VDB_CLUSTER_DIM_Z (30)
#define VDB_BRICK_COUNT   (1800)
#define VDB_BRICK_SIZE    (0x49249)

#define VDB_VOXEL_IS_SOLID_MASK (0x80000000)

#define VDB_MORTON_ENCODE(X, Y, Z) \
	vdb_morton_table_x[X] |        \
	vdb_morton_table_y[Y] |        \
	vdb_morton_table_z[Z]

#define VDB_MORTON_DECODE_X(V)  \
	((((V) >>  0) & 1u) << 0) | \
	((((V) >>  3) & 1u) << 1) | \
	((((V) >>  6) & 1u) << 2)

#define VDB_MORTON_DECODE_Y(V) \
	((((V) >> 1) & 1u) << 0) | \
	((((V) >> 4) & 1u) << 1) | \
	((((V) >> 7) & 1u) << 2)

#define VDB_MORTON_DECODE_Z(V)  \
	((((V) >>  2) & 1u) << 0) | \
	((((V) >>  5) & 1u) << 1) | \
	((((V) >>  8) & 1u) << 2)

//#define VDB_BRICK_OFFSET(X, Y, Z) ((VDB_MORTON_ENCODE(X, Y, Z)) * VDB_BRICK_SIZE)
#define VDB_BRICK_OFFSET(X, Y, Z) ((X + (Y * VDB_CLUSTER_DIM_X) + (Z * VDB_CLUSTER_DIM_X * VDB_CLUSTER_DIM_Y)) * VDB_BRICK_SIZE)

/*
const int vdb_morton_table_x[VDB_CLUSTER_DIM_X] = int[VDB_CLUSTER_DIM_X](
	0x0000, 0x0001, 0x0004, 0x0005,
	0x0010, 0x0011, 0x0014, 0x0015
);
const int vdb_morton_table_y[VDB_CLUSTER_DIM_Y] = int[VDB_CLUSTER_DIM_Y](
	0x0000, 0x0002, 0x0008, 0x000A,
	0x0020, 0x0022, 0x0028, 0x002A
);
const int vdb_morton_table_z[VDB_CLUSTER_DIM_Z] = int[VDB_CLUSTER_DIM_Z](
	0x0000, 0x0004, 0x0010, 0x0014,
	0x0040, 0x0044, 0x0050, 0x0054
);
*/

const int vdb_voxel_count_per_lod[VDB_MAX_LOD + 1] = int[VDB_MAX_LOD + 1](
	1, 8, 64, 512, 4096, 32768, 262144
);
const int vdb_voxel_offset_per_lod[VDB_MAX_LOD + 1] = int[VDB_MAX_LOD + 1](
	32768, 4096, 512, 64, 8, 1, 0
);

int vdb_voxels_per_axis(int lod) {
	return VDB_BASE_RES >> lod;
}
int vdb_voxel_size(int lod) {
	return 1 << lod;
}
int vdb_total_voxel_count(int lod) {
	return vdb_voxel_count_per_lod[lod];
}
int vdb_voxel_index(int lod, ivec3 voxel_position) {
	int shift = VDB_MAX_LOD - lod;

	return voxel_position.x + (voxel_position.y << shift) + (voxel_position.z << (shift << 1));
}

bool vdb_voxel_is_solid(int lod, int brick_offset, ivec3 voxel_position) {
	int lod_offset = vdb_voxel_offset_per_lod[lod];
	int voxel_index = vdb_voxel_index(lod, voxel_position);

	return (vdb_cluster.voxel[brick_offset + lod_offset + voxel_index].value & VDB_VOXEL_IS_SOLID_MASK) != 0;
}

void vdb_voxel_set(int lod, int brick_offset, ivec3 voxel_position) {
	int lod_offset = vdb_voxel_offset_per_lod[lod];
	int voxel_index = vdb_voxel_index(lod, voxel_position);

	vdb_cluster.voxel[brick_offset + lod_offset + voxel_index].value |= VDB_VOXEL_IS_SOLID_MASK;
}
void vdb_voxel_clr(int lod, int brick_offset, ivec3 voxel_position) {
	int lod_offset = vdb_voxel_offset_per_lod[lod];
	int voxel_index = vdb_voxel_index(lod, voxel_position);

	vdb_cluster.voxel[brick_offset + lod_offset + voxel_index].value &= ~VDB_VOXEL_IS_SOLID_MASK;
}

vec3 vdb_safe_inverse(vec3 vector) {
	return vec3(
		(vector.x == 0.0) ? VDB_INFINITE : 1.0 / vector.x,
		(vector.y == 0.0) ? VDB_INFINITE : 1.0 / vector.y,
		(vector.z == 0.0) ? VDB_INFINITE : 1.0 / vector.z);
}

vdb_hit_t vdb_hdda_trace(vec3 ray_origin, vec3 ray_direction, float max_distance, int max_iteration) {
	vdb_hit_t hit;
	vdb_lod_t lod_stack[VDB_MAX_LOD];
	vdb_lod_t lod_curr;

	hit.intersect = false;

	bool in_brick = false;
	bool in_voxel = false;
	bool is_solid = false;

	int iter = 0;
	int lod = VDB_MAX_LOD;
	int voxels_per_axis = vdb_voxels_per_axis(lod);
	int voxel_size = vdb_voxel_size(lod);
	int stack_depth = 0;
	int brick_offset = 0;

	float t = 0.0;
	float voxel_size_f = float(voxel_size);
	float t_min = 0.0;
	float t_exit = 0.0;
	float dist = 0.0;

	ivec3 step_direction = ivec3(sign(ray_direction));
	ivec3 voxel_position = ivec3(0);
	ivec3 brick_position = ivec3(floor(ray_origin / VDB_BASE_RES));
	ivec3 prev_brick_position = brick_position;

	vec3 position = ray_origin;
	vec3 prev_position = ray_origin;
	vec3 next_boundary = vec3(0.0);
	vec3 t_max = vec3(0.0);
	vec3 brick_origin = brick_position * VDB_BASE_RES;
	vec3 ray_direction_inv = vdb_safe_inverse(ray_direction);
	vec3 current_min = vec3(0.0);
	vec3 current_max = vec3(0.0);
	vec3 cluster_min = vec3(0.0);
	vec3 cluster_max = vec3(vec3(VDB_CLUSTER_DIM_X, VDB_CLUSTER_DIM_Y, VDB_CLUSTER_DIM_Z) * VDB_BASE_RES);

	brick_offset = VDB_BRICK_OFFSET(brick_position.x, brick_position.y, brick_position.z);

	while (t < max_distance && iter < max_iteration) {

		prev_brick_position = brick_position;
		brick_position = ivec3(floor(position / VDB_BASE_RES));

		vec3 position_clamped = clamp(position, cluster_min, cluster_max - 0.001);
		if (position_clamped != position) {
			break;
		}

		if (brick_position.x != prev_brick_position.x ||
			brick_position.y != prev_brick_position.y ||
			brick_position.z != prev_brick_position.z) {
		
			lod = VDB_MAX_LOD;

			voxels_per_axis = vdb_voxels_per_axis(lod);
			voxel_size = vdb_voxel_size(lod);
			voxel_size_f = float(voxel_size);

			brick_offset = VDB_BRICK_OFFSET(brick_position.x, brick_position.y, brick_position.z);

			stack_depth = 0;
		}

		while (stack_depth > 0) {

			lod_curr = lod_stack[stack_depth - 1];

			if (position.x >= lod_curr.min_bounds.x && position.x < lod_curr.max_bounds.x &&
				position.y >= lod_curr.min_bounds.y && position.y < lod_curr.max_bounds.y &&
				position.z >= lod_curr.min_bounds.z && position.z < lod_curr.max_bounds.z) {
			
				break;
			}

			lod = lod_curr.lod;

			voxels_per_axis = vdb_voxels_per_axis(lod);
			voxel_size = vdb_voxel_size(lod);
			voxel_size_f = float(voxel_size);

			brick_offset = VDB_BRICK_OFFSET(brick_position.x, brick_position.y, brick_position.z);

			stack_depth--;
		}

		brick_origin = brick_position * VDB_BASE_RES;

		voxel_position = ivec3(floor((position - brick_origin) / voxel_size_f));

		voxel_position.x = int(max(0.0, min(float(voxels_per_axis - 1), float(voxel_position.x))));
		voxel_position.y = int(max(0.0, min(float(voxels_per_axis - 1), float(voxel_position.y))));
		voxel_position.z = int(max(0.0, min(float(voxels_per_axis - 1), float(voxel_position.z))));

		in_brick = (brick_position.x >= 0 && brick_position.x < VDB_CLUSTER_DIM_X &&
					brick_position.y >= 0 && brick_position.y < VDB_CLUSTER_DIM_Y &&
					brick_position.z >= 0 && brick_position.z < VDB_CLUSTER_DIM_Z);

		in_voxel = (voxel_position.x >= 0 && voxel_position.x < voxels_per_axis &&
					voxel_position.y >= 0 && voxel_position.y < voxels_per_axis &&
					voxel_position.z >= 0 && voxel_position.z < voxels_per_axis);

		if (in_brick && in_voxel) {

			is_solid = vdb_voxel_is_solid(lod, brick_offset, voxel_position);

			if (is_solid) {

				if (lod > 0) {

					if (stack_depth < VDB_MAX_LOD) {

						current_min = vec3(
							brick_origin.x + float(voxel_position.x) * voxel_size_f,
							brick_origin.y + float(voxel_position.y) * voxel_size_f,
							brick_origin.z + float(voxel_position.z) * voxel_size_f
						);
						current_max = vec3(
							current_min.x + voxel_size_f,
							current_min.y + voxel_size_f,
							current_min.z + voxel_size_f
						);

						lod_stack[stack_depth].lod = lod;
						lod_stack[stack_depth].min_bounds = current_min;
						lod_stack[stack_depth].max_bounds = current_max;

						stack_depth++;
					}

					lod--;

					voxels_per_axis = vdb_voxels_per_axis(lod);
					voxel_size = vdb_voxel_size(lod);
					voxel_size_f = float(voxel_size);

					brick_offset = VDB_BRICK_OFFSET(brick_position.x, brick_position.y, brick_position.z);

					continue;
				}

				t_min = min(t_max.x, min(t_max.y, t_max.z));

				vec3 normal = vec3(0);

				if (abs(t_max.x - t_min) < VDB_EPSILON_5) {
					normal.x = -float(step_direction.x);
				} else if (abs(t_max.y - t_min) < VDB_EPSILON_5) {
					normal.y = -float(step_direction.y);
				} else {
					normal.z = -float(step_direction.z);
				}

				hit.intersect = true;
				hit.brick_position = brick_position;
				hit.voxel_position = voxel_position;
				hit.hit_position = position;
				hit.normal = normalize(normal);

				return hit;
			}
		}

		next_boundary = vec3(
			brick_origin.x + ((step_direction.x > 0) ? float(voxel_position.x + 1) : float(voxel_position.x)) * voxel_size_f,
			brick_origin.y + ((step_direction.y > 0) ? float(voxel_position.y + 1) : float(voxel_position.y)) * voxel_size_f,
			brick_origin.z + ((step_direction.z > 0) ? float(voxel_position.z + 1) : float(voxel_position.z)) * voxel_size_f
		);
		t_max = vec3(
			max((next_boundary.x - position.x) * ray_direction_inv.x, 0.0),
			max((next_boundary.y - position.y) * ray_direction_inv.y, 0.0),
			max((next_boundary.z - position.z) * ray_direction_inv.z, 0.0)
		);

		t_exit = min(t_max.x, min(t_max.y, t_max.z));
		dist = t_exit + VDB_EPSILON_6;

		if (t_exit <= 0.0) {
			dist = VDB_EPSILON_4;
		}

		if ((t + dist) >= max_distance) {
			break;
		}

		t += dist;

		if ((iter & 127) == 0) {
			position = ray_origin + ray_direction * t;
		} else {
			position += ray_direction * dist;
		}

		iter++;
	}

	return hit;
}

#endif // VDB_H