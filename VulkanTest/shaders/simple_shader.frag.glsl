#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 uv;
layout (location = 2) flat in uint texId;

layout (location = 0) out vec4 outColor;
layout (location = 1) out uint outBin;
//layout (location = 2) out uint outTexId;






layout(push_constant) uniform Push {
	mat4 transform; // projection * view * model
	int matCount;
} push;



const float STBSP_NOMINAL_TEX_RES = 4096.0f;
const uint MATERIAL_HISTOGRAM_BIN_COUNT = 16;
void main() {

	vec2 dx = dFdxFine(uv) * STBSP_NOMINAL_TEX_RES;
	vec2 dy = dFdyFine(uv) * STBSP_NOMINAL_TEX_RES;
	float dot = max(dot(dx,dx),dot(dy,dy));

	float mipLevel = floor(clamp( 0.5 * log2(dot), 0.0f, float(MATERIAL_HISTOGRAM_BIN_COUNT - 1) ));

	outColor = vec4(fragColor.x,fragColor.y,mipLevel/15.0f, 1.0);

	

	outBin = MATERIAL_HISTOGRAM_BIN_COUNT * texId + uint(mipLevel);
	
	//outTexId = texId;

	//atomicAdd(g_histograms[texId*MATERIAL_HISTOGRAM_BIN_COUNT+bin],1);
}