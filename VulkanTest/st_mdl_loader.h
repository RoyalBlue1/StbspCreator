#pragma once 

#include "st_model.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <iostream>
#include <fstream>

namespace st{
	class MdlLoader {
	public:
		struct Mesh {
			std::vector<Vertex> verts;
			std::vector<uint32_t> indices;
			std::string material;
		};




		void loadFileMultipleMeshes(const char* fileName);
		void loadFileSingleMesh(const char* fileName);
		std::vector<Mesh> meshes;
	private:




		StudioHeader header{};
		std::ifstream file{};

	};

}