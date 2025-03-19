#pragma once 

#include "st_model.h"
#include "studio.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <iostream>
#include <fstream>


namespace st{
	class MdlLoader {
	public:
		MdlLoader(const char* fileName, bool multipleMeshes = false) {
			if(multipleMeshes)
				loadFileMultipleMeshes(fileName);
			else
				loadFileSingleMesh(fileName);
		}



		void loadFileMultipleMeshes(const char* fileName);
		void loadFileSingleMesh(const char* fileName);
		std::string name;
		std::vector<Mesh> meshes;
	private:




		r2::studiohdr_t header{};
		std::ifstream file{};

	};

}