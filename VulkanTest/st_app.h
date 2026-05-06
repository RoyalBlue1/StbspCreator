#pragma once

#include "st_window.h"

#include "st_renderer.h"
#include "st_game_object.h"
#include "st_bsp_loader.h"
#include "st_mdl_loader.h"
#include "st_buffer.h"
#include "st_descriptors.h"
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

namespace st {
	class StApp {
	public:
		static constexpr int WIDTH = 1024;
		static constexpr int HEIGHT = 1024;
		void run();
		StApp(fs::path bspFilePath);
		~StApp();

		StApp(const StApp&) = delete;
		StApp &operator=(const StApp &)=delete;
	private:
		void loadGameObjects(fs::path bspFilePath);

		

		StWindow stWindow{WIDTH, HEIGHT, "StBspGen"};
		StDevice stDevice{stWindow};
		StRenderer stRenderer{stWindow,stDevice};

		std::unique_ptr<StDescriptorPool> globalPool{};
		std::vector<Cell> cells;
		std::vector<StGameObject> gameObjects;
		
		//void createListOfValidCenters(Mesh& mesh);
	};
}  