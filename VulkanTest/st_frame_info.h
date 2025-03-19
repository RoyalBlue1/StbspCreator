#pragma once
#include "st_camera.h"
// lib
#include <vulkan/vulkan.h>
namespace st {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		StCamera &camera;
		VkDescriptorSet globalDescriptorSet;
	};
} 