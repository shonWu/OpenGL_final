#version 410 core                                                                     
in vec4 clipSpace;
in vec3 position;
in vec3 normal;
out vec4 fragColor;      
uniform samplerCube tex_envmap;
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D dudvTex;
uniform int useFog = 0;
float fogFactor = 0;
float fogDensity = 0.0125;
uniform vec3 light_pos_w = vec3(-3.0, 200.0, -9.0);
uniform float move_factor;
float wavestrength = 0.02f;
void main(void)                                                                       
{
	//Environment mapping
		vec3 position_n = normalize(position);
		vec3 normal_n = normalize(normal);
		vec3 r = normalize(reflect(position , normal));
		vec3 env_color = texture(tex_envmap , r).xyz;
	//Environment mapping end

	//Calculating water's screen coordinate
		vec2 texCoord = vec2(position.x / 2000.0f , position.z / 2000.0f) + 0.5f;
		vec2 ndc = (clipSpace.xy / clipSpace.w) * 0.5 + 0.5;
	//Calculating water's screen coordinate end

	//Calculating distortion
		vec2 distortion = (texture(dudvTex , vec2(mod(texCoord.x + move_factor, 1.0f) , texCoord.y)).rg * 2.0f - 1.0f) * wavestrength;
	//Calculating distortion end

	//Caluculating reflextion texture coordinates
		vec2 reflectTexCoords = vec2(ndc.x , 1-ndc.y);
		vec2 refractTexCoords = vec2(ndc.x , ndc.y);
		reflectTexCoords += distortion;
		reflectTexCoords = clamp(reflectTexCoords , 0.001 , 0.999);
	//Caluculating reflextion texture coordinates end

	//Sample texture
		vec4 reflectColor = texture(reflectionTexture , reflectTexCoords);
		vec4 refractColor = texture(refractionTexture , refractTexCoords);
	//Sample texture end

	vec4 color = vec4(92.0f / 255.0f  , 102.0f / 255.0f, 231.0f / 255.0f , 1.0f) * 0.5 + reflectColor * 0.5;
	fragColor = color; 
	
}                         