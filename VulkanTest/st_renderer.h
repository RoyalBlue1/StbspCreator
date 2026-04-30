#pragma once

#include "st_device.h"
#include "st_swap_chain.h"
#include "st_window.h"



namespace st {
	class StRenderer {
	public:


		StRenderer(StWindow& window,StDevice& device);
		~StRenderer();

		StRenderer(const StRenderer&) = delete;
		StRenderer &operator=(const StRenderer &)=delete;


		VkRenderPass getSwapChainRenderPass() const{return stSwapChain->getRenderPass(); };

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Can not get comamnd buffer when no frame in progress");
			return commandBuffers[currentFrameIndex];
		}

		bool isFrameInProgress() const {
			return isFrameStarted;
		}
		
		uint32_t getFrameIndex() const {
			assert(isFrameStarted && "Can not get frame index when no frame in progress");
			return currentFrameIndex;
		}
		float getAspectRatio()const {
			return stSwapChain->extentAspectRatio();
		}
		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderpass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderpass(VkCommandBuffer commandBuffer);
		VkDescriptorImageInfo* binDescriptorInfo(int index) {
			return stSwapChain->binDescriptorInfo(index);
		}

		void imageRenderBarrier(VkCommandBuffer commandBuffer) {

			VkImageSubresourceRange range = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			};

			VkImageMemoryBarrier barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL, // or SHADER_READ_ONLY_OPTIMAL
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = stSwapChain->getBinImage(currentFrameIndex),
				.subresourceRange = range

			};
			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0,
				NULL,
				0,
				NULL,
				1,
				&barrier
			);
		}
	private:


		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		StWindow& stWindow;
		StDevice& stDevice;
		std::unique_ptr<StSwapChain> stSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		uint32_t currentFrameIndex;
		bool isFrameStarted;
	};
}