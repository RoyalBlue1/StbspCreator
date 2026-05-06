#include "st_app.h"
#include "simple_render_system.h"
#include "keyboard_movement_controller.h"
#include "st_camera.h"
#include "st_material_management.h"
#include "st_buffer.h"
#include <chrono>
#include <execution>
#include <algorithm>


#include "st_math_lib.h"
#include "st_settings_controller.h"

#define GLM_FORCE_SSE2
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>



namespace st {

	struct GlobalUbo {
		glm::mat4 projectionView{ 1.f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
	};

	StApp::StApp(fs::path bspFilePath)
	{
		globalPool = StDescriptorPool::Builder(stDevice)
			.setMaxSets(StSwapChain::MAX_FRAMES_IN_FLIGHT)
			//.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,StSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, StSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, StSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		loadGameObjects(bspFilePath);

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
			memset(histogramBuffer[i]->getMappedMemory(),0,histogramBuffer[i]->getInstanceSize());
			histogramBuffer[i]->unmap();
		}


		auto globalSetLayout = StDescriptorSetLayout::Builder(stDevice)
			//.addBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.build();


		std::vector<VkDescriptorSet> globalDescriptorSets(StSwapChain::MAX_FRAMES_IN_FLIGHT);





		SimpleRenderSystem simpleRender{ stDevice,stRenderer.getSwapChainRenderPass(),globalSetLayout->getDescriptorSetLayout()};
		StCamera camera{};
		//auto viewerObject = StGameObject::createGameObject();
		//KeyboardMovementController cameraController{};
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
		std::ofstream debugData("test.txt");
		int count = 0;

		for (auto& cell : cells) {
			for (auto cubeMapPos : cell.outputArray) {
				float pos[4];
				_mm_store_ps(pos, cubeMapPos);
				debugData << std::format("< {}, {}, {} >,\n",
					pos[0], pos[1], pos[2]);
				if(count++>32000)break;
			}
			if(count>32000)break;
		}
		debugData.close();
		bool shouldClose = false;
		for (auto& cell : cells) {
			if (!cell.outputArray.size())continue;
			for (auto cubeMapPos : cell.outputArray) {
				for (int sideIndex = 0; sideIndex < 6; sideIndex++) {
					bool shouldClose = stWindow.shouldClose();
					if(shouldClose)break;
					glfwPollEvents();
					auto newTime = std::chrono::high_resolution_clock::now();
					float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
					currentTime = newTime;


					float x = _mm_cvtss_f32(cubeMapPos);
					float y = _mm_cvtss_f32(_mm_shuffle_ps(cubeMapPos, cubeMapPos, _MM_SHUFFLE(0, 0, 0, 1)));
					float z = _mm_cvtss_f32(_mm_shuffle_ps(cubeMapPos, cubeMapPos, _MM_SHUFFLE(0, 0, 0, 2)));

					//cameraController.moveInPlaneXZ(stWindow.getGLFWwindow(), frameTime, viewerObject);
					static std::vector<glm::vec3> sides = {
						{0.f,0.f,0.f},
						{0.f,glm::radians(90.f),0.f},
						{0.f,glm::radians(180.f),0.f},
						{0.f,glm::radians(270.f),0.f},
						{glm::radians(90.f),0.f,0.f},
						{glm::radians(-90.f),0.f,0.f}
					};


					camera.setViewYXZ(glm::vec3{ x,-z,y }, sides[sideIndex]);
					float aspect = stRenderer.getAspectRatio();
					camera.setPerspectiveProjection(glm::radians(90.0f), aspect, 0.01f, 30000.f);


					if (auto commandBuffer = stRenderer.beginFrame()) {

						int frameIndex = stRenderer.getFrameIndex();
						FrameInfo frameInfo{ frameIndex, frameTime, commandBuffer, camera,globalDescriptorSets[frameIndex] };

						// update
						// GlobalUbo ubo{};
						//ubo.projectionView = camera.getProjection() * camera.getView();
						//uboBuffers[frameIndex]->writeToBuffer(&ubo);
						//uboBuffers[frameIndex]->flush();

						



						// render
						stRenderer.beginSwapChainRenderpass(commandBuffer);
						simpleRender.renderGameObjects(frameInfo, gameObjects);


						//if (glfwGetKey(stWindow.getGLFWwindow(), GLFW_KEY_P) == GLFW_PRESS) {
						//    
						//    for (int j = 0; j < matNames.size(); j++) {
						//        
						//        std::string print = matNames[j];
						//        for (int i = 0; i < 16; i++) {
						//            print = std::format("{} {}", print, histogramData[j * 16 + i]);
						//        }
						//        printf("%s\n",print.c_str());
						//    }
						//    spdlog::info("{}",frameTime);
						//}


						
						


						stRenderer.endSwapChainRenderpass(commandBuffer);
						stRenderer.binImageComputeStartBarrier(commandBuffer);
						simpleRender.computeHistogram(commandBuffer,stWindow.getExtent().width,stWindow.getExtent().height,StMaterialManager::getManager().getMaterialCount(), &globalDescriptorSets[frameIndex]);
						stRenderer.binImageComputeEndBarrier(commandBuffer);

						stRenderer.endFrame();

					}
				}
				if(shouldClose)break;
			}
			//read histogram here
			vkDeviceWaitIdle(stDevice.device());
			cell.histogramData.resize(StMaterialManager::getManager().getMaterialCount()*16);
			memset(cell.histogramData.data(),0,sizeof(uint32_t)*cell.histogramData.size());
			for (auto& histoBuf:histogramBuffer)
			{
				histoBuf->map();
				uint32_t* data = (uint32_t*)histoBuf->getMappedMemory();
				for (int i = 0;i<StMaterialManager::getManager().getMaterialCount()*16;i++)
				{
					cell.histogramData[i] += data[i];
				}
				memset(histoBuf->getMappedMemory(),0,histoBuf->getInstanceSize());
				histoBuf->unmap();
			}
			// for (int i = 0;i<StMaterialManager::getManager().getMaterialCount();i++)
			// {
			//
			// 	std::string name = StMaterialManager::getManager().getMaterialName(i);
			// 	uint32_t* counts = &cell.histogramData[i*16];
			// 	spdlog::info("Material {} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X} {:X}",
			// 		name,
			// 		counts[0],counts[1],counts[2],counts[3],
			// 		counts[4],counts[5],counts[6],counts[7],
			// 		counts[8],counts[9],counts[10],counts[11],
			// 		counts[12],counts[13],counts[14],counts[15]);
			// }

			if(shouldClose)break;
		}
		vkDeviceWaitIdle(stDevice.device());
		if (shouldClose)return;
		for (auto& cell:cells)
		{
			if (cell.xIndex == 0 && cell.yIndex == 0)
			{
				struct PageData
				{
					std::string name;
					int bin;
					int cvg;
				};
				std::vector<PageData> pageData;
				for (int i = 0;i < cell.histogramData.size();i++)
				{
					PageData page;
					page.name = StMaterialManager::getManager().getMaterialName(i/16);
					page.bin = i%16;
					page.cvg = cell.histogramData[i];
					pageData.push_back(page);
				}
				std::sort(pageData.begin(),pageData.end(),[](const PageData& a,const PageData& b)
				{
					return a.cvg>b.cvg;
				});
				for (auto& page:pageData)
				{
					if (!strncmp(page.name.c_str(),"TOOLS",5))continue;
					if (!strncmp(page.name.c_str(),"WORLD\\DEV",9))continue;
					printf("%-110s %2d %d\n",page.name.c_str(),page.bin,page.cvg);
				}

			}
		}
	}

	

	void calculateCellPositions(Cell& cell) {
		size_t nodeCount = StSettingsManager::getManager().kmeansNodeCount;
		const std::vector<__m128>& input = cell.inputArray;
		if (input.size() == 0)return;
		if (input.size() <= nodeCount) {
			cell.outputArray = input;
			return;
		}

		std::vector<__m128> nodes;
		std::vector<size_t> closestPointNodeIndex;
		std::vector<size_t> pointsPerNode;
		std::vector<__m128> bestCaseNodes;
		closestPointNodeIndex.resize(input.size());
		nodes.reserve(nodeCount);
		pointsPerNode.resize(nodeCount);

		__m128 bestDistance = _mm_set1_ps(std::numeric_limits<float>::max());
		for (int iterations = 0; iterations < StSettingsManager::getManager().kmeansIterations; iterations++) {
			for (int i = 0; i < nodeCount; i++) {
				__m128 n;
				do {
					int index = rand()%input.size();
					n = input[index];
					bool dublicate = false;
					for (const auto& a : nodes) {
						if (_mm_movemask_ps(_mm_cmpeq_ps(a, n)) == 0xF) {
							dublicate = true;
							break;
						}
					}
					if(!dublicate)break;
				} while(true);
				nodes.push_back(n);
			}




			bool keepRunning = true;
			while (keepRunning) {
				keepRunning = false;
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
					if (closestPointNodeIndex[i] != closestIndex)keepRunning = false;
					closestPointNodeIndex[i] = closestIndex;

				}
				for (size_t i = 0; i < nodes.size(); i++) {
					nodes[i] = _mm_setzero_ps();
					pointsPerNode[i] = 0;
				}

				for (size_t i = 0; i < input.size(); i++) {
					nodes[closestPointNodeIndex[i]] = _mm_add_ps(nodes[closestPointNodeIndex[i]], input[i]);
					pointsPerNode[closestPointNodeIndex[i]]++;
				}
				for (size_t i = 0; i < nodes.size(); i++) {
					nodes[i] = _mm_div_ps(nodes[i], _mm_set1_ps(pointsPerNode[i]));
				}
			}
			__m128 averageDistance = _mm_setzero_ps();
			for (size_t i = 0; i < input.size(); i++) {
				averageDistance = _mm_add_ps(averageDistance,magnitude_ps(_mm_sub_ps(input[i],nodes[closestPointNodeIndex[i]])));
			}
			if (_mm_movemask_ps(_mm_cmplt_ps(averageDistance, bestDistance))) {
				bestCaseNodes = nodes;
			}
			nodes.clear();
		}
		cell.outputArray = bestCaseNodes;

	}

	void testPointCollisions(BspLoader& bspLoader,Cell& cell){
		std::vector<__m128> newInputArray;
		for (__m128 point: cell.inputArray) {
			if (!bspLoader.doesPointCollide(point)) {
				newInputArray.push_back(point);
			}
		}
		cell.inputArray = newInputArray;
	}

	void StApp::loadGameObjects(fs::path bspFilePath) {

		BspLoader loader{bspFilePath};

		
		cells = std::move(loader.cells);
		for (auto& mesh : loader.meshes) {

			auto cube = StGameObject::createGameObject();
			cube.model = std::make_shared<StModel>(stDevice, mesh);
			cube.transform3d.translation = { 0.f,0.f,0.0f };
			cube.transform3d.scale = { 1.0,1.0,1.0 };//{ 1.0,1.0,1.0 } { 0.1,0.1,0.1 }
			cube.transform3d.rotation = { 0.5f * glm::pi<float>(),0.f,0.f, };
			gameObjects.push_back(std::move(cube));

		}

		std::for_each(std::execution::par, cells.begin(), cells.end(), [&loader](Cell& cell) {
			testPointCollisions(loader,cell);
			calculateCellPositions(cell);
			if(cell.outputArray.size())
				spdlog::info("cell {} {} computed with {} data points", cell.xIndex, cell.yIndex,cell.inputArray.size());
		});

		size_t numCubeMaps = 0;
		for (const auto& cell : cells) {
			numCubeMaps += cell.outputArray.size();
		}
		spdlog::info("Computing {} cubemaps",numCubeMaps);
	}

	

	
	
}