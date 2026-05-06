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

layout (std430, binding = 2)buffer histograms{
	uint g_histograms[];
};

const uint MATERIAL_HISTOGRAM_BIN_COUNT = 16u;
const uint MAX_MATERIAL_COUNT = 512u;
shared uint[MAX_MATERIAL_COUNT*MATERIAL_HISTOGRAM_BIN_COUNT] localHistogram;

void main(){
	
	for (uint i = gl_LocalInvocationIndex; i < push.matCount * MATERIAL_HISTOGRAM_BIN_COUNT; i += gl_WorkGroupSize.x * gl_WorkGroupSize.y) {
        localHistogram[i] = 0u;
    }
	barrier();


	uvec2 globalID = gl_GlobalInvocationID.xy;
    uint z_start = uint(gl_WorkGroupID.z);
    uint x = globalID.x;
    uint y = globalID.y;
    uint bin_value = imageLoad(bin,ivec2(x,y)).r;
    if(bin_value<(MAX_MATERIAL_COUNT*(z_start+1)*MATERIAL_HISTOGRAM_BIN_COUNT)){
        if(bin_value>(MAX_MATERIAL_COUNT*z_start*MATERIAL_HISTOGRAM_BIN_COUNT)){
            atomicAdd(localHistogram[bin_value-(MAX_MATERIAL_COUNT*z_start*MATERIAL_HISTOGRAM_BIN_COUNT)],1u);
        }
    }
	barrier();

    if(gl_LocalInvocationIndex == 0){


        int localSize = int(MAX_MATERIAL_COUNT);
        if(push.matCount < (MAX_MATERIAL_COUNT*(z_start+1)))
        localSize = int(push.matCount%MAX_MATERIAL_COUNT);
        for (int i = 0; i < localSize * MATERIAL_HISTOGRAM_BIN_COUNT; i++) {
            if(localHistogram[i] == 0) continue;
            atomicAdd(g_histograms[i + z_start*MAX_MATERIAL_COUNT*MATERIAL_HISTOGRAM_BIN_COUNT], localHistogram[i]);
        }
    }

}
