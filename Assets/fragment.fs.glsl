#version 410 core

uniform sampler2D tex;
uniform sampler2DShadow shadow_tex;
uniform int useFog = 0;

layout(location = 0) out vec4 fragColor;

const vec4 fogColor = vec4(0.7, 0.7, 0.7, 0.1);
float fogFactor = 0;
float fogDensity = 0.0125;
float fog_start = 1;
float fog_end = 6.0f;


uniform int normal_flag;
in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
	vec3 V;
    vec3 H; // eye space halfway vector

    vec2 texcoord;

	vec3 normal;
} vertexData;

in vec4 shadow_coord;
in vec4 viewSpace_coord;

// Material properties
uniform vec3 diffuse_albedo = vec3(0.5, 0.2, 0.7);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 100.0;

void main()
{
	vec3 texColor = texture(tex,vertexData.texcoord).rgb;
	
	// Normalize the incoming N, L and V vectors
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);
	vec3 H = normalize(L + V);

	//diffuse
	vec3 diffuse = max(dot(N, L), 0.0) * texColor;

	//specular
	vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

	float shadow_factor = textureProj(shadow_tex, shadow_coord);
	if(shadow_factor < 0.5) shadow_factor = 0.5;

	//fragColor = vec4(texColor, 1.0);
	if(normal_flag == 1){

		//hw2ªºnormal color ¨S¥Î
		fragColor = vec4(vertexData.normal, 1.0);
	}
	else{
		fragColor = shadow_factor * vec4(diffuse * 1.8 + specular, 1.0);
		//fragColor = vec4(texColor, 1.0);
	}
	// for fog effect
	if (useFog == 1) {
		vec4 color = texture(tex, vertexData.texcoord);
		float dist = length(viewSpace_coord);
		fogFactor = 1.0 / exp(dist * fogDensity);
		fogFactor = clamp(fogFactor, 0.0, 1.0);
		fragColor = mix(fogColor, fragColor, fogFactor);
	}
}