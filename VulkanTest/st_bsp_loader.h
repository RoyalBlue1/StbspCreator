#pragma once

#include "st_model.h"
#include "bsp_types.h"
#include "st_mdl_loader.h"
#define GLM_FORCE_SSE2
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "st_math_lib.h"

#include <iostream>
#include <fstream>
#include <set>

#define MAGIC(a, b, c, d)  ((a << 0) | (b << 8) | (c << 16) | (d << 24))
#define MAGIC_rBSP  MAGIC('r', 'B', 'S', 'P')


namespace st {

	struct Cell {
		float xMin;
		float xMax;
		float yMin;
		float yMax;
		int xIndex;
		int yIndex;
		std::vector<__m128> inputArray;
		std::vector<__m128> outputArray;
	};
	

	class BspLoader {
		

    public:
		
		BspLoader(const char* fileName, bool multipleMeshes = false) {
			if(multipleMeshes)
				loadFileMultipleMeshes(fileName);
			else
				loadFileSingleMesh(fileName);
			loadCollision(fileName);
		}



		void loadFileMultipleMeshes(const char* fileName);
		void loadFileSingleMesh(const char* fileName);
		void loadCollision(const char* fileName);
		bool doesPointCollide(__m128 point);

		std::set<uint32_t> alreadyAddedPrimitives;
		CMGrid grid;
		int cellYRowCount;
		int cellXMin;
		int cellXMax;
		int cellYMin;
		int cellYMax;
		std::vector<Cell> cells;
		std::vector<Mesh> meshes;
	private:

        template<typename T> std::vector<T> loadLump(int index);

        
		BspHeader header{};
        std::ifstream file{};

		
		std::vector<Tricoll_Header> tricollHeader;
		std::vector<GeoSet> geoSets;
		std::vector<uint32_t> primitives;
		std::vector<uint32_t> tricollTris;
		std::vector<uint32_t> uniqueContents;
		std::vector<MdlLoader> mdls;
		std::vector<StaticProp> props;
		std::vector<glm::vec3> vertices;
		std::vector<Brush> brushes;
		std::vector<uint16_t> brushSidePlaneOffsets;
		std::vector<Vector4> brushPlanes;
		std::vector<GridCell> gridCells;
		std::vector<__m128i> geoSetBounds;
		std::vector<__m128i> primitiveBounds;
		bool doesPointCollidePrimitive(__m128 point, uint32_t primitive); 
		void addPrimitive(uint32_t prim);
		void addFace(__m128 v0,__m128 v1,__m128 v2);
		void addPoint(__m128 p);
	};

}