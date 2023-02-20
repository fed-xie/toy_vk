#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec3 frag_color;

void main() {
    gl_Position = vec4(in_pos.x, -in_pos.y, in_pos.z, 1.0);
	frag_uv = in_uv;
	frag_color = in_normal;
}
