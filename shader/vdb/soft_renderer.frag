#version 460 core

#extension GL_ARB_shading_language_include : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../vdb/common.glsl"

#define VDB_GET_VOXEL(BRICK_INDEX, VOXEL_POSITION) \
	(uint(texelFetch(vdb_brick[BRICK_INDEX], VOXEL_POSITION, 0).r))

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

layout (location = 0) out vec4 output_color;

layout (binding = 0) uniform camera_info_t {
	vec3 position;
	float max_ray_distance;
	mat4 view;
	mat4 projection;
	mat4 view_projection;
	mat4 view_projection_inv;
} camera_info;
layout (binding = 1) uniform screen_info_t {
	vec2 resolution;
} screen_info;
layout (binding = 2) uniform vdb_info_t {
	ivec3 cluster_dim;
} vdb_info;
layout (binding = 3) uniform usampler3D vdb_brick[];

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
	int lod = 0; // VDB_MAX_LOD;
	int voxels_per_axis = VDB_VOXELS_PER_AXIS(lod);
	int voxel_size = VDB_VOXEL_SIZE(lod);
	int stack_depth = 0;
	int brick_index = 0;

	uint voxel = 0;

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

	brick_index = VDB_BRICK_INDEX(lod, brick_position.x, brick_position.y, brick_position.z);

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

			voxels_per_axis = VDB_VOXELS_PER_AXIS(lod);
			voxel_size = VDB_VOXEL_SIZE(lod);
			voxel_size_f = float(voxel_size);

			brick_index = VDB_BRICK_INDEX(lod, brick_position.x, brick_position.y, brick_position.z);

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

			voxels_per_axis = VDB_VOXELS_PER_AXIS(lod);
			voxel_size = VDB_VOXEL_SIZE(lod);
			voxel_size_f = float(voxel_size);

			brick_index = VDB_BRICK_INDEX(lod, brick_position.x, brick_position.y, brick_position.z);

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

			voxel = VDB_GET_VOXEL(brick_index, voxel_position);

			is_solid = VDB_VOXEL_IS_SOLID(voxel);

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

					voxels_per_axis = VDB_VOXELS_PER_AXIS(lod);
					voxel_size = VDB_VOXEL_SIZE(lod);
					voxel_size_f = float(voxel_size);

					brick_index = VDB_BRICK_INDEX(lod, brick_position.x, brick_position.y, brick_position.z);

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

void main() {
	vec2 frag_uv = (gl_FragCoord.xy + 0.5) / screen_info.resolution;
	vec2 ndc = frag_uv * 2.0 - 1.0;

	vec4 position = camera_info.view_projection_inv * vec4(ndc, 1.0, 1.0);

	vec3 world_position = position.xyz / position.w;
	vec3 ray_origin = camera_info.position;
	vec3 ray_direction = normalize(world_position - ray_origin);

	vdb_hit_t hit = vdb_hdda_trace(ray_origin, ray_direction, camera_info.max_ray_distance, 10000);

	if (!hit.intersect) {
		discard;
	}

	output_color = vec4(hit.normal, 1.0);
}