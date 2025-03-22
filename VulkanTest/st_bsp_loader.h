#pragma once

#include "st_model.h"

#define GLM_FORCE_SSE2
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <iostream>
#include <fstream>


#define MAGIC(a, b, c, d)  ((a << 0) | (b << 8) | (c << 16) | (d << 24))
#define MAGIC_rBSP  MAGIC('r', 'B', 'S', 'P')


namespace st {

	typedef glm::vec2 Vector2D;
	typedef glm::vec3 Vector;
	typedef glm::vec4 Vector4D;

	typedef glm::vec3 Vector3;


	struct LumpHeader {
		uint32_t offset;
		uint32_t length;
		uint32_t version;
		uint32_t fourCC;
	};

	struct BspHeader {
		uint32_t   magic;
		uint32_t   version;
		uint32_t   revision;
		uint32_t   _127;
		LumpHeader lumps[128];
	};

	struct VertexUnlit {
		int vertexIndex;
		int normalIndex;
		glm::vec2 albedoUv;
		uint32_t color;
	};

	struct VertexLitFlat {
		uint32_t vertexIndex;
		uint32_t normalIndex;
		glm::vec2 albedoUv;
		uint32_t color;
		float lightMapUv[2];
		float lightMapXy[2];
	};

	struct VertexLitBump {
		int vertexIndex;
		int normalIndex;
		glm::vec2 albedoUv;
		uint32_t color;
		float lightMapUv[2];
		float lightMapXy[2];
		int tangent[2];
	};

	struct VertexUnlitTS{
		int vertexIndex;
		int normalIndex;
		glm::vec2 albedoUv;
		uint32_t color;
		uint32_t unk[2];
	};

	struct VertexBlinnPhong {
		int vertexIndex;
		int normalIndex;
		uint32_t color;
		float uv[4];
		float tangent[16];
	};

	struct BspMesh{

		unsigned int first_mesh_index;
		unsigned short num_triangles;
		unsigned short first_vertex;
		unsigned short num_vertices;
		unsigned short vertex_type;
		BYTE styles[4];
		short luxel_origin[2];
		BYTE luxel_offset_max[2];
		unsigned short material_sort;
		unsigned int meshFlags;

	};

	struct MaterialSort {
		short textureData;
		short lightMapHeader;
		short cubemap;
		short lastVertex;
		int vertexOffset;
	};

	struct TextureData {
		float reflectivity[3];
		int nameStringId;
		int width;
		int height;
		int view_width;
		int view_height;
		int flags;
	};

	struct StaticProp
	{
		Vector3 m_Origin;
		Vector3 m_Angles;
		float scale;
		uint16_t modelIndex;
		BYTE m_Solid;
		BYTE m_flags;
		WORD skin;
		WORD word_22;
		float forced_fade_scale;
		Vector3 m_LightingOrigin;
		uint8_t m_DiffuseModulation_r;
		uint8_t m_DiffuseModulation_g;
		uint8_t m_DiffuseModulation_b;
		uint8_t m_DiffuseModulation_a;
		int unk;
		DWORD collision_flags_remove;
	};

	class BspLoader {
		

    public:
		
		BspLoader(const char* fileName, bool multipleMeshes = false) {
			if(multipleMeshes)
				loadFileMultipleMeshes(fileName);
			else
				loadFileSingleMesh(fileName);
		}



		void loadFileMultipleMeshes(const char* fileName);
		void loadFileSingleMesh(const char* fileName);
        std::vector<Mesh> meshes;
		
	private:

        template<typename T> std::vector<T> loadLump(int index);

        
		BspHeader header{};
        std::ifstream file{};

	};

}