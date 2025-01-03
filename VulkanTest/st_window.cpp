#include "st_window.h"



namespace st {

	StWindow::StWindow(uint32_t w, uint32_t h, std::string name) :width{ w }, height{ h }, windowName{ name } {
		initWindow();
	}
	StWindow::~StWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void StWindow::initWindow() {
		
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE,GLFW_TRUE);
		window = glfwCreateWindow(width,height,windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window,this);
		glfwSetFramebufferSizeCallback(window,framebufferResizeCallback);

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr,&extensionCount,nullptr);
		spdlog::info("{} Extentions supported",extensionCount);
	}

	void StWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		VkResult res = glfwCreateWindowSurface(instance, window, nullptr, surface);
		if ( res != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}

	void StWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto stWindow = reinterpret_cast<StWindow*>(glfwGetWindowUserPointer(window));
		stWindow->width = width;
		stWindow->height = height;
		stWindow->framebufferResized = true;
	}

}
