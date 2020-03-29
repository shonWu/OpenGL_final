#version 410 core                                                                     
	                                                                                      
layout(location = 0) in vec3 iposition;	      
uniform mat4 mvp;
uniform vec3 plane_normal;
out vec4 clipSpace;
out vec3 position;
out vec3 normal;

void main(void)                                                                       
{                  
	normal = plane_normal;
	position = iposition;
	clipSpace = mvp * vec4(iposition , 1.0f);
	gl_Position = clipSpace;
}                         