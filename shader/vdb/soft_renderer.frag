#version 460 core

#extension GL_ARB_shading_language_include : require
#extension GL_EXT_nonuniform_qualifier : require

#include "fwd.glsl"

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

#include "vdb.glsl"

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