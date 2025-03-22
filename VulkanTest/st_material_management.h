#pragma once

#include <vector>
#include <string>
#include <optional>

#include "spdlog/spdlog.h"

namespace st {

	class StMaterial {
		std::string name;
		uint32_t index;
		StMaterial(std::string matName, uint32_t id) :name{ matName },index { id } {};

		friend class StMaterialManager;
	};



	class StMaterialManager {
	public:

		static StMaterialManager& getManager() {
			static StMaterialManager matManager{};
			return matManager;
		}


		uint32_t addMaterial(std::string name){
			for (auto& mat : materials) {
				if(mat.name == name)return mat.index;
			}


			materials.push_back(StMaterial{ name,lastIndex });
			spdlog::info("mat {} {}",lastIndex,name);
			return lastIndex++;
		}

		std::vector<std::string> getMaterialNameList() {
			std::vector<std::string> ret;
			for(const auto&mat:materials)
				ret.push_back(mat.name);
			return ret;
		}

		std::string getMaterialName(uint32_t id) {
			for (auto& mat: materials) {
				if(mat.index==id)return mat.name;
			}
			return "";
		}
		uint32_t getMaterialCount() {
			return materials.size();
		}


	private:
		std::vector<StMaterial> materials;
		uint32_t lastIndex;
	};


}