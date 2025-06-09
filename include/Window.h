#pragma once
#include <vulkan/vulkan.hpp>
#include <string>
#include <cstdint>

#include <glfw/glfw3.h>

namespace coldwind {
	class Window {
	public:
		explicit Window(uint32_t width, uint32_t height, const std::string& title);
		~Window();

        // delete copy constructor and assignment operator
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		// can be implemented but not needed for now
		Window(Window&&) noexcept = delete;
		Window& operator=(Window&&) noexcept = delete; 

		bool shouldClose() const { return glfwWindowShouldClose(m_window); }
		void pollEvents() const { glfwPollEvents(); }

		inline GLFWwindow* getWindowPtr() const noexcept { return m_window; }

		vk::Extent2D getWindowExtent2D() const noexcept { return m_extent; }

	private:
		void initWindow();

		void destroyWindow();

		vk::Extent2D m_extent;
		std::string m_title;
		GLFWwindow* m_window = nullptr;

	};
}