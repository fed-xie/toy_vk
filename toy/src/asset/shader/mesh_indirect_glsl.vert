#version 450

struct Camera {
	mat4 view;
	mat4 project;
};

struct VertexAttribute {
	float data[8];
};

struct InstanceData {
	uint vertex_base;
	uint instance_index;
};


layout(set = 0, binding = 0, std430) buffer readonly _VertexAttribute {
	VertexAttribute vertex_attr[];
};
layout(set = 0, binding = 1) buffer readonly _Camera {
	Camera cameras[];
};
layout(set = 0, binding = 2) buffer readonly _Model {
	mat4 model_matrices[];
};
layout(set = 0, binding = 3) buffer readonly _InstanceData {
	InstanceData instance_data[];
};

layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec3 frag_color;

void main() {
	VertexAttribute attr = vertex_attr[instance_data[gl_InstanceIndex].vertex_base + gl_VertexIndex];
	
	vec4 position = vec4(attr.data[0], attr.data[1], attr.data[2], 1.0);
	vec3 normal = vec3(attr.data[3], attr.data[4], attr.data[5]);
	vec2 uv = vec2(attr.data[6], attr.data[7]);
	
	gl_Position = cameras[0].project * cameras[0].view * model_matrices[instance_data[gl_InstanceIndex].instance_index] * position;
	frag_uv = uv;
	if (gl_Position.z != 0.0f)
		frag_color = vec3(gl_Position.x, gl_Position.y, gl_Position.z) / gl_Position.z;
	else
		frag_color = vec3(gl_Position.x, gl_Position.y, gl_Position.z);
}
