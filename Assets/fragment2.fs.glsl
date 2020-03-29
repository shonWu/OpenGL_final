#version 410
uniform sampler2D tex;
uniform sampler2D noise_tex;
out vec4 color;

uniform int mode = 0;
uniform float time;
uniform int mouse_x;
uniform int mouse_y;
uniform int bar_x = 300;

uniform int mag_rad = 100;
uniform int mag_center_x = 300;
uniform int mag_center_y = 300;

in VS_OUT
{
	vec2 texcoord;
} fs_in;

vec2 sinewave(vec2 p) {
	float pi = 3.14159;
	float p1 = 0.015;
	float p2 = 20.0 ;

	float x = p.x + p1 * sin( p2 * p.y + time); 
	return vec2(x, p.y);
}

void main()
{
	vec4 texture_color = texture(tex,fs_in.texcoord);
	if(gl_FragCoord.x > bar_x-3 && gl_FragCoord.x < bar_x+3 && mode!=7 && mode != 0){
		color = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else if(gl_FragCoord.x < bar_x-4 && mode!=7){
		if(mode == 0){
			color = texture_color;
			//color = texture(noise_tex,fs_in.texcoord);
		}
		else if(mode == 2){
			// Median filter
			int half_size = 2;
			vec4 color_sum = vec4(0);
			for (int i = -half_size; i <= half_size; i++){
				for (int j = -half_size; j <= half_size; j++){
					ivec2 coord = ivec2(gl_FragCoord.xy) + ivec2(i, j);
					color_sum += texelFetch(tex, coord, 0);
				}
			}
			int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
			vec4 cc = color_sum / sample_count;

			// Quantization
			float nbins = 8.0;
			vec3 tex_color = cc.rgb;
			tex_color = floor(tex_color * nbins) / nbins;
			cc = vec4(tex_color, 1.0);

			// Difference of Gaussian
			float sigma_e = 2.0f;
			float sigma_r = 2.8f;
			float phi = 3.4f;
			float tau = 0.99f;

			float twoSigmaESquared = 2.0 * sigma_e * sigma_e;
			float twoSigmaRSquared = 2.0 * sigma_r * sigma_r;
			int halfWidth = int(ceil(2.0 * sigma_r));
			int img_size = 600;

			vec2 sum = vec2(0.0);
			vec2 norm = vec2(0.0);
			for (int i = -halfWidth; i <= halfWidth; i++){
				for (int j = -halfWidth; j <= halfWidth; j++){
					float d = length(vec2(i,j));
					vec2 kernel = vec2(exp(-d * d / twoSigmaESquared), exp(-d * d / twoSigmaRSquared));
					vec4 c = texture(tex, fs_in.texcoord + vec2(i, j) / img_size);
					vec2 L = vec2(0.299 * c.r + 0.587 * c.g + 0.114 * c.b);
					norm += 2.0 * kernel;
					sum += kernel * L;
				}
			}
			sum /= norm;
			float H = 100.0 * (sum.x - tau * sum.y);
			//perform Hermite interpolation between two values
			float edge = (H > 0.0) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H);
			//linearly interpolate between two values
			//color = mix(vec4(0.0, 0.0, 0.0, 1.0), cc, edge);
			color = cc*edge;
			//color = vec4(edge,edge,edge,1.0);
		}
		else if(mode == 3){

			float waterLevel = 3.0;
			vec4 gray = texture(noise_tex,fs_in.texcoord);
			/*
			vec2 uv = fs_in.texcoord + vec2(gray.x,gray.y)/250;
			vec4 new_color = texture(tex,uv);
			*/

			int half_size = 1;
			vec4 color_sum = vec4(0);
			for (int i = -half_size; i <= half_size; i++){
				for (int j = -half_size; j <= half_size; j++){
					ivec2 coord = ivec2(gl_FragCoord.xy + vec2(gray.x,gray.x) * waterLevel ) + ivec2(i, j) ;
					color_sum += texelFetch(tex, coord , 0);
				}
			}
			int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
			vec4 new_color = color_sum / sample_count;
			
			// Quantization
			float nbins = 16.0;
			vec3 water = new_color.rgb;
			water = floor(water * nbins) / nbins;
			color = vec4(water, 1.0);
		}
		else if(mode == 4){
			int half_size = 5;
			vec4 color_sum = vec4(0);
			for (int i = -half_size; i <= half_size; i++){
				for (int j = -half_size; j <= half_size; j++){
					ivec2 coord = ivec2(gl_FragCoord.xy) + ivec2(i, j);
					color_sum += texelFetch(tex, coord, 0);
				}
			}
			int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
			vec4 blur1 = color_sum / sample_count;

			half_size = 5;
			color_sum = vec4(0);
			for (int i = -half_size; i <= half_size; i++){
				for (int j = -half_size; j <= half_size; j++){
					ivec2 coord = ivec2(gl_FragCoord.xy) + ivec2(i, j);
					color_sum += texelFetch(tex, coord, 0);
				}
			}
			sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
			vec4 blur2 = color_sum / sample_count;
		
			color = texture_color * 0.8 + blur1 * 0.4 + blur2 * 0.2;
		
		}
		else if(mode == 5){
			
			int half_size = 2; //5x5 patch
			vec4 color_sum = vec4(0);
   
			for (int i = -half_size; i <= half_size; i++){
				for (int j = -half_size; j <= half_size; j++){
					ivec2 coord = ivec2(floor(gl_FragCoord.x / (half_size * 2 + 1)) * (half_size * 2 + 1) , floor(gl_FragCoord.y / (half_size * 2 + 1)) * (half_size * 2 + 1)) + ivec2(i, j);
					color_sum += texelFetch(tex, coord, 0);
				}
			}
			int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
			color = color_sum / sample_count;
		}
		else if(mode == 6){
			vec2 uv = sinewave(fs_in.texcoord); 
			vec4 tcolor = texture(tex, uv); 
			color = tcolor; 
			//color = texture(tex,SineWave(fs_in.texcoord));
		}
	}
	else if(mode == 7){
		ivec2 mag_center = ivec2(mag_center_x,mag_center_y);
		if( length(gl_FragCoord.xy - mag_center) < mag_rad+2 && length(gl_FragCoord.xy - mag_center) > mag_rad-2 ){
			color = vec4(0.0, 0.0, 0.0, 1.0);
		}
		else if( length(gl_FragCoord.xy - mag_center) < mag_rad-2){
			ivec2 big = ivec2(gl_FragCoord.xy) - (ivec2(gl_FragCoord.xy) - mag_center) / 2;
			texture_color = texelFetch(tex, big, 0);
			color = texture_color;
		}
		else{
			texture_color = texture(tex,fs_in.texcoord);
			color = texture_color;
		}
	}
	else{
		//color = texture(noise_tex,fs_in.texcoord);
		color = texture_color;
	}
}