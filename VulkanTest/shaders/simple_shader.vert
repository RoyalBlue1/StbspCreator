#version 450


layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec3 texColor;
layout(location = 4) in vec2 uv;
layout(location = 5) in uint textureHash;


layout(location = 0) out vec3 fragColor;


layout(push_constant) uniform Push {
	mat4 transform; // projection * view * model
	mat4 normalMatrix;
} push;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0,-3.0,-1.0));
void main(){
	gl_Position = push.transform * vec4(position,1.0);

	vec3 normalWorldSpace = normalize((push.normalMatrix * vec4(normal, 0.0)).xyz);
	float lightIntensity = max(dot(normalWorldSpace,DIRECTION_TO_LIGHT),0.0)* 0.7 + 0.3;
	//fragColor = (normal+1)/2;
	//fragColor = color*lightIntensity;
	fragColor = texColor*lightIntensity;
}