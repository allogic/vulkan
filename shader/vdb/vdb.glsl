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

struct vdb_hit_t {
	bool intersect;
	ivec3 index;
	vec3 hit_position;
	vec3 face_position;
	vec3 normal;
	vec2 uv;
	float ao;
};

int vdb_voxels_per_axis(int lod) {
	return VDB_BASE_RES >> lod;
}
int vdb_total_voxel_count(int lod) {
	int n = vdb_voxels_per_axis(lod);

	return n * n * n;
}
int vdb_word_count(int lod) {
	int n = vdb_total_voxel_count(lod);

	return (n + (VDB_BITS_PER_WORD - 1)) >> 5;
}
int vdb_voxel_index(int lod, ivec3 index) {
	int n = vdb_voxels_per_axis(lod);

	return index.x + index.y * n + index.z * n * n;
}
int vdb_voxel_size(int lod) {
	return 1 << lod;
}

uint vdb_word_index(int voxel_index) {
	return voxel_index >> 5;
}
uint vdb_bit_mask(int voxel_index) {
	return 1u << (voxel_index & 31);
}

bool vdb_voxel_is_solid(int brick_index, int lod, ivec3 index) {
	int i = vdb_voxel_index(lod, index);

	uint word = vdb_word_offset[lod] + vdb_word_index(i);
	uint bit = vdb_bit_mask(i);

	return (vdb_brick[brick_index].mask_buffer[word] & bit) != 0u;
}

void vdb_voxel_set(int brick_index, int lod, ivec3 index) {
	int i = vdb_voxel_index(lod, index);

	uint word = vdb_word_offset[lod] + vdb_word_index(i);
	uint bit = vdb_bit_mask(i);

	atomicOr(vdb_brick[brick_index].mask_buffer[word], bit);
}
void vdb_voxel_clr(int brick_index, int lod, ivec3 index) {
	int i = vdb_voxel_index(lod, index);

	uint word = vdb_word_offset[lod] + vdb_word_index(i);
	uint bit = ~vdb_bit_mask(i);

	atomicAnd(vdb_brick[brick_index].mask_buffer[word], bit);
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

vdb_hit_t vdb_hdda_raymarch(vec3 ray_origin, vec3 ray_direction, float max_distance) {
	vdb_hit_t hit;

	hit.intersect = false;
	hit.index = ivec3(0);
	hit.hit_position = vec3(0.0);
	hit.face_position = vec3(0.0);
	hit.normal = vec3(0.0);
	hit.uv = vec2(0.0);
	hit.ao = 1.0;

	// TODO
	int lod = 0; // VDB_MAX_LOD_LEVEL;
	int hit_axis = -1;

	float voxel_size = float(vdb_voxel_size(lod));
	int voxel_count = vdb_total_voxel_count(lod);

	ivec3 brick = ivec3(floor(ray_origin / VDB_BASE_RES));
	ivec3 index = ivec3(floor(ray_origin / voxel_size));
	ivec3 step = ivec3(sign(ray_direction));

	vec3 direction_inv = vec3(
		ray_direction.x != 0.0 ? 1.0 / ray_direction.x : FLT_MAX,
		ray_direction.y != 0.0 ? 1.0 / ray_direction.y : FLT_MAX,
		ray_direction.z != 0.0 ? 1.0 / ray_direction.z : FLT_MAX);

	vec3 next_boundary = vec3(
		(step.x > 0 ? float(index.x + 1) : float(index.x)) * voxel_size,
		(step.y > 0 ? float(index.y + 1) : float(index.y)) * voxel_size,
		(step.z > 0 ? float(index.z + 1) : float(index.z)) * voxel_size);

	vec3 t_max = (next_boundary - ray_origin) * direction_inv;

	vec3 t_delta = vec3(
		abs(voxel_size * direction_inv.x),
		abs(voxel_size * direction_inv.y),
		abs(voxel_size * direction_inv.z));

	float t = 0.0;

	while (t < max_distance) {

		while (1) {

			if (index.x < 0 || index.y < 0 || index.z < 0 ||
				index.x >= voxel_count || index.y >= voxel_count || index.z >= voxel_count) {

				break;
			}

			bool solid = vdb_voxel_is_solid(lod, index);

			if (solid && lod > 0) {

				lod--;

				float advance = min(t_max.x, min(t_max.y, t_max.z));

				ray_origin += ray_direction * (advance + EPSILON_6);

				t += advance + EPSILON_6;

				voxel_size = float(vdb_voxel_size(lod));
				voxel_count = vdb_total_voxel_count(lod);

				brick = ivec3(floor(ray_origin / VDB_BASE_RES));
				index = ivec3(floor(ray_origin / voxel_size));
				step = ivec3(sign(ray_direction));

				direction_inv = vec3(
					ray_direction.x != 0.0 ? 1.0 / ray_direction.x : FLT_MAX,
					ray_direction.y != 0.0 ? 1.0 / ray_direction.y : FLT_MAX,
					ray_direction.z != 0.0 ? 1.0 / ray_direction.z : FLT_MAX);

				next_boundary = vec3(
					(step.x > 0 ? float(index.x + 1) : float(index.x)) * voxel_size,
					(step.y > 0 ? float(index.y + 1) : float(index.y)) * voxel_size,
					(step.z > 0 ? float(index.z + 1) : float(index.z)) * voxel_size);

				t_max = (next_boundary - ray_origin) * direction_inv;

				t_delta = vec3(
					abs(voxel_size * direction_inv.x),
					abs(voxel_size * direction_inv.y),
					abs(voxel_size * direction_inv.z));

				continue;
			}

			if (solid && lod == 0) {

				vec3 hit_position = ray_origin + ray_direction * t;

				vec3 normal = vec3(0.0);
				normal[hit_axis] = -float(step[hit_axis]);

				float t_hit;

				if (hit_axis == 0)
					t_hit = t_max.x - t_delta.x;
				else if (hit_axis == 1)
					t_hit = t_max.y - t_delta.y;
				else
					t_hit = t_max.z - t_delta.z;

				vec3 face_position = ray_origin + ray_direction * t_hit;

				vec3 local = (face_position - vec3(index) * voxel_size) / voxel_size;
				local = clamp(local, 0.0, 1.0);

				vec2 uv;

				// TODO make switch!
				if (hit_axis == 0)      uv = local.yz;
				else if (hit_axis == 1) uv = local.xz;
				else                    uv = local.xy;
			
				// Flip ONLY U based on face direction
				if (step[hit_axis] > 0)
					uv.x = 1.0 - uv.x;

				// --- Ambient Occlusion
				vec3 N = normal;
				vec3 T, B;

				if (abs(N.x) > 0.5) {
					T = vec3(0, 1, 0);
					B = vec3(0, 0, 1);
				} else if (abs(N.y) > 0.5) {
					T = vec3(1, 0, 0);
					B = vec3(0, 0, 1);
				} else {
					T = vec3(1, 0, 0);
					B = vec3(0, 1, 0);
				}

				ivec3 s1 = ivec3(sign(T));
				ivec3 s2 = ivec3(sign(B));
				ivec3 p = index;

				float ao0 = vdb_voxel_ao(lod, p, s1, s2, s1 + s2);
				float ao1 = vdb_voxel_ao(lod, p, -s1, s2, -s1 + s2);
				float ao2 = vdb_voxel_ao(lod, p, s1, -s2, s1 - s2);
				float ao3 = vdb_voxel_ao(lod, p, -s1, -s2, -s1 - s2);

				float ao = mix(
					mix(ao0, ao1, uv.x),
					mix(ao2, ao3, uv.x),
					uv.y);

				hit.intersect = true;
				hit.index = index;
				hit.hit_position = hit_position;
				hit.face_position = face_position;
				hit.normal = normal;
				hit.uv = uv;
				hit.ao = ao;

				return hit;
			}

			float t_next = min(t_max.x, min(t_max.y, t_max.z));

			int next_axis = -1;

			if (t_max.x <= t_max.y && t_max.x <= t_max.z) {
				next_axis = 0;
			} else if (t_max.y <= t_max.z) {
				next_axis = 1;
			} else {
				next_axis = 2;
			}

			bool step_x = abs(t_max.x - t_next) < EPSILON_6;
			bool step_y = abs(t_max.y - t_next) < EPSILON_6;
			bool step_z = abs(t_max.z - t_next) < EPSILON_6;

			if (step_x) {
				index.x += step.x;
				t_max.x += t_delta.x;
			}

			if (step_y) {
				index.y += step.y;
				t_max.y += t_delta.y;
			}

			if (step_z) {
				index.z += step.z;
				t_max.z += t_delta.z;
			}

			hit_axis = next_axis;

			t = t_next + EPSILON_6;
		}
	}

	return hit;
}

#endif // VDB_H