#include "st_bsp_loader.h"
#include "st_mdl_loader.h"
#include "st_utils.h"
#include <unordered_map>
#include "st_material_management.h"
#include "st_game_object.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


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

			if(bspMesh.meshFlags&0x60000)continue;
			std::unordered_map<Vertex,size_t> vertBuildList;
			Mesh loadedMesh{};
			uint32_t vertexOffset = materialSorts[bspMesh.material_sort].vertexOffset;
			uint32_t vertexOffset2 = bspMesh.first_vertex;
			uint32_t materialBspId = materialSorts[bspMesh.material_sort].textureData;
			auto& texture = textureData[materialBspId];
			std::string materialName = std::string(&textureStringData[textureStringTable[texture.nameStringId]]);
			uint32_t material = StMaterialManager::getManager().addMaterial(materialName);
			for (int j = 0; j < bspMesh.num_triangles * 3; j++) {



				uint32_t vertexIndex = indi[(size_t)j+bspMesh.first_mesh_index]+vertexOffset;
				Vertex v;
				switch ((bspMesh.meshFlags >> 9) & 3) {
				case 0:
				{
					auto& vert = vertex_lit_flat[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					//v.uv = vert.albedoUv;
				}
					break;
				case 1:
				{
					auto& vert = vertex_lit_bump[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					//v.uv = vert.albedoUv;
				}
					break;
				case 2:
				{
					auto& vert = vertex_unlit[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					//v.uv = vert.albedoUv;
				}
					break;
				case 3:
				{
					auto& vert = vertex_unlit_ts[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					//v.uv = vert.albedoUv;
				}
					break;
				}
				std::hash<std::string> strHasher;
				uint32_t materialHash = strHasher(materialName);
				v.textureColor = {((materialHash) & 0xFF) / 255.0,((materialHash >> 8) & 0xFF) / 255.0,((materialHash >> 16) & 0xFF) / 255.0 }; 
				v.materialId = material;
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

	void AngleMatrix( const Vector3 &angles,const Vector3& position,float scale)
	{

		const static __m128 conversionFactor = _mm_set1_ps(180.f/glm::pi<float>());
		float sr, sp, sy, cr, cp, cy;
		__m128 ang = _mm_mul_ps(_mm_set_ps(0.f,angles.z,angles.y,angles.x),conversionFactor);
		__m128 sinm = _mm_sin_ps(ang);
		__m128 cosm = _mm_cos_ps(ang);

		sy = sin(angles.y*(180.f/glm::pi<float>()));
		sp = sin(angles.x*(180.f/glm::pi<float>()));
		sr = sin(angles.z*(180.f/glm::pi<float>()));
		cy = cos(angles.y*(180.f/glm::pi<float>()));
		cp = cos(angles.x*(180.f/glm::pi<float>()));
		cr = cos(angles.z*(180.f/glm::pi<float>()));


		// matrix = (YAW * PITCH) * ROLL
		//matrix.m[0][0] = cp*cy*scale;
		//matrix.m[1][0] = cp*sy*scale;
		//matrix.m[2][0] = -sp*scale;

		float crcy = cr*cy;
		float srcy = sr*cy;
		float crsy = cr*sy;
		float srsy = sr*sy;

		__m128 shuffle_0 = _mm_shuffle_ps(sinm,cosm,_MM_SHUFFLE(1,1,1,1));
		__m128 shuffle_1 = _mm_shuffle_ps(sinm,cosm,_MM_SHUFFLE(2,2,2,2));
		__m128 shuffle_2 = _mm_shuffle_ps(shuffle_1,shuffle_1,_MM_SHUFFLE(3,1,3,1));
		__m128 mul = _mm_mul_ps(shuffle_0,shuffle_2);
		//matrix.m[0][1] = (sp*srcy-crsy)*scale;
		//matrix.m[1][1] = (sp*srsy+crcy)*scale;
		//matrix.m[2][1] = (sr*cp)*scale;

		//matrix.m[0][2] = (sp*crcy+srsy)*scale;
		//matrix.m[1][2] = (sp*crsy-srcy)*scale;
		//matrix.m[2][2] = (cr*cp)*scale;

		//matrix.m[0][3] = position.x;
		//matrix.m[1][3] = position.y;
		//matrix.m[2][3] = position.z;
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
		
		std::vector<char> gameLump = loadLump<char>(35);

		std::unordered_map<Vertex,size_t> vertBuildList;
		Mesh loadedMesh{};
		for (auto& bspMesh : bspMeshes) {

			if(bspMesh.meshFlags&0x60000)continue;

			
			
			uint32_t vertexOffset = materialSorts[bspMesh.material_sort].vertexOffset;
			uint32_t vertexOffset2 = bspMesh.first_vertex;
			uint32_t materialBspId = materialSorts[bspMesh.material_sort].textureData;
			auto& texture = textureData[materialBspId];
			std::string materialName = std::string(&textureStringData[textureStringTable[texture.nameStringId]]);
			uint32_t materialId = StMaterialManager::getManager().addMaterial(materialName);
			for (int j = 0; j < bspMesh.num_triangles * 3; j++) {



				uint32_t vertexIndex = indi[(size_t)j+bspMesh.first_mesh_index]+vertexOffset;
				Vertex v;
				switch ((bspMesh.meshFlags >> 9) & 3) {
				case 0:
				{
					auto& vert = vertex_lit_flat[vertexIndex];
					v.position = vertices[vert.vertexIndex&0x7FFFFFFF];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case 1:
				{
					auto& vert = vertex_lit_bump[vertexIndex];
					v.position = vertices[vert.vertexIndex&0x7FFFFFFF];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case 2:
				{
					auto& vert = vertex_unlit[vertexIndex];
					v.position = vertices[vert.vertexIndex&0x7FFFFFFF];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case 3:
				{
					auto& vert = vertex_unlit_ts[vertexIndex];
					v.position = vertices[vert.vertexIndex];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				}
				std::hash<std::string> strHasher;
				uint32_t materialHash = strHasher(materialName);
				v.textureColor = {((materialHash) & 0xFF) / 255.0,((materialHash >> 8) & 0xFF) / 255.0,((materialHash >> 16) & 0xFF) / 255.0 }; 
				v.materialId = materialId;
				size_t index;
				if (vertBuildList.contains(v)) {
					index = vertBuildList[v];
				}
				else {
					index = vertBuildList.size();
					vertBuildList.emplace(v,index);
				}
				loadedMesh.addVert(v);


			}
			//printf("mesh type %d mesh vert count %d vert offset %d\n",(mesh.meshFlags >> 9) & 3,mesh.num_vertices,vertexOffset);
			//printf("mesh offset %d first vertex %d\n",vertexOffset,vertexOffset2);

			//uint32_t off = materialSorts[mesh.material_sort].vertexOffset+offsets[(mesh.meshFlags>>9)&3];
			

		}
		
		size_t readPtr = 20;
		int modelNameCount,leafCount,propCount;
		modelNameCount = *(int*)&gameLump[readPtr];
		readPtr+=4;
		std::vector<MdlLoader> mdls;
		for (int i = 0; i < modelNameCount; i++) {
			char mdlName[129];
			strncpy(mdlName,&gameLump[readPtr],128);
			mdlName[128] = 0;
			readPtr+=128;
			mdls.emplace_back((std::string("H:\\r2\\r2_vpk\\")+mdlName).c_str());
		}
		//skip leafData
		//leafCount = *(int*)&gameLump[readPtr];
		readPtr += 8;
		propCount = *(int*)&gameLump[readPtr];
		readPtr+=4;
		for (int i = 0; i < propCount; i++) {

			if((i%10)==0)printf("%d/%d props\n",i,propCount);
			StaticProp prop;
			memcpy(&prop, &gameLump[readPtr], sizeof(prop));
			readPtr += sizeof(prop);
			//Transform3dComponent transform;
			//transform.scale = glm::vec3(prop.scale);
			//transform.translation = prop.m_Origin;
			//transform.rotation= glm::vec3{glm::pi<float>() *prop.m_Angles.x /180.0f,glm::pi<float>() *prop.m_Angles.z /180.0f,glm::pi<float>() *prop.m_Angles.y /180.0f};
			//glm::mat4x4 mat = transform.mat4();
			float sy = sin(prop.m_Angles.y*(glm::pi<float>()/180.f));
			float sp = sin(prop.m_Angles.x*(glm::pi<float>()/180.f));
			float sr = sin(prop.m_Angles.z*(glm::pi<float>()/180.f));
			float cy = cos(prop.m_Angles.y*(glm::pi<float>()/180.f));
			float cp = cos(prop.m_Angles.x*(glm::pi<float>()/180.f));
			float cr = cos(prop.m_Angles.z*(glm::pi<float>()/180.f));
			glm::mat4 mat = {
				{
					cp*cy*prop.scale,
					cp*sy*prop.scale,
					-1*sp*prop.scale,
					0.0f
				},
				{
					(sp*sr*cy-cr*sy)*prop.scale,
					(sp*sr*sy+cr*cy)*prop.scale,
					sr*cp*prop.scale,
					0.0f
				},
				{
					(sp*cr*cy+sr*sy)*prop.scale,
					(sp*cr*sy-sr*cy)*prop.scale,
					cr*cp*prop.scale,
					0.0f
				},
				{
					prop.m_Origin.x,
					prop.m_Origin.y,
					prop.m_Origin.z,
					0.0f
				}

			};
			//AngleMatrix(prop.m_Angles,prop.m_Origin,prop.scale);

			MdlLoader& mdl = mdls[prop.modelIndex];
			if(mdl.meshes.size()==0)continue;
			for (int i : mdl.meshes[0].indices) {
				Vertex v = mdl.meshes[0].verts[i];
				v.position = mat*glm::vec4(v.position,1);
				
				loadedMesh.addVert(v);
			}
		}

		loadedMesh.finishMesh();
		
		spdlog::info("vertCount {} triCount {}",loadedMesh.verts.size(),loadedMesh.indices.size()/3);
		meshes.push_back(loadedMesh);	
	}
	
};