#include "st_renderer.h"



namespace st {




	StRenderer::StRenderer(StWindow& window, StDevice& device) :stWindow{ window }, stDevice{ device }, isFrameStarted{ false }, currentImageIndex{ 0 }, currentFrameIndex{ 0 } {

		recreateSwapChain();
		createCommandBuffers();
	}
	StRenderer::~StRenderer() {
		freeCommandBuffers();
	}

	
	

	void StRenderer::recreateSwapChain() {
		auto extent = stWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = stWindow.getExtent();
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(stDevice.device());
		if (stSwapChain == nullptr) {
			stSwapChain = std::make_unique<StSwapChain>(stDevice, extent);
		}
		else {
			std::shared_ptr<StSwapChain> oldSwapChain = std::move(stSwapChain);
			stSwapChain = std::make_unique<StSwapChain>(stDevice, extent,oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*stSwapChain.get())) {
				throw std::runtime_error("SwapChain format has changed");
			}

		}

	}

	void StRenderer::createCommandBuffers() {
		commandBuffers.resize(StSwapChain::MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = stDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		if(vkAllocateCommandBuffers(stDevice.device(),&allocInfo,commandBuffers.data())!=VK_SUCCESS)
			throw std::runtime_error("failed to alloc command buffers");

	}

	void StRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(stDevice.device(),stDevice.getCommandPool(),static_cast<uint32_t>(commandBuffers.size()),commandBuffers.data());
		commandBuffers.clear();
	}



	VkCommandBuffer StRenderer::beginFrame() {
		assert(!isFrameStarted && "Frame already Started");
		auto res = stSwapChain->acquireNextImage(&currentImageIndex);

		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acqure swap chain image");
		}
		isFrameStarted = true;
		auto buffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if(vkBeginCommandBuffer(buffer,&beginInfo)!=VK_SUCCESS)
			throw std::runtime_error("faild to begin recording command buffer");


		return buffer;
	}

	void StRenderer::endFrame() {
		assert(isFrameStarted && "Cant end frame when no frame rendering");
		auto buffer = getCurrentCommandBuffer();
		
		if(vkEndCommandBuffer(buffer)!=VK_SUCCESS)
			throw std::runtime_error("failed to record command buffer");
		VkResult res = stSwapChain->submitCommandBuffers(&buffer,&currentImageIndex);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || stWindow.wasWindowResized()) {
			stWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to submit command buffer");
		}
		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex +1) % StSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void StRenderer::beginSwapChainRenderpass(VkCommandBuffer commandBuffer){
		assert(isFrameStarted && "Cant start RenderPass when no frame rendering");
		assert(commandBuffer==getCurrentCommandBuffer() && "Cant begin render pass on comamnd buffer from different frame");
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = stSwapChain->getRenderPass();
		renderPassInfo.framebuffer = stSwapChain->getFrameBuffer(currentImageIndex);
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = stSwapChain->getSwapChainExtent();
		std::array<VkClearValue, 3> clearValues{};
		clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
		clearValues[1].color.int32[0] = -1;
		clearValues[1].color.int32[1] = -1;
		clearValues[1].color.int32[2] = -1;
		clearValues[1].color.int32[3] = -1;
		//clearValues[2].color.int32[0] = -1;
		//clearValues[2].color.int32[1] = -1;
		//clearValues[2].color.int32[2] = -1;
		//clearValues[2].color.int32[3] = -1;
		clearValues[2].depthStencil = {1.0f, 0};

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(stSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(stSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{{0, 0}, stSwapChain->getSwapChainExtent()};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void StRenderer::endSwapChainRenderpass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Cant end RenderPass when no frame rendering");
		assert(commandBuffer==getCurrentCommandBuffer() && "Cant end render pass on comamnd buffer from different frame");


		vkCmdEndRenderPass(commandBuffer);
	}
}

