#include "simple_render_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace st {

	struct SimplePushConstantData {
		glm::mat4 transform{ 1.0f };
		glm::mat4 modelMatrix{ 1.0f };
	};


	SimpleRenderSystem::SimpleRenderSystem(StDevice& device, VkRenderPass renderPass) :stDevice{ device } {

		createPipelineLayout();
		createPipeline(renderPass);
	}
	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(stDevice.device(),pipelineLayout,nullptr);
	}

	
	void SimpleRenderSystem::createPipelineLayout() {

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(stDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("couldn't create pipeline layout");
		}
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
		PipelineConfigInfo pipelineConfig;
		StPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		stPipeline = std::make_unique<StPipeline>(stDevice, "simple_shader.vert.spv","simple_shader.frag.spv",pipelineConfig);
	}





	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer,std::vector<StGameObject>& gameObjects,const StCamera& camera) {
		stPipeline->bind(commandBuffer);
		auto projectionView = camera.getProjection() *camera.getView();
		for (auto& obj : gameObjects) {

			SimplePushConstantData push{};		
			push.transform = projectionView * obj.transform3d.mat4();
			push.modelMatrix = obj.transform3d.normalMatrix();
			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}


}