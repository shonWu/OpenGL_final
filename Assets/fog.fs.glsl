#version 410 core

in vec2 vv2texcoord;
in vec4 viewSpace_coord;

uniform sampler2D diffuseTexture;
uniform int fog_type=1;

out vec4 fragColor;

const vec4 fogColor = vec4(0.5, 0.5, 0.5, 1.0);
float fogFactor = 0;
float fogDensity = 0.2;
float fog_start = 1;
float fog_end = 6.0f;

void main()
{
	vec4 color = texture(diffuseTexture, vv2texcoord);
	float dist = length(viewSpace_coord);
	switch (fog_type)
	{
	case 0: //Linear
		fogFactor = (fog_end - dist) / (fog_end - fog_start);
		break;
	case 1: //Exp
		fogFactor = 1.0 / exp(dist * fogDensity);
		break;
	case 2: //Exp sqare
		fogFactor = 1.0 / exp((dist * fogDensity)* (dist * fogDensity));
		break;
	}
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	fragColor = mix(fogColor, color, fogFactor);
}