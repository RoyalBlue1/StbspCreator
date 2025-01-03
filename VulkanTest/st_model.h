#pragma once

#include "st_device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>




namespace st {

	struct Vertex {
		glm::vec3 position{};
		glm::vec3 normal{};
		glm::vec3 color{};
		glm::vec3 textureColor{};
		glm::vec2 uv{};
		uint32_t textureHash{};

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		bool operator==(const Vertex& other) const {
			return position==other.position && color == other.color && normal == other.normal && uv == other.uv;
		}

	};

	class StModel {
	public:



		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
		};

		StModel(StDevice& device,const StModel::Builder& builder);
		~StModel();

		StModel(const StModel&)=delete;
		StModel& operator=(const StModel&)=delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:

		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffers(const std::vector<uint32_t> &indices);
		StDevice& stDevice;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;
	};

}