#pragma once

#include "st_device.h"

#include <string>
#include <vector>


namespace st {

	struct PipelineConfigInfo {
		VkPipelineViewportStateCreateInfo viewportInfo{};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		VkPipelineColorBlendAttachmentState colorBlendAttachment[3];
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		std::vector<VkDynamicState> dynamicStateEnables{};
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class StPipeline {
	public:
		StPipeline(StDevice& device, const std::string& vertFilepath, const std::string& fragFilepath,const std::string& compFilepath, const PipelineConfigInfo& configInfo);
		~StPipeline();

		StPipeline(const StPipeline&)=delete;
		StPipeline& operator=(const StPipeline&)=delete;

		void bindGraphics(VkCommandBuffer commandBuffer);
		void bindCompute(VkCommandBuffer commandBuffer);
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

	private:
		static std::vector<char> readFile(const std::string& filepath);
		void createGraphicsPipeline(const StDevice& device, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo);
		void createComputePipeline(StDevice& device, const std::string& computeFilepath,VkPipelineLayout pipelineLayout);
		void createShaderModule(const std::vector<char>& code,VkShaderModule* shaderModule);
		
		StDevice& stDevice;
		VkQueue computeQueue;
		VkPipeline graphicPipeline;
		VkPipeline computePipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
		VkShaderModule computeShaderModule;
		//VkDescriptorSetLayout computeDescriptorSetLayout;
		//VkPipelineLayout computePipelineLayout;
	};
}