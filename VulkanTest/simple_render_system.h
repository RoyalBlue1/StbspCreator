#pragma once

#include "st_window.h"
#include "st_pipeline.h"
#include "st_device.h"
#include "st_game_object.h"
#include "st_camera.h"
#include "st_frame_info.h"
#include <memory>


namespace st {
	class SimpleRenderSystem {
	public:


		SimpleRenderSystem(StDevice& device,VkRenderPass renderPass,VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem &operator=(const SimpleRenderSystem &)=delete;

		void renderGameObjects(FrameInfo & frameInfo,std::vector<StGameObject>& gameObjects);
		void computeHistogram(VkCommandBuffer& commandBuffer,uint32_t windowX, uint32_t windowY,VkDescriptorSet* descriptorSet);
	private:

		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass rendrPass);
		



		StDevice& stDevice;

		std::unique_ptr<StPipeline> stPipeline;
		VkPipelineLayout graphicPipelineLayout;

	};
}  