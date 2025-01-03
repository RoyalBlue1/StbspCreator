#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "spdlog/spdlog.h"
#include <string>


namespace st{
	class StWindow {
	public:
		StWindow(uint32_t w,uint32_t h,std::string name);
		~StWindow();

		StWindow(const StWindow &) = delete;
		StWindow &operator=(const StWindow &) = delete;

		bool shouldClose() const { return glfwWindowShouldClose(window); }
		VkExtent2D getExtent() const { return {static_cast<uint32_t>(width),static_cast<uint32_t>(height)}; }
		bool wasWindowResized() const {return framebufferResized;}
		void resetWindowResizedFlag() { framebufferResized = false; }
		GLFWwindow *getGLFWwindow() const { return window; }

		void createWindowSurface(VkInstance instance,VkSurfaceKHR* surface);
	private:


		GLFWwindow* window;

		uint32_t width;
		uint32_t height;
		bool framebufferResized = false;
		std::string windowName;
		void initWindow();

		static void framebufferResizeCallback(GLFWwindow* window,int width,int height);
	};
}