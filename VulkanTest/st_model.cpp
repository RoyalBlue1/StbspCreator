#include "st_model.h"

namespace st {

	StModel::StModel(StDevice& device, const Mesh& builder) :stDevice{ device } {
		createVertexBuffers(builder.verts);

		createIndexBuffers(builder.indices);

	}
	StModel::~StModel() {	}

	void StModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "vertexCount needs to be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		uint32_t vertexSize = sizeof(vertices[0]);
	

		StBuffer stagingBuffer{
			stDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)vertices.data());

		vertexBuffer = std::make_unique<StBuffer>(
			stDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		stDevice.copyBuffer(stagingBuffer.getBuffer(),vertexBuffer->getBuffer(),bufferSize);
	}
	void StModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		if(!indexCount)return;
		assert(indexCount>=3 && "IndexCount needs to be at least 3");
		hasIndexBuffer = true;
		VkDeviceSize bufferSize = sizeof(indices[0])*indices.size();

		uint32_t indexSize = sizeof(indices[0]);

		StBuffer stagingBuffer{
			stDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)indices.data());

		indexBuffer = std::make_unique<StBuffer>(
			stDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		stDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
	}

	void StModel::bind(VkCommandBuffer commandBuffer) {
		VkBuffer buffers[] = {vertexBuffer->getBuffer()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer,0,1,buffers,offsets);
		if (hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer,indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		size_t i = 0;
		VkVertexInputAttributeDescription desc;
		desc.binding = 0;
		desc.location = (uint32_t)attributeDescriptions.size();
		desc.format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.offset = offsetof(Vertex,position);
		attributeDescriptions.push_back(desc);
		//desc.binding = 0;
		//desc.location = (uint32_t)attributeDescriptions.size();
		//desc.format = VK_FORMAT_R32G32B32_SFLOAT;
		//desc.offset = offsetof(Vertex,normal);
		//attributeDescriptions.push_back(desc);
		//desc.binding = 0;
		//desc.location = (uint32_t)attributeDescriptions.size();
		//desc.format = VK_FORMAT_R32G32B32_SFLOAT;
		//desc.offset = offsetof(Vertex,color);
		//attributeDescriptions.push_back(desc);
		desc.binding = 0;
		desc.location = (uint32_t)attributeDescriptions.size();
		desc.format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.offset = offsetof(Vertex,textureColor);
		attributeDescriptions.push_back(desc);
		desc.binding = 0;
		desc.location = (uint32_t)attributeDescriptions.size();
		desc.format = VK_FORMAT_R32G32_SFLOAT;
		desc.offset = offsetof(Vertex,uv);
		attributeDescriptions.push_back(desc);
		desc.binding = 0;
		desc.location = (uint32_t)attributeDescriptions.size();
		desc.format = VK_FORMAT_R32_UINT;
		desc.offset = offsetof(Vertex,materialId);
		attributeDescriptions.push_back(desc);

		return attributeDescriptions;
	}
}

