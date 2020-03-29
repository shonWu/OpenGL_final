#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

out vec4 viewSpace_coord;
out vec2 vv2texcoord;

void main()
{
	viewSpace_coord = view_matrix * model_matrix * vec4(position, 1.0);
	gl_Position = proj_matrix * viewSpace_coord;
	vv2texcoord = texcoord;
}