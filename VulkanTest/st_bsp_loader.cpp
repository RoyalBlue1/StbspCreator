#include "st_bsp_loader.h"

#include "st_utils.h"
#include <unordered_map>
#include "st_material_management.h"
#include "st_game_object.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "st_math_lib.h"
#include "st_settings_controller.h"
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

	glm::mat4 createTransformationMatrix_Source(glm::vec3 angles, glm::vec3 origin,float scale) {
		float sy = sin(angles.y*(glm::pi<float>()/180.f));
		float sp = sin(angles.x*(glm::pi<float>()/180.f));
		float sr = sin(angles.z*(glm::pi<float>()/180.f));
		float cy = cos(angles.y*(glm::pi<float>()/180.f));
		float cp = cos(angles.x*(glm::pi<float>()/180.f));
		float cr = cos(angles.z*(glm::pi<float>()/180.f));
		return glm::mat4 {
			{
				cp*cy*scale,
				cp*sy*scale,
				-1*sp*scale,
				0.0f
			},
			{
				(sp*sr*cy-cr*sy)*scale,
				(sp*sr*sy+cr*cy)*scale,
				sr*cp*scale,
				0.0f
			},
			{
				(sp*cr*cy+sr*sy)*scale,
				(sp*cr*sy-sr*cy)*scale,
				cr*cp*scale,
				0.0f
			},
			{
				origin.x,
				origin.y,
				origin.z,
				0.0f
			}

		};
	}


	void BspLoader::loadFileSingleMesh(const char* fileName) {


		file.open(fileName,std::ios::binary);
		file.seekg(0,std::ios::beg);
		file.read((char*) & header, sizeof(header));
		if(header.magic!=MAGIC_rBSP)return;

		vertices = loadLump<glm::vec3>(VERTICES);
		std::vector<glm::vec3> normals = loadLump<glm::vec3>(VERTEX_NORMALS);


		std::vector<VertexUnlit> vertex_unlit = loadLump<VertexUnlit>(VERTEX_UNLIT);
		std::vector<VertexLitFlat> vertex_lit_flat = loadLump<VertexLitFlat>(VERTEX_LIT_FLAT);
		std::vector<VertexLitBump> vertex_lit_bump = loadLump<VertexLitBump>(VERTEX_LIT_BUMP);
		std::vector<VertexUnlitTS> vertex_unlit_ts = loadLump<VertexUnlitTS>(VERTEX_UNLIT_TS);

		std::vector<uint16_t> indi = loadLump<uint16_t>(MESH_INDICES);
		std::vector<BspMesh> bspMeshes = loadLump<BspMesh>(MESHES);
		std::vector<MaterialSort> materialSorts = loadLump<MaterialSort>(MATERIAL_SORTS);

		std::vector<uint32_t> textureStringTable = loadLump<uint32_t>(TEXTURE_DATA_STRING_TABLE);
		std::vector<char> textureStringData = loadLump<char>(TEXTURE_DATA_STRING_DATA);
		std::vector<TextureData> textureData = loadLump<TextureData>(TEXTURE_DATA);
		
		std::vector<char> gameLump = loadLump<char>(GAME_LUMP);

		std::unordered_map<Vertex,size_t> vertBuildList;
		Mesh loadedMesh{};
		for (auto& bspMesh : bspMeshes) {

			if(bspMesh.meshFlags&((int)MeshFlags::TRIGGER|(int)MeshFlags::TRIGGER))continue;

			if(bspMesh.meshFlags&TRANSLUCENT)continue;
			
			uint32_t vertexOffset = materialSorts[bspMesh.material_sort].vertexOffset;
			uint32_t vertexOffset2 = bspMesh.first_vertex;
			uint32_t materialBspId = materialSorts[bspMesh.material_sort].textureData;
			auto& texture = textureData[materialBspId];
			std::string materialName = std::string(&textureStringData[textureStringTable[texture.nameStringId]]);
			uint32_t materialId = StMaterialManager::getManager().addMaterial(materialName);
			for (int j = 0; j < bspMesh.num_triangles * 3; j++) {



				uint32_t vertexIndex = indi[(size_t)j+bspMesh.first_mesh_index]+vertexOffset;
				Vertex v;
				switch (bspMesh.meshFlags & (int)MeshFlags::MASK_VERTEX) {
				case (int)MeshFlags::VERTEX_LIT_FLAT:
				{
					auto& vert = vertex_lit_flat[vertexIndex];
					v.position = vertices[vert.vertexIndex&0x7FFFFFFF];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case (int)MeshFlags::VERTEX_LIT_BUMP:
				{
					auto& vert = vertex_lit_bump[vertexIndex];
					v.position = vertices[vert.vertexIndex&0x7FFFFFFF];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case (int)MeshFlags::VERTEX_UNLIT:
				{
					auto& vert = vertex_unlit[vertexIndex];
					v.position = vertices[vert.vertexIndex&0x7FFFFFFF];
					//v.normal = normals[vert.normalIndex];
					//v.color = { ((vert.color) & 0xFF) / 255.0,((vert.color >> 8) & 0xFF) / 255.0,((vert.color >> 16) & 0xFF) / 255.0 };
					v.uv = vert.albedoUv;
				}
				break;
				case (int)MeshFlags::VERTEX_UNLIT_TS:
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

	
			StaticProp prop;
			memcpy(&prop, &gameLump[readPtr], sizeof(prop));
			readPtr += sizeof(prop);
			props.push_back(prop);
		}
		for(auto& prop:props){
			//Transform3dComponent transform;
			//transform.scale = glm::vec3(prop.scale);
			//transform.translation = prop.m_Origin;
			//transform.rotation= glm::vec3{glm::pi<float>() *prop.m_Angles.x /180.0f,glm::pi<float>() *prop.m_Angles.z /180.0f,glm::pi<float>() *prop.m_Angles.y /180.0f};
			//glm::mat4x4 mat = transform.mat4();
			glm::mat4 mat = createTransformationMatrix_Source(prop.m_Angles,prop.m_Origin,prop.scale);
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
		file.close();
	}

	void BspLoader::addPrimitive(uint32_t prim){
		uint32_t flags = uniqueContents[prim&0xFF];
		uint32_t index = (prim>>8)&0x1FFFFF;
		uint32_t type = (prim>>29)&0x7;
		if((flags&PLAYER_CLIP)==0)return;
		//if((flags&SOLID)==0)return;

		if (alreadyAddedPrimitives.contains(index+(type<<21)))return;
		alreadyAddedPrimitives.insert(index+(type<<21));
		switch ((PrimitiveType)type) {
		case PrimitiveType::Brush:
		{
			
			Brush& brush = brushes[index];
			uint32_t priorNonAxialCount = brush.brush_side_offset;
			std::vector<Vector4> planes;
			
			for (int i = 0; i < brush.num_plane_offsets; i++) {
				uint32_t planeIndex = grid.basePlaneOffset + i + priorNonAxialCount - brushSidePlaneOffsets[priorNonAxialCount+i];
				planes.push_back(brushPlanes[planeIndex]);
			}
			float brushGridSize = StSettingsManager::getManager().brushProbeGenerationGridSize;

			for (float x = -brush.extends.x; x < brush.extends.x; x += brushGridSize ) {
				for (float y = -brush.extends.y; y < brush.extends.y; y += brushGridSize ) {

					bool inside = true;
					for (const auto& plane : planes) {
						float side = glm::dot(plane, glm::vec4{ x,y,brush.extends.z,1.f });
						if (side < 0) {
							inside = false;
							break;
						}
					}
					if (inside) {
						addPoint(_mm_set_ps(0.f,brush.origin.z + brush.extends.z,brush.origin.y+y,brush.origin.x+x));
						continue;
					}


					for (const auto& plane : planes) {
						float angle = acosf(plane.z);//all other numbers shorten out
						if(angle<.75f)continue;
						float z = (x*plane.x+y*plane.y-plane.w)/-plane.z;
						bool inside = true;
						if(z>brush.extends.z||z<(-brush.extends.z))continue;
						for (const auto& p : planes) {
							if(p==plane)continue;
							float side = glm::dot(p, glm::vec4{ x,y,z,1.f });
							if (side < 0) {
								inside = false;
								break;
							}
						}
						if (inside) {
							addPoint(_mm_set_ps(0.f,z+brush.origin.z,y+brush.origin.y,x+brush.origin.x));
							break;
						}
					}
				}
			}
		}
			break;
		case PrimitiveType::Ticoll:
		{
			const Tricoll_Header& hdr = tricollHeader[index];
			for (int i = 0; i < hdr.trisCount; i++) {
				uint32_t tri = tricollTris[hdr.trisStart + i];
				glm::vec3& v0 = vertices[hdr.vertStart + (tri & 0x3FF)];
				glm::vec3& v1 = vertices[hdr.vertStart + ((tri >> 10) & 0x7F)];
				glm::vec3& v2 = vertices[hdr.vertStart + ((tri >> 17) & 0x7F)];
				addFace(_mm_set_ps(0.f, v0.z, v0.y, v0.x), _mm_set_ps(0.f, v1.z, v1.y, v1.x), _mm_set_ps(0.f, v2.z, v2.y, v2.x));
			}
		}
			break;
		case PrimitiveType::Prop:
			if(index>=props.size())break;
			StaticProp& prop = props[index];
			MdlLoader& mdl = mdls[prop.modelIndex];
			glm::mat4 transform = createTransformationMatrix_Source(prop.m_Angles,prop.m_Origin,prop.scale);
			std::vector<__m128> mdlVerts;
			for (const auto& v : mdl.collVerts) {
				glm::vec4 newV = transform* glm::vec4{v,1.f};
				mdlVerts.push_back(_mm_set_ps(0,v.z,v.y,v.x));
			}
			for (int i = 0; i < mdl.collIndices.size(); i += 3) {
				addFace(mdlVerts[mdl.collIndices[i]],mdlVerts[mdl.collIndices[i+1]],mdlVerts[mdl.collIndices[i+2]]);
			}
			break;
		}
	}

	bool vecContains(std::vector<__m128>& vec, __m128& a) {
		for (__m128& b : vec) {
			if(_mm_movemask_ps(_mm_cmpeq_ps(a,b))==0xF)return true;
		}
		return false;
	}

	void BspLoader::addFace(__m128 v0, __m128 v1, __m128 v2) {
		__m128 normal = faceNormal_ps(v0, v1, v2);
		const static __m128 up = _mm_set_ps(0.f, -1.f, 0.f, 0.f);
		__m128 angle = _mm_acos_ps(dotProduct_ps(up,normal));
		int mask = _mm_movemask_ps(_mm_cmpge_ps(angle, _mm_set1_ps(.5)));
		if (mask) {
			__m128 center = faceCenter_ps(v0, v1, v2);
			addPoint(center);
		}
	}

	void BspLoader::addPoint(__m128 point) {

		if(point.m128_f32[2]>StSettingsManager::getManager().maxProbeZ)return;

		__m128 cellIndices = _mm_div_ps(point, _mm_set1_ps(128.f));
		int xIndex = (int)cellIndices.m128_f32[0] - cellXMin;
		int yIndex = (int)cellIndices.m128_f32[1] - cellYMin;
		if (point.m128_f32[0] < 0);
		xIndex--;
		if (point.m128_f32[1] < 0);
		yIndex--;
		std::vector<__m128>& input = cells[yIndex + xIndex * cellYRowCount].inputArray;
		__m128 p = _mm_add_ps(point,_mm_set_ps(0.f,StSettingsManager::getManager().probeHeight, 0.f, 0.f));
		if(!vecContains(input,p))input.push_back(p);
	}

	void BspLoader::loadCollision(const char* fileName) {
		file.open(fileName,std::ios::binary);
		file.seekg(0,std::ios::beg);
		file.read((char*) & header, sizeof(header));
		if(header.magic!=MAGIC_rBSP)return;

		
		tricollHeader = loadLump<Tricoll_Header>(TRICOLL_HEADERS);
		geoSets = loadLump<GeoSet>(CM_GEO_SETS);
		primitives = loadLump<uint32_t>(CM_PRIMITIVES);
		tricollTris = loadLump<uint32_t>(TRICOLL_TRIANGLES);
		uniqueContents = loadLump<uint32_t>(CM_UNIQUE_CONTENTS);
		brushes = loadLump<Brush>(CM_BRUSHES);
		brushSidePlaneOffsets = loadLump<uint16_t>(CM_BRUSH_SIDE_PLANE_OFFSETS);
		brushPlanes = loadLump<Vector4>(BRUSH_PLANES);
		std::vector<Vector3> verts = loadLump<Vector3>(VERTICES);
		grid = loadLump<CMGrid>(CM_GRID)[0];
		float xMin = grid.cellSize*grid.cellOrg[0];
		float xMax = grid.cellSize*(grid.cellOrg[0]+grid.cellCount[0]);
		float yMin = grid.cellSize*grid.cellOrg[1];
		float yMax = grid.cellSize*(grid.cellOrg[1]+grid.cellCount[1]);


		
		float cellSize = StSettingsManager::getManager().cellSize;
		cellXMin = (int)(xMin / cellSize ) - 1;
		cellXMax = (int)(xMax / cellSize );
		cellYMin = (int)(yMin / cellSize ) - 1;
		cellYMax = (int)(yMax / cellSize );
		cellYRowCount = cellYMax - cellYMin + 1;
		for (int x = cellXMin; x <= cellXMax; x++) {
			for (int y = cellYMin; y <= cellYMax; y++) {
				Cell cell;
				cell.xMin = cellSize  * x;
				cell.xMax = cellSize  * (x + 1);
				cell.yMin = cellSize  * y;
				cell.yMax = cellSize  * (y + 1);
				cell.xIndex = x;
				cell.yIndex = y;
				cells.push_back(cell);

			}
		}

		for (const auto& geoSet : geoSets) {
			if (geoSet.primCount == 1) {
				addPrimitive(geoSet.primStart);
			}
			else {
				for (int i = 0; i < geoSet.primCount; i++) {
					addPrimitive(primitives[((geoSet.primStart>>8)&0x1FFFFF)+i]);
				}
			}
		}

		file.close();
	}


};