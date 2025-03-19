#pragma once

#include "st_device.h"
#include "st_buffer.h"
#include "st_utils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>
#include <memory>

namespace st {
	struct Vertex {
		glm::vec3 position{};
		//glm::vec3 normal{};
		//glm::vec3 color{};
		glm::vec3 textureColor{};
		glm::vec2 uv{};
		uint32_t materialId{};

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		bool operator==(const Vertex& other) const {
			return position==other.position && textureColor == other.textureColor && materialId == other.materialId && uv == other.uv;//&& color == other.color && normal == other.normal ;
		}

	};
}

namespace std {
	template <>
	struct hash<st::Vertex> {
		size_t operator()(st::Vertex const &vertex) const {
			size_t seed = 0;
			st::hashCombine(seed, vertex.position, vertex.textureColor,vertex.materialId,vertex.uv);//vertex.color, vertex.normal, vertex.uv,vertex.materialId);
			return seed;
		}
	};

} 

namespace st {



	struct Mesh {

		std::vector<Vertex> verts;
		std::vector<uint32_t> indices;
		std::unordered_map<Vertex,size_t> vertBuildList;
		void addVert(Vertex&v){
			size_t index;
			if (vertBuildList.contains(v)) {
				index = vertBuildList[v];
			}
			else {
				index = vertBuildList.size();
				vertBuildList.emplace(v,index);
			}
			indices.push_back((uint32_t)index);
		}
		void finishMesh() {
			verts.resize(vertBuildList.size());
			for (auto& p : vertBuildList) {
				verts[p.second] = p.first;
			}
			vertBuildList.clear();
		}
	};

	class StModel {
	public:



		StModel(StDevice& device,const Mesh& builder);
		~StModel();

		StModel(const StModel&)=delete;
		StModel& operator=(const StModel&)=delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:

		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffers(const std::vector<uint32_t> &indices);
		StDevice& stDevice;
		std::unique_ptr<StBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<StBuffer> indexBuffer;
		uint32_t indexCount;
	};

}

