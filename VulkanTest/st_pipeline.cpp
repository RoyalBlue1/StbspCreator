#include "st_pipeline.h"

#include "st_model.h"
// std
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace st {
	StPipeline::StPipeline(StDevice& device, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo) :stDevice{ device } {
		createGraphicsPipeline(device, vertFilepath, fragFilepath, configInfo);
	}
	StPipeline::~StPipeline() {
		vkDestroyShaderModule(stDevice.device(), vertShaderModule, nullptr);
		vkDestroyShaderModule(stDevice.device(), fragShaderModule, nullptr);
		vkDestroyPipeline(stDevice.device(), graphicPipeline, nullptr);
	}
	std::vector<char> StPipeline::readFile(const std::string& filepath) {
		std::ifstream file{ filepath, std::ios::ate | std::ios::binary };
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filepath);
		}
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}
	void StPipeline::createGraphicsPipeline(const StDevice& device, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo) {
		
		assert(
			configInfo.pipelineLayout != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline: no pipelineLayout provided in configInfo");
		assert(
			configInfo.renderPass != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline: no renderPass provided in configInfo");

		auto vertCode = readFile(vertFilepath);
		auto fragCode = readFile(fragFilepath);
		std::cout << "Vertex Shader Code Size: " << vertCode.size() << '\n';
		std::cout << "Fragment Shader Code Size: " << fragCode.size() << '\n';

		createShaderModule(vertCode,&vertShaderModule);
		createShaderModule(fragCode,&fragShaderModule);

		VkPipelineShaderStageCreateInfo shaderStages[2];

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;




		auto bindingDescriptions = Vertex::getBindingDescriptions();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();


		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(
			stDevice.device(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&graphicPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline");
		}

	}

	void StPipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* module) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		createInfo.pNext = nullptr;
		
		if(vkCreateShaderModule(stDevice.device(), &createInfo, nullptr, module))
			throw std::runtime_error("failed to create shader module");

	}

	void StPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& info) {

		memset(&info,0,sizeof(info));

		info.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		info.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		info.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
		info.inputAssemblyInfo.flags = 0;
		info.inputAssemblyInfo.pNext = nullptr;

		
		info.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		info.viewportInfo.viewportCount = 1;
		info.viewportInfo.pViewports = nullptr;
		info.viewportInfo.scissorCount = 1;
		info.viewportInfo.pScissors = nullptr;
		info.viewportInfo.pNext = nullptr;


		info.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		info.rasterizationInfo.depthClampEnable = VK_FALSE;
		info.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		info.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		info.rasterizationInfo.lineWidth = 1.0f;
		info.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		info.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		info.rasterizationInfo.depthBiasEnable = VK_FALSE;
		info.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
		info.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
		info.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional
		info.rasterizationInfo.pNext = nullptr;

		info.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		info.multisampleInfo.sampleShadingEnable = VK_FALSE;
		info.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		info.multisampleInfo.minSampleShading = 1.0f;           // Optional
		info.multisampleInfo.pSampleMask = nullptr;             // Optional
		info.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
		info.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional
		info.multisampleInfo.pNext = nullptr;

		info.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		info.colorBlendAttachment.blendEnable = VK_FALSE;
		info.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		info.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		info.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		info.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		info.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		info.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
		

		info.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		info.colorBlendInfo.logicOpEnable = VK_FALSE;
		info.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		info.colorBlendInfo.attachmentCount = 1;
		info.colorBlendInfo.pAttachments = &info.colorBlendAttachment;
		info.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
		info.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
		info.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
		info.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional
		info.colorBlendInfo.pNext = nullptr;

		info.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		info.depthStencilInfo.depthTestEnable = VK_TRUE;
		info.depthStencilInfo.depthWriteEnable = VK_TRUE;
		info.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		info.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		info.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
		info.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		info.depthStencilInfo.stencilTestEnable = VK_FALSE;
		info.depthStencilInfo.front = {};  // Optional
		info.depthStencilInfo.back = {};   // Optional

		info.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		info.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		info.dynamicStateInfo.pDynamicStates = info.dynamicStateEnables.data();
		info.dynamicStateInfo.dynamicStateCount =
			static_cast<uint32_t>(info.dynamicStateEnables.size());
		info.dynamicStateInfo.flags = 0;
		info.dynamicStateInfo.pNext = nullptr;

	}

	void StPipeline::bind(VkCommandBuffer commandBuffer){
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,graphicPipeline);
	}
}