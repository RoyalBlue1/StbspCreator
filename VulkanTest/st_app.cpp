#include "st_app.h"
#include "simple_render_system.h"
#include "keyboard_movement_controller.h"
#include "st_camera.h"
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace st {


	
	StApp::StApp() {
		loadGameObjects();

        
	}
	StApp::~StApp() {
		
	}

	
	
	void StApp::run() {
		SimpleRenderSystem simpleRender{stDevice,stRenderer.getSwapChainRenderPass()};
        StCamera camera{};
        auto viewerObject = StGameObject::createGameObject();
        KeyboardMovementController cameraController{};
        camera.setViewDirection(glm::vec3{0.f},glm::vec3(0.5f,0.f,1.f));

        auto currentTime = std::chrono::high_resolution_clock::now();

        

		while (!stWindow.shouldClose()) {
			glfwPollEvents();
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float,std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            cameraController.moveInPlaneXZ(stWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform3d.translation, viewerObject.transform3d.rotation);
            float aspect = stRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(90.0f), aspect,0.01f ,20000.f );
			if (auto commandBuffer = stRenderer.beginFrame()) {
				stRenderer.beginSwapChainRenderpass(commandBuffer);

				simpleRender.renderGameObjects(commandBuffer,gameObjects,camera);

				stRenderer.endSwapChainRenderpass(commandBuffer);
				stRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(stDevice.device());
	}


    void StApp::loadGameObjects() {
        //std::shared_ptr<StModel> stModel = createCubeModel(stDevice,{.0f,.0f,.0f});
        BspLoader loader{};
        //loader.loadFileMultipleMeshes("E:\\tf2Unpacked\\mp\\maps\\bsp\\mp_rise.bsp");
        loader.loadFileSingleMesh("E:\\tf2Unpacked\\mp\\maps\\bsp\\mp_rise.bsp");
        for (auto& mesh : loader.meshes) {
            StModel::Builder builder;
            builder.indices = mesh.indices;
            builder.vertices = mesh.verts;
            auto cube = StGameObject::createGameObject();
            cube.model = std::make_shared<StModel>(stDevice,builder);
            cube.transform3d.translation = {0.f,0.f,0.0f};
            cube.transform3d.scale = {0.01f,0.01f,0.01f};
            cube.transform3d.rotation = {0.5f*glm::pi<float>(),0.f,0.f,};
            gameObjects.push_back(std::move(cube));
        }
    }
}