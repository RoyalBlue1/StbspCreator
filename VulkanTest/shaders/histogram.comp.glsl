#version 460
#pragma shader_stage(compute)

#extension GL_GOOGLE_include_directive: enable
#extension GL_KHR_shader_subgroup_basic: enable
#extension GL_KHR_shader_subgroup_arithmetic: enable
#extension GL_KHR_shader_subgroup_ballot: enable

#define WORKGROUP_SIZE 256

#define SUBGROUP_SIZE 32

#define BIN_SIZE 16

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(push_constant) uniform Push {
	mat4 transform; // projection * view * model
	uint matCount;
} push;

layout(binding = 1,r32ui)uniform readonly uimage2D bin;
//layout(binding = 2,r32ui)uniform readonly uimage2D texId;
layout (std430, binding = 2)buffer histograms{
	uint g_histograms[];
};

const uint MATERIAL_HISTOGRAM_BIN_COUNT = 16u;
const uint MAX_MATERIAL_COUNT = 512u;
shared uint[MAX_MATERIAL_COUNT*MATERIAL_HISTOGRAM_BIN_COUNT] localHistogram;

void main(){
	
	for (int i = 0; i < push.matCount * MATERIAL_HISTOGRAM_BIN_COUNT; i++) {
		localHistogram[i] = 0u;
	}
	barrier();
	uint x_start = uint(gl_WorkGroupID.x) * 16u;
	uint y_start = uint(gl_WorkGroupID.y) * 16u;

	for (uint x = x_start; x < x_start + 16u; x++) {
		for (uint y = y_start; y < y_start + 16u; y++) {
			uint bin_value = imageLoad(bin,ivec2(x,y)).r;
			if(bin_value>(MAX_MATERIAL_COUNT*16))continue;
			atomicAdd(localHistogram[bin_value],1u);
		}
	}
	barrier();
	for (int i = 0; i < push.matCount * MATERIAL_HISTOGRAM_BIN_COUNT; i++) {
		atomicAdd(g_histograms[i],localHistogram[i]);
	}
}
