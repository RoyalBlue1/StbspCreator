#include "st_bsp_loader.h"
#include "st_utils.h"
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
	template <>
	struct hash<st::Vertex> {
		size_t operator()(st::Vertex const &vertex) const {
			size_t seed = 0;
			st::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv,vertex.textureHash);
			return seed;
		}
	};
} 

namespace st {




	template<typename T> std::vector<T> BspLoader::loadLump(int index) {
		std::vector<T> vec;
		file.seekg(header.lumps[index].offset,std::ios::beg);
		for (int i = 0; i < (header.lumps[index].length / sizeof(T));i++) {
			T ele;
			file.read((char*)&ele,sizeof(T));
			vec.push_back(ele);
		}
		return vec;
	}

	void BspLoader::loadFileMultipleMeshes(const char* fileName) {


		file.open(fileName,std::ios::binary);
		file.seekg(0,std::ios::beg);
		file.read((char*) & header, sizeof(header));
		if(header.magic!=MAGIC_rBSP)return;
		
		std::vector<glm::vec3> vertices = loadLump<glm::vec3>(0x03);
		std::vector<glm::vec3> normals = loadLump<glm::vec3>(0x1E);


		std::vector<VertexUnlit> vertex_unlit = loadLump<VertexUnlit>(0x47);
		std::vector<VertexLitFlat> vertex_lit_flat = loadLump<VertexLitFlat>(0x48);
		std::vector<VertexLitBump> vertex_lit_bump = loadLump<VertexLitBump>(0x49);
		std::vector<VertexUnlitTS> vertex_unlit_ts = loadLump<VertexUnlitTS>(0x4A);

		std::vector<uint16_t> indi = loadLump<uint16_t>(0x4F);
		std::vector<BspMesh> bspMeshes = loadLump<BspMesh>(0x50);
		std::vector<MaterialSort> materialSorts = loadLump<MaterialSort>(0x52);
		
		std::vector<uint32_t> textureStringTable = loadLump<uint32_t>(0x2C);
		std::vector<char> textureStringData = loadLump<char>(0x2B);
		std::vector<TextureData> textureData = loadLump<TextureData>(0x2);
		
		for (auto& bspMesh : bspMeshes) {

			if(bspMesh.meshFlags&0x20000)continue;
			std::unordered_map<Vertex,size_t> vertBuildList;
			Mesh loadedMesh;
			uint32_t vertexOffset = materialSorts[bspMesh.material_sort].vertexOffset;
			uint32_t vertexOffset2 = bspMesh.first_vertex;
			uint32_t materialId = materialSorts[bspMesh.material_sort].textureData;
			auto& texture = textureData[materialId];
			loadedMesh.material = std::string(&textureStringData[textureStringTable[texture.nameStringId]]);
			for (int j = 0; j < bspMesh.num_triangles * 3; j++) {



				uint32_t vertexIndex = indi[(size_t)j+bspMesh.first_mesh_index]+vertexOffset;
				Vertex v;
				switch ((bspMesh.meshFlags >> 9) & 3) {
				case 0:
				{
					auto& vert = vertex_lit_flat[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
					break;
				case 1:
				{
					auto& vert = vertex_lit_bump[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
					break;
				case 2:
				{
					auto& vert = vertex_unlit[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
					break;
				case 3:
				{
					auto& vert = vertex_unlit_ts[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
					break;
				}
				size_t index;
				if (vertBuildList.contains(v)) {
					index = vertBuildList[v];
				}
				else {
					index = vertBuildList.size();
					vertBuildList.emplace(v,index);
				}
				loadedMesh.indices.push_back((uint32_t)index);//vertBuildList.size());

				
			}
			//printf("mesh type %d mesh vert count %d vert offset %d\n",(mesh.meshFlags >> 9) & 3,mesh.num_vertices,vertexOffset);
			//printf("mesh offset %d first vertex %d\n",vertexOffset,vertexOffset2);
			
			//uint32_t off = materialSorts[mesh.material_sort].vertexOffset+offsets[(mesh.meshFlags>>9)&3];
			loadedMesh.verts.resize(vertBuildList.size());
			for (auto& p : vertBuildList) {
				loadedMesh.verts[p.second] = p.first;
			}
			meshes.push_back(loadedMesh);
		}
		

		
	}
	void BspLoader::loadFileSingleMesh(const char* fileName) {


		file.open(fileName,std::ios::binary);
		file.seekg(0,std::ios::beg);
		file.read((char*) & header, sizeof(header));
		if(header.magic!=MAGIC_rBSP)return;

		std::vector<glm::vec3> vertices = loadLump<glm::vec3>(0x03);
		std::vector<glm::vec3> normals = loadLump<glm::vec3>(0x1E);


		std::vector<VertexUnlit> vertex_unlit = loadLump<VertexUnlit>(0x47);
		std::vector<VertexLitFlat> vertex_lit_flat = loadLump<VertexLitFlat>(0x48);
		std::vector<VertexLitBump> vertex_lit_bump = loadLump<VertexLitBump>(0x49);
		std::vector<VertexUnlitTS> vertex_unlit_ts = loadLump<VertexUnlitTS>(0x4A);

		std::vector<uint16_t> indi = loadLump<uint16_t>(0x4F);
		std::vector<BspMesh> bspMeshes = loadLump<BspMesh>(0x50);
		std::vector<MaterialSort> materialSorts = loadLump<MaterialSort>(0x52);

		std::vector<uint32_t> textureStringTable = loadLump<uint32_t>(0x2C);
		std::vector<char> textureStringData = loadLump<char>(0x2B);
		std::vector<TextureData> textureData = loadLump<TextureData>(0x2);
		
		std::unordered_map<Vertex,size_t> vertBuildList;
		Mesh loadedMesh;
		for (auto& bspMesh : bspMeshes) {

			if(bspMesh.meshFlags&0x20000)continue;
			
			
			uint32_t vertexOffset = materialSorts[bspMesh.material_sort].vertexOffset;
			uint32_t vertexOffset2 = bspMesh.first_vertex;
			uint32_t materialId = materialSorts[bspMesh.material_sort].textureData;
			auto& texture = textureData[materialId];
			loadedMesh.material = "";
			std::string material = std::string(&textureStringData[textureStringTable[texture.nameStringId]]);
			for (int j = 0; j < bspMesh.num_triangles * 3; j++) {



				uint32_t vertexIndex = indi[(size_t)j+bspMesh.first_mesh_index]+vertexOffset;
				Vertex v;
				switch ((bspMesh.meshFlags >> 9) & 3) {
				case 0:
				{
					auto& vert = vertex_lit_flat[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case 1:
				{
					auto& vert = vertex_lit_bump[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case 2:
				{
					auto& vert = vertex_unlit[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case 3:
				{
					auto& vert = vertex_unlit_ts[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					v.normal = normals[vert.normalIndex];
					v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				}
				std::hash<std::string> strHasher;
				v.textureHash = strHasher(material);
				v.textureColor = {((v.textureHash) & 0xFF) / 255.0,((v.textureHash >> 8) & 0xFF) / 255.0,((v.textureHash >> 16) & 0xFF) / 255.0 }; 
				size_t index;
				if (vertBuildList.contains(v)) {
					index = vertBuildList[v];
				}
				else {
					index = vertBuildList.size();
					vertBuildList.emplace(v,index);
				}
				loadedMesh.indices.push_back((uint32_t)index);


			}
			//printf("mesh type %d mesh vert count %d vert offset %d\n",(mesh.meshFlags >> 9) & 3,mesh.num_vertices,vertexOffset);
			//printf("mesh offset %d first vertex %d\n",vertexOffset,vertexOffset2);

			//uint32_t off = materialSorts[mesh.material_sort].vertexOffset+offsets[(mesh.meshFlags>>9)&3];
			

		}
		loadedMesh.verts.resize(vertBuildList.size());
		for (auto& p : vertBuildList) {
			loadedMesh.verts[p.second] = p.first;
		}
		meshes.push_back(loadedMesh);


	}

};