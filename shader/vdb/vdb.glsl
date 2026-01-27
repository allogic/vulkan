#ifndef VDB_H
#define VDB_H

#include "../math/constants.glsl"

#define VDB_MAX_LOD_LEVEL (0x6)
#define VDB_BASE_RES (0x40)
#define VDB_BITS_PER_WORD (0x20)

const int vdb_word_offset[7] = int[7](
	0,
	8192,
	8192 + 1024,
	8192 + 1024 + 128,
	8192 + 1024 + 128 + 16,
	8192 + 1024 + 128 + 16 + 2,
	8192 + 1024 + 128 + 16 + 2 + 1
);
const int vdb_voxel_count[7] = int[7](
	1 * 1 * 1,
	2 * 2 * 2,
	4 * 4 * 4,
	8 * 8 * 8,
	16 * 16 * 16,
	32 * 32 * 32,
	64 * 64 * 64
);

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
	//vec3 face_position;
	//vec2 uv;
	float ao;
};

int vec_to_index(ivec3 position, ivec3 size) {
	return (position.x) + (position.y * size.x) + (position.z * size.x * size.y);
}

int vdb_voxels_per_axis(int lod) {
	return VDB_BASE_RES >> lod;
}
int vdb_total_voxel_count(int lod) {
	return vdb_voxel_count[lod];
}
int vdb_word_count(int lod) {
	return (vdb_voxel_count[lod] + (VDB_BITS_PER_WORD - 1)) >> 5;
}
int vdb_voxel_index(int lod, ivec3 voxel_position) {
	switch (lod) {
		case 0: return voxel_position.x + (voxel_position.y << 6) + (voxel_position.z << 12);
		case 1: return voxel_position.x + (voxel_position.y << 5) + (voxel_position.z << 10);
		case 2: return voxel_position.x + (voxel_position.y << 4) + (voxel_position.z << 8);
		case 3: return voxel_position.x + (voxel_position.y << 3) + (voxel_position.z << 6);
		case 4: return voxel_position.x + (voxel_position.y << 2) + (voxel_position.z << 4);
		case 5: return voxel_position.x + (voxel_position.y << 1) + (voxel_position.z << 2);
		case 6: return 0;
	}

	return -1;
}
int vdb_voxel_size(int lod) {
	return 1 << lod;
}
int vdb_word_index(int voxel_index) {
	return voxel_index >> 5;
}

bool vdb_voxel_is_solid(int brick_index, int lod, ivec3 voxel_position) {
	int i = vdb_voxel_index(lod, voxel_position);
	int word = vdb_word_offset[lod] + vdb_word_index(i);

	return bool(vdb_brick[brick_index].mask_buffer[word] & (1u << (i & 31)));
}

void vdb_voxel_set(int brick_index, int lod, ivec3 voxel_position) {
	int i = vdb_voxel_index(lod, voxel_position);
	int word = vdb_word_offset[lod] + vdb_word_index(i);

	atomicOr(vdb_brick[brick_index].mask_buffer[word], (1u << (i & 31)));
}
void vdb_voxel_clr(int brick_index, int lod, ivec3 voxel_position) {
	int i = vdb_voxel_index(lod, voxel_position);
	int word = vdb_word_offset[lod] + vdb_word_index(i);

	atomicAnd(vdb_brick[brick_index].mask_buffer[word], ~(1u << (i & 31)));
}

float vdb_voxel_ao(int brick_index, int lod, ivec3 p, ivec3 s1, ivec3 s2, ivec3 c) {
	int o1 = int(vdb_voxel_is_solid(brick_index, lod, p + s1));
	int o2 = int(vdb_voxel_is_solid(brick_index, lod, p + s2));
	int oc = int(vdb_voxel_is_solid(brick_index, lod, p + c));

	if (o1 == 1 && o2 == 1) {
		return 0.0;
	}

	return 1.0 - float(o1 + o2 + oc) / 3.0;
}

vdb_hit_t vdb_hdda_raymarch(vec3 ray_origin, vec3 ray_direction, ivec3 vdb_dimension, float max_distance, int max_iteration) {
	vdb_hit_t hit;
	vdb_lod_t lod_stack[VDB_MAX_LOD_LEVEL];

	hit.intersect = false;

	bool in_brick = false;
	bool in_voxel = false;
	bool is_solid = false;

	int iter = 0;
	int lod = VDB_MAX_LOD_LEVEL;
	int voxels_per_axis = vdb_voxels_per_axis(lod);
	int voxel_size = vdb_voxel_size(lod);
	int brick_index = -1;
	int stack_depth = 0;

	ivec3 step_direction = ivec3(sign(ray_direction));
	ivec3 voxel_position = ivec3(0);
	ivec3 brick_position = ivec3(floor(ray_origin / VDB_BASE_RES));
	ivec3 prev_brick_position = brick_position;

	vec3 position = ray_origin;
	vec3 prev_position = ray_origin;
	vec3 next_boundary = vec3(0.0);
	vec3 t_max = vec3(0.0);
	vec3 brick_origin = brick_position * VDB_BASE_RES;
	vec3 ray_direction_inv = vec3(
		(ray_direction.x == 0.0) ? FLT_MAX : 1.0 / ray_direction.x,
		(ray_direction.y == 0.0) ? FLT_MAX : 1.0 / ray_direction.y,
		(ray_direction.z == 0.0) ? FLT_MAX : 1.0 / ray_direction.z
	);
	vec3 current_min = vec3(0.0);
	vec3 current_max = vec3(0.0);
	vec3 cluster_min = vec3(0.0);
	vec3 cluster_max = vec3(vdb_dimension * VDB_BASE_RES);

	float t = 0.0;

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
		
			lod = VDB_MAX_LOD_LEVEL;

			voxels_per_axis = vdb_voxels_per_axis(lod);
			voxel_size = vdb_voxel_size(lod);

			stack_depth = 0;
		}

		while (stack_depth > 0) {

			if (position.x >= lod_stack[stack_depth - 1].min_bounds.x && position.x < lod_stack[stack_depth - 1].max_bounds.x &&
				position.y >= lod_stack[stack_depth - 1].min_bounds.y && position.y < lod_stack[stack_depth - 1].max_bounds.y &&
				position.z >= lod_stack[stack_depth - 1].min_bounds.z && position.z < lod_stack[stack_depth - 1].max_bounds.z) {
			
				break;
			}

			lod = lod_stack[stack_depth - 1].lod;

			voxels_per_axis = vdb_voxels_per_axis(lod);
			voxel_size = vdb_voxel_size(lod);

			stack_depth--;
		}

		brick_origin = brick_position * VDB_BASE_RES;

		voxel_position = ivec3(floor((position - brick_origin) / voxel_size));

		voxel_position.x = int(max(0.0, min(float(voxels_per_axis - 1), float(voxel_position.x))));
		voxel_position.y = int(max(0.0, min(float(voxels_per_axis - 1), float(voxel_position.y))));
		voxel_position.z = int(max(0.0, min(float(voxels_per_axis - 1), float(voxel_position.z))));

		in_brick = (brick_position.x >= 0 && brick_position.x < vdb_dimension.x &&
					brick_position.y >= 0 && brick_position.y < vdb_dimension.y &&
					brick_position.z >= 0 && brick_position.z < vdb_dimension.z);

		in_voxel = (voxel_position.x >= 0 && voxel_position.x < voxels_per_axis &&
					voxel_position.y >= 0 && voxel_position.y < voxels_per_axis &&
					voxel_position.z >= 0 && voxel_position.z < voxels_per_axis);

		brick_index = vec_to_index(brick_position, vdb_dimension);

		if (in_brick && in_voxel) {

			is_solid = vdb_voxel_is_solid(brick_index, lod, voxel_position);

			if (is_solid) {

				if (lod > 0) {

					if (stack_depth < VDB_MAX_LOD_LEVEL) {

						current_min = vec3(
							brick_origin.x + float(voxel_position.x) * float(voxel_size),
							brick_origin.y + float(voxel_position.y) * float(voxel_size),
							brick_origin.z + float(voxel_position.z) * float(voxel_size)
						);
						current_max = vec3(
							current_min.x + float(voxel_size),
							current_min.y + float(voxel_size),
							current_min.z + float(voxel_size)
						);

						lod_stack[stack_depth].lod = lod;
						lod_stack[stack_depth].min_bounds = current_min;
						lod_stack[stack_depth].max_bounds = current_max;

						stack_depth++;
					}

					lod--;

					voxels_per_axis = vdb_voxels_per_axis(lod);
					voxel_size = vdb_voxel_size(lod);

					continue;
				}

				float min_t = min(t_max.x, min(t_max.y, t_max.z));

				vec3 normal = vec3(0);

				if (abs(t_max.x - min_t) < EPSILON_5) {
					normal.x = -float(step_direction.x);
				} else if (abs(t_max.y - min_t) < EPSILON_5) {
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
			brick_origin.x + ((step_direction.x > 0) ? float(voxel_position.x + 1) : float(voxel_position.x)) * float(voxel_size),
			brick_origin.y + ((step_direction.y > 0) ? float(voxel_position.y + 1) : float(voxel_position.y)) * float(voxel_size),
			brick_origin.z + ((step_direction.z > 0) ? float(voxel_position.z + 1) : float(voxel_position.z)) * float(voxel_size)
		);
		t_max = vec3(
			max((next_boundary.x - position.x) * ray_direction_inv.x, 0.0),
			max((next_boundary.y - position.y) * ray_direction_inv.y, 0.0),
			max((next_boundary.z - position.z) * ray_direction_inv.z, 0.0)
		);

		float t_exit = min(t_max.x, min(t_max.y, t_max.z));
		float dist = t_exit + EPSILON_6;

		if (t_exit <= 0.0) {
			dist = EPSILON_4;
		}

		if ((t + dist) >= max_distance) {
			break;
		}

		t += dist;

		if ((iter % 100) == 0) {
			position = ray_origin + ray_direction * t;
		} else {
			position += ray_direction * dist;
		}

		iter++;
	}

	return hit;
}

#endif // VDB_H