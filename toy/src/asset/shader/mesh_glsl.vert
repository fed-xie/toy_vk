#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec3 frag_color;

layout(set = 0, binding = 0) uniform ViewProject {
	mat4 view;
	mat4 project;
} vp;

layout(set = 0, binding = 1) uniform Model {
	mat4 model;
} m;

void main() {
	gl_Position = vp.project * vp.view * m.model * vec4(in_pos, 1.0);
	frag_color = vec3(gl_Position.x, gl_Position.y, gl_Position.z) / gl_Position.z;
}
