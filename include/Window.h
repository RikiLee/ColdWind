#pragma once
#include "Instance.h"

#include <glfw/glfw3.h>

namespace coldwind {
	class Window {
	public:
		explicit Window(Instance& instance, uint32_t width, uint32_t height, const std::string& title);
		~Window();

        // delete copy constructor and assignment operator
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		// can be implemented but not needed for now
		Window(Window&&) noexcept = delete;
		Window& operator=(Window&&) noexcept = delete; 

		[[nodiscard]] bool shouldClose() const { return glfwWindowShouldClose(m_window); }
		void pollEvents() const { glfwPollEvents(); }

		[[nodiscard]] inline GLFWwindow* getWindowPtr() const noexcept { return m_window; }
		[[nodiscard]] vk::UniqueSurfaceKHR& getSurface() noexcept { return m_surface; }

		[[nodiscard]] vk::Extent2D getWindowExtent2D() const noexcept;
		[[nodiscard]] bool isMinimized() const noexcept;

	private:
		std::string m_title;
		GLFWwindow* m_window = nullptr;

		void initWindow(uint32_t width, uint32_t height);
		void destroyWindow();

		vk::UniqueSurfaceKHR m_surface;
		void createSurface(vk::UniqueInstance& instance);

	};
}