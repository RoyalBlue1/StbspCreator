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