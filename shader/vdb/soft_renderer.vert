#version 460 core

#extension GL_ARB_shading_language_include : require
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 vertex_position;

void main() {
	vec4 clip_position = vec4(vertex_position, 1.0);

	gl_Position = clip_position;
}