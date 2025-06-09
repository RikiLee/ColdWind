#include "Window.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace coldwind {
	Window::Window(uint32_t width, uint32_t height, const std::string& title)
        : m_extent(width, height), m_title(title)
    { 
		initWindow();
    }

	Window::~Window() 
	{ 
		destroyWindow();
	}

	void Window::initWindow() 
	{
		if (glfwInit() != GLFW_TRUE) {
			spdlog::error("Failed to init GLFW!");
			throw std::runtime_error("Failed to init GLFW!");
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		m_window = glfwCreateWindow(m_extent.width, m_extent.height, m_title.c_str(), nullptr, nullptr);

		if (m_window == nullptr) {
			spdlog::error("Failed to create window!");
			throw std::runtime_error("Failed to create window!");
		}
		else {
			spdlog::debug("Succeed to create window!");
		}
	}

	void Window::destroyWindow()
	{
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		glfwTerminate();

		spdlog::debug("Succeed to destroy window!");
	}

}