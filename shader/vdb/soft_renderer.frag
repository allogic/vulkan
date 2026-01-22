#version 460 core

#extension GL_ARB_shading_language_include : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../math/aabb.glsl"

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

layout (binding = 2) buffer vdb_brick_t {
	uint64_t mask_buffer[];
} vdb_brick;

#include "vdb.glsl"

void main() {
	vec2 ndc = (2.0 * gl_FragCoord.xy / screen_info.resolution) - vec2(1.0);

	vec3 ray_origin = camera_info.position;
	vec3 ray_direction = normalize(vec3(camera_info.view_projection_inv * vec4(ndc, 1.0, 1.0)));
	vec3 box_min = vec3(0.0, 0.0, 0.0);
	vec3 box_max = vec3(VDB_BRICK_SIZE, VDB_BRICK_SIZE, VDB_BRICK_SIZE);

	float t_enter = 0.0;
	float t_exit = 0.0;

	if (!ray_aabb_intersect(
			ray_origin,
			ray_direction,
			box_min,
			box_max,
			t_enter,
			t_exit)) {
		discard;
	}

	float t = max(t_enter, 0.0);

	if (t > camera_info.max_ray_distance) {
		discard;
	}

	float max_distance = min(t_exit - t, camera_info.max_ray_distance - t);

	ray_origin = ray_origin + ray_direction * t;

	vdb_hit_t hit = vdb_hdda_raymarch(ray_origin, ray_direction, max_distance);

	if (!hit.intersect) {
		discard;
	}

	output_color = vec4(1.0, 0.0, 0.0, 1.0);
}