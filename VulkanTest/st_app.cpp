#include "st_app.h"
#include "simple_render_system.h"
#include "keyboard_movement_controller.h"
#include "st_camera.h"
#include "st_material_management.h"
#include "st_buffer.h"
#include <chrono>
#include <execution>
#include <algorithm>

#include <smmintrin.h>

#include "st_math_lib.h"


#define GLM_FORCE_SSE2
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

const size_t kmeans_node_count = 16;

namespace st {

	struct GlobalUbo {
		glm::mat4 projectionView{ 1.f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
	};

	StApp::StApp()
	{
		globalPool = StDescriptorPool::Builder(stDevice)
			.setMaxSets(StSwapChain::MAX_FRAMES_IN_FLIGHT)
			//.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,StSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, StSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, StSwapChain::MAX_FRAMES_IN_FLIGHT)
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
				StMaterialManager::getManager().getMaterialCount() * 16 * sizeof(uint32_t),
				1,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			histogramBuffer[i]->map();
		}


		auto globalSetLayout = StDescriptorSetLayout::Builder(stDevice)
			//.addBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(StSwapChain::MAX_FRAMES_IN_FLIGHT);



		SimpleRenderSystem simpleRender{ stDevice,stRenderer.getSwapChainRenderPass(),globalSetLayout->getDescriptorSetLayout() };
		StCamera camera{};
		auto viewerObject = StGameObject::createGameObject();
		KeyboardMovementController cameraController{};
		camera.setViewDirection(glm::vec3{ 0.f }, glm::vec3(0.5f, 0.f, 1.f));


		for (size_t i = 0; i < globalDescriptorSets.size(); i++) {
			//auto uboInfo = uboBuffers[i]->descriptorInfo();
			auto histoInfo = histogramBuffer[i]->descriptorInfo();


			StDescriptorWriter(*globalSetLayout, *globalPool)
				//.writeBuffer(0,&uboInfo)
				.writeImage(1, stRenderer.binDescriptorInfo(i))
				.writeBuffer(2, &histoInfo)
				.build(globalDescriptorSets[i]);

		}

		auto currentTime = std::chrono::high_resolution_clock::now();

		std::vector<std::string> matNames = StMaterialManager::getManager().getMaterialNameList();

		while (!stWindow.shouldClose()) {
			glfwPollEvents();
			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			cameraController.moveInPlaneXZ(stWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform3d.translation, viewerObject.transform3d.rotation);
			float aspect = stRenderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(90.0f), aspect, 0.01f, 3000.f);


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

	void calculateCellPositions(Cell& cell);
	void StApp::loadGameObjects() {
		//centers.resize();

		//std::shared_ptr<StModel> stModel = createCubeModel(stDevice,{.0f,.0f,.0f});
		//BspLoader loader{"H:\\r2\\r2_vpk\\maps\\mp_rise.bsp"};
		//BspLoader loader{"H:\\r2\\r2_vpk\\maps\\sp_beacon.bsp"};
		//BspLoader loader{"H:\\r2\\r2_vpk\\maps\\sp_tday.bsp"};
		//BspLoader loader{"H:\\r2\\r2_vpk\\maps\\mp_angel_city.bsp"};
		BspLoader loader{ "/home/krista/code/StbspCreator/vpk/maps/mp_glitch.bsp" };
		//MdlLoader loader{ "H:\\r2\\r2_vpk\\models\\handrails\\handrail_yellow_128.mdl"};
		Mesh::Extends& extends = loader.meshes[0].extends;

		cellXMin = (int)(extends.xMin / 128.f) - 1;
		cellXMax = (int)(extends.xMax / 128.f);
		cellYMin = (int)(extends.yMin / 128.f) - 1;
		cellYMax = (int)(extends.yMax / 128.f);
		cellYRowCount = cellYMax - cellYMin + 1;
		for (int x = cellXMin; x <= cellXMax; x++) {
			for (int y = cellYMin; y <= cellYMax; y++) {
				Cell cell;
				cell.xMin = 128 * x;
				cell.xMax = 128 * (x + 1);
				cell.yMin = 128 * y;
				cell.yMax = 128 * (y + 1);
				cell.xIndex = x;
				cell.yIndex = y;
				cells.push_back(cell);

			}
		}

		for (auto& mesh : loader.meshes) {

			auto cube = StGameObject::createGameObject();
			cube.model = std::make_shared<StModel>(stDevice, mesh);
			cube.transform3d.translation = { 0.f,0.f,0.0f };
			cube.transform3d.scale = { 0.1,0.1,0.1 };//{ 1.0,1.0,1.0 } { 0.1,0.1,0.1 }
			cube.transform3d.rotation = { 0.5f * glm::pi<float>(),0.f,0.f, };
			gameObjects.push_back(std::move(cube));
			createListOfValidCenters(mesh);

		}

		std::for_each(std::execution::par, cells.begin(), cells.end(), [](Cell& cell) {
			calculateCellPositions(cell);
		});

		
		printf("%lld centers", cells.size());
	}


	void StApp::createListOfValidCenters(Mesh& mesh) {
		for (size_t i = 0; i < mesh.indices.size(); i += 3) {
			__m128 v0 = mesh.mathVerts[mesh.indices[i]];
			__m128 v1 = mesh.mathVerts[mesh.indices[i + 1]];
			__m128 v2 = mesh.mathVerts[mesh.indices[i + 2]];

			__m128 normal = faceNormal_ps(v0, v1, v2);
			const static __m128 up = _mm_set_ps(0.f, 1.f, 0.f, 0.f);
			float angle = acosf(_mm_extract_ps(dotProduct_ps(normal, up), 0));
			// int mask = _mm_movemask_ps(_mm_cmpge_ps(angle, _mm_set1_ps(.5)));
			if (angle > .5f) {
				__m128 center = faceCenter_ps(v0, v1, v2);
				__m128 cellIndices = _mm_div_ps(center, _mm_set1_ps(128.f));
				int xIndex = (int)_mm_extract_ps(cellIndices, 0) - cellXMin;
				int yIndex = (int)_mm_extract_ps(cellIndices, 1) - cellYMin;
				if (_mm_extract_ps(center, 0) < 0);
				xIndex--;
				if (_mm_extract_ps(center, 1) < 0);
				yIndex--;
				cells[yIndex + xIndex * cellYRowCount].inputArray.push_back(center);
			}

		}
	}

	void calculateCellPositions(Cell& cell) {
		if (cell.inputArray.size() == 0)return;
		if (cell.inputArray.size() <= kmeans_node_count) {
			cell.outputArray.insert(cell.outputArray.begin(), cell.inputArray.begin(), cell.inputArray.end());
			cell.inputArray.clear();
			return;
		}
		std::vector<__m128>& input = cell.inputArray;
		std::vector<__m128> nodes;
		std::vector<size_t> closestNodeIndex;
		std::vector<size_t> pointsPerNode;
		closestNodeIndex.resize(input.size());
		nodes.reserve(kmeans_node_count);
		
		for (int i = 0; i < kmeans_node_count; i++) {
			__m128 n;
			do {
				n = input[rand()%input.size()];
			} while(std::find_if(nodes.begin(), nodes.end(), [n](const __m128& a)->bool {
				return (_mm_movemask_ps(_mm_cmpeq_ps(a,n))==0xF);
			}) == nodes.end());
			nodes.push_back(n);
		}
		pointsPerNode.resize(nodes.size());
		for (int iterations = 0; iterations < 32; iterations++) {
			for (size_t i = 0; i < input.size(); i++) {
				__m128 closestDistance = magnitude_ps(_mm_sub_ps(nodes[0], input[i]));
				size_t closestIndex = 0;
				for (size_t j = 1; j < nodes.size(); j++) {
					__m128 newDist = magnitude_ps(_mm_sub_ps(nodes[j], input[i]));
					if (_mm_movemask_ps(_mm_cmplt_ps(newDist, closestDistance))) {
						closestDistance = newDist;
						closestIndex = j;
					}
				}
				closestNodeIndex[i] = closestIndex;
			}
			for (size_t i = 0; i < nodes.size(); i++) {
				nodes[i] = _mm_setzero_ps();
				pointsPerNode[i] = 0;
			}

			for (size_t i = 0; i < input.size(); i++) {
				nodes[closestNodeIndex[i]] = _mm_add_ps(nodes[closestNodeIndex[i]], input[i]);
				pointsPerNode[closestNodeIndex[i]]++;
			}
			for (size_t i = 0; i < nodes.size(); i++) {
				nodes[i] = _mm_div_ps(nodes[i], _mm_set1_ps(pointsPerNode[i]));
			}
		}
	}

}