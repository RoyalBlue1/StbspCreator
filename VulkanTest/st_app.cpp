#include "st_app.h"
#include "simple_render_system.h"
#include "keyboard_movement_controller.h"
#include "st_camera.h"
#include "st_material_management.h"
#include "st_buffer.h"
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace st {

    struct GlobalUbo {
        glm::mat4 projectionView{1.f};
        glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f, -3.f, -1.f});
    };
	
	StApp::StApp()
    {
        globalPool = StDescriptorPool::Builder(stDevice)
            .setMaxSets(StSwapChain::MAX_FRAMES_IN_FLIGHT)
            //.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,StSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,StSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,StSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
		loadGameObjects();

	}
	StApp::~StApp() {
		
	}

	
	
	void StApp::run() {
        //std::vector<std::unique_ptr<StBuffer>> uboBuffers(StSwapChain::MAX_FRAMES_IN_FLIGHT);
        //for (size_t i = 0; i < uboBuffers.size(); i++) {
        //    uboBuffers[i] = std::make_unique<StBuffer>(
        //        stDevice,
        //        sizeof(GlobalUbo),
        //        1,
        //        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        //        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        //    uboBuffers[i]->map();
        //}
        std::vector<std::unique_ptr<StBuffer>> histogramBuffer(StSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < histogramBuffer.size(); i++) {
            histogramBuffer[i] = std::make_unique<StBuffer>(
                stDevice,
                StMaterialManager::getManager().getMaterialCount()*16*sizeof(uint32_t),
                1,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            histogramBuffer[i]->map();
        }


        auto globalSetLayout = StDescriptorSetLayout::Builder(stDevice)
            //.addBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT)
            .addBinding(1,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(StSwapChain::MAX_FRAMES_IN_FLIGHT);



		SimpleRenderSystem simpleRender{stDevice,stRenderer.getSwapChainRenderPass(),globalSetLayout->getDescriptorSetLayout()};
        StCamera camera{};
        auto viewerObject = StGameObject::createGameObject();
        KeyboardMovementController cameraController{};
        camera.setViewDirection(glm::vec3{0.f},glm::vec3(0.5f,0.f,1.f));


        for (size_t i = 0; i < globalDescriptorSets.size(); i++) {
            //auto uboInfo = uboBuffers[i]->descriptorInfo();
            auto histoInfo = histogramBuffer[i]->descriptorInfo();


            StDescriptorWriter(*globalSetLayout,*globalPool)
                //.writeBuffer(0,&uboInfo)
                .writeImage(1,stRenderer.binDescriptorInfo(i))
                .writeBuffer(2,&histoInfo)
                .build(globalDescriptorSets[i]);

        }

        auto currentTime = std::chrono::high_resolution_clock::now();

        std::vector<std::string> matNames = StMaterialManager::getManager().getMaterialNameList();

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
                
                int frameIndex = stRenderer.getFrameIndex();
                FrameInfo frameInfo{ frameIndex, frameTime, commandBuffer, camera,globalDescriptorSets[frameIndex] };

                // update
                // GlobalUbo ubo{};
                //ubo.projectionView = camera.getProjection() * camera.getView();
                //uboBuffers[frameIndex]->writeToBuffer(&ubo);
                //uboBuffers[frameIndex]->flush();

                //uint32_t* histogramData = (uint32_t*)histogramBuffer[frameIndex]->getMappedMemory();


                
                // render
                stRenderer.beginSwapChainRenderpass(commandBuffer);
                simpleRender.renderGameObjects(frameInfo, gameObjects);
                

                //if (glfwGetKey(stWindow.getGLFWwindow(), GLFW_KEY_P) == GLFW_PRESS) {
                //    
                //    for (int j = 0; j < matNames.size(); j++) {
                //        int j = 230;
                //        std::string print = matNames[j];
                //        for (int i = 0; i < 16; i++) {
                //            print = std::format("{} {}", print, histogramData[j * 16 + i]);
                //        }
                //        printf("%s\n",print.c_str());
                //    }
                //    spdlog::info("{}",frameTime);
                //}
                

                //memset(histogramData, 0, histogramBuffer[frameIndex]->getBufferSize());
                //histogramBuffer[frameIndex]->flush();
                

				stRenderer.endSwapChainRenderpass(commandBuffer);
                //simpleRender.computeHistogram(commandBuffer,stWindow.getExtent().width,stWindow.getExtent().height, &globalDescriptorSets[frameIndex]);

				stRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(stDevice.device());
	}


    void StApp::loadGameObjects() {
        //std::shared_ptr<StModel> stModel = createCubeModel(stDevice,{.0f,.0f,.0f});
        //BspLoader loader{"H:\\r2\\r2_vpk\\maps\\mp_rise.bsp"};
        BspLoader loader{"E:\\Titanfall2\\R2Northstar\\mods\\bobthebob.mp_runoff\\mod\\maps\\mp_runoff.bsp"};
        //MdlLoader loader{ "H:\\r2\\r2_vpk\\models\\handrails\\handrail_yellow_128.mdl"};
        for (auto& mesh : loader.meshes) {

            auto cube = StGameObject::createGameObject();
            cube.model = std::make_shared<StModel>(stDevice,mesh);
            cube.transform3d.translation = {0.f,0.f,0.0f};
            cube.transform3d.scale = { 0.1,0.1,0.1 };//{ 1.0,1.0,1.0 } { 0.1,0.1,0.1 }
            cube.transform3d.rotation = {0.5f*glm::pi<float>(),0.f,0.f,};
            gameObjects.push_back(std::move(cube));
        }
    }
}