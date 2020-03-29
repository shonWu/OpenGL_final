#version 410 core

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

//uniform mat4 um4mv;
//uniform mat4 um4p;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

//©T©wªº
uniform mat4 m_matrix;
uniform mat4 view_matrix;

//for shadow
uniform mat4 shadow_matrix;
uniform vec4 plane_normal;


out VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
	vec3 V;
    vec3 H; // eye space halfway vector

    vec2 texcoord;

	vec3 normal;
} vertexData;

out vec4 shadow_coord;
out vec4 viewSpace_coord;


// Position of light
uniform vec3 light_pos = vec3(-3.0, 200.0, -9.0);//vec3(25.0, 250.0, 5.0);

void main()
{
	// Calculate view-space coordinate
	vec4 P = view_matrix * m_matrix * vec4(iv3vertex, 1.0);

	// Calculate normal in view-space
	//vertexData.N = mat3(mv_matrix) * iv3normal;
	vertexData.N = mat3(view_matrix*m_matrix) * iv3normal;

	// Calculate light vector
	vertexData.L = light_pos - P.xyz;

	// Calculate view vector
	vertexData.V = -P.xyz;
	
	//shadow coordinate
	shadow_coord = shadow_matrix * vec4(iv3vertex,1.0);

	gl_Position = proj_matrix * mv_matrix * vec4(iv3vertex, 1.0);
	vec4 plane_view = transpose(inverse(view_matrix)) * plane_normal;
	gl_ClipDistance[0] = dot(vec4(P.xyz , 1.0f) , plane_view);

    vertexData.texcoord = iv2tex_coord;
	vertexData.normal = iv3normal;

	// for fog effect
	viewSpace_coord = mv_matrix * vec4(iv3vertex, 1.0);
}