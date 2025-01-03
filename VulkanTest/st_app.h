#pragma once

#include "st_window.h"

#include "st_renderer.h"
#include "st_game_object.h"
#include "st_bsp_loader.h"
#include <memory>


namespace st {
	class StApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;
		void run();
		StApp();
		~StApp();

		StApp(const StApp&) = delete;
		StApp &operator=(const StApp &)=delete;
	private:
		void loadGameObjects();

		

		StWindow stWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
		StDevice stDevice{stWindow};
		StRenderer stRenderer{stWindow,stDevice};
		std::vector<StGameObject> gameObjects;
	};
}  