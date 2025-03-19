#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 texColor;
layout(location = 2) in vec2 uv;
layout(location = 3) in uint texId;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 uvOut;
layout(location = 2) out uint texIdOut;



layout(push_constant) uniform Push {
	mat4 transform; // projection * view * model
	int matCount;
} push;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0,-3.0,-1.0));
void main(){
	gl_Position = push.transform * vec4(position,1.0);

	
	fragColor = texColor;//debug only
	uvOut = uv;
	texIdOut = texId;
	
}