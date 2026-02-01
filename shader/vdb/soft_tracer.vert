#version 460 core

#extension GL_ARB_shading_language_include : require
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 vertex_position;

void main() {
	gl_Position = vec4(vertex_position, 1.0);
}