#include "st_model.h"

namespace st {

	StModel::StModel(StDevice& device, const StModel::Builder& builder) :stDevice{ device } {
		createVertexBuffers(builder.vertices);

		createIndexBuffers(builder.indices);

	}
	StModel::~StModel() {
		vkDestroyBuffer(stDevice.device(),vertexBuffer,nullptr);
		vkFreeMemory(stDevice.device(),vertexBufferMemory,nullptr);

		if (hasIndexBuffer) {
			vkDestroyBuffer(stDevice.device(),indexBuffer,nullptr);
			vkFreeMemory(stDevice.device(),indexBufferMemory,nullptr);
		}
	}

	void StModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "vertexCount needs to be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		stDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(stDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(stDevice.device(), stagingBufferMemory);

		stDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBuffer,
			vertexBufferMemory);
		stDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		vkDestroyBuffer(stDevice.device(), stagingBuffer,nullptr);
		vkFreeMemory(stDevice.device(), stagingBufferMemory,nullptr);
	}
	void StModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		if(!indexCount)return;
		assert(indexCount>=3 && "IndexCount needs to be at least 3");
		hasIndexBuffer = true;
		VkDeviceSize bufferSize = sizeof(indices[0])*indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		stDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(stDevice.device(),stagingBufferMemory,0,bufferSize,0,&data);
		memcpy(data,indices.data(),bufferSize);
		vkUnmapMemory(stDevice.device(),stagingBufferMemory);

		stDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBuffer,
			indexBufferMemory);
		stDevice.copyBuffer(stagingBuffer,indexBuffer,bufferSize);
		vkDestroyBuffer(stDevice.device(),stagingBuffer,nullptr);
		vkFreeMemory(stDevice.device(),stagingBufferMemory,nullptr);
	}

	void StModel::bind(VkCommandBuffer commandBuffer) {
		VkBuffer buffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer,0,1,buffers,offsets);
		if (hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer,indexBuffer,0,VK_INDEX_TYPE_UINT32);
		}
	}

	void StModel::draw(VkCommandBuffer commandBuffer) {
		if (hasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer,indexCount,1,0,0,0);
		}
		else {
			vkCmdDraw(commandBuffer,vertexCount,1,0,0);
		}
		
	}

	std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}
	std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(6);
		size_t i = 0;
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = (uint32_t)i;
		attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[i].offset = offsetof(Vertex,position);
		i++;
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = (uint32_t)i;
		attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[i].offset = offsetof(Vertex,normal);
		i++;
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = (uint32_t)i;
		attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[i].offset = offsetof(Vertex,color);
		i++;
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = (uint32_t)i;
		attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[i].offset = offsetof(Vertex,textureColor);
		i++;
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = (uint32_t)i;
		attributeDescriptions[i].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[i].offset = offsetof(Vertex,uv);
		i++;
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = (uint32_t)i;
		attributeDescriptions[i].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[i].offset = offsetof(Vertex,textureHash);
		i++;

		return attributeDescriptions;
	}
}