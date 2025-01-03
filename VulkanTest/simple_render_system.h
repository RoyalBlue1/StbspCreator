#pragma once

#include "st_window.h"
#include "st_pipeline.h"
#include "st_device.h"
#include "st_game_object.h"
#include "st_camera.h"
#include <memory>


namespace st {
	class SimpleRenderSystem {
	public:


		SimpleRenderSystem(StDevice& device,VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem &operator=(const SimpleRenderSystem &)=delete;

		void renderGameObjects(VkCommandBuffer commandBuffer,std::vector<StGameObject>& gameObjects,const StCamera& camera);
	private:

		void createPipelineLayout();
		void createPipeline(VkRenderPass rendrPass);
		



		StDevice& stDevice;

		std::unique_ptr<StPipeline> stPipeline;
		VkPipelineLayout pipelineLayout;

	};
}  