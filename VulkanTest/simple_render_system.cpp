#include "simple_render_system.h"
#include "st_material_management.h"

#define GLM_FORCE_SSE2
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace st {

	struct SimplePushConstantData {
		glm::mat4 transform{ 1.0f };
		uint32_t materialCount;
	};


	SimpleRenderSystem::SimpleRenderSystem(StDevice& device, VkRenderPass renderPass,VkDescriptorSetLayout globalSetLayout) :stDevice{ device } {

		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}
	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(stDevice.device(),graphicPipelineLayout,nullptr);
	}

	
	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};


		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(stDevice.device(), &pipelineLayoutInfo, nullptr, &graphicPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("couldn't create pipeline layout");
		}
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
		PipelineConfigInfo pipelineConfig;
		StPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = graphicPipelineLayout;
		stPipeline = std::make_unique<StPipeline>(stDevice, "simple_shader.vert.spv","simple_shader.frag.spv","histogram.comp.spv", pipelineConfig);
	}





	void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo,std::vector<StGameObject>& gameObjects) {
		stPipeline->bindGraphics(frameInfo.commandBuffer);
		auto projectionView =frameInfo.camera.getProjection() * frameInfo.camera.getView();
		
		//vkCmdBindDescriptorSets(
		//	frameInfo.commandBuffer,
		//	VK_PIPELINE_BIND_POINT_GRAPHICS,
		//	graphicPipelineLayout,
		//	0,
		//	1,
		//	&frameInfo.globalDescriptorSet,
		//	0,
		//	nullptr
		//);
		
		
		
		for (auto& obj : gameObjects) {

			SimplePushConstantData push{};		
			push.transform = projectionView * obj.transform3d.mat4();
			push.materialCount = StMaterialManager::getManager().getMaterialCount();
			
			vkCmdPushConstants(
				frameInfo.commandBuffer,
				graphicPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);
		}

		
	}
	void SimpleRenderSystem::computeHistogram(VkCommandBuffer& commandBuffer,uint32_t windowX, uint32_t windowY,VkDescriptorSet* descriptorSet) {
		stPipeline->bindCompute(commandBuffer);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			graphicPipelineLayout,
			0,
			1,
			descriptorSet,
			0,
			nullptr
		);
		vkCmdDispatch(commandBuffer,windowX/16,windowY/16,1);
	}
	
}