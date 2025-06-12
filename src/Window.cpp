#include "Window.h"

#include <spdlog/spdlog.h>
#include <stdexcept>
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

namespace coldwind {
	Window::Window(Instance& instance, uint32_t width, uint32_t height, const std::string& title)
        : m_title(title)
    { 
		initWindow(width, height);
		createSurface(instance.getVKInstance());
    }

	Window::~Window() 
	{ 
		destroyWindow();
	}

	vk::Extent2D Window::getWindowExtent2D() const noexcept
	{
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		return vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}

	bool Window::isMinimized() const noexcept
	{
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		return width == 0 || height == 0;
	}

	void Window::initWindow(uint32_t width, uint32_t height)
	{
		if (glfwInit() != GLFW_TRUE) {
			const char* errorDescription;
			glfwGetError(&errorDescription);
			spdlog::error("Failed to init GLFW! Error: {}", errorDescription ? errorDescription : "Unknown error");
			throw std::runtime_error("Failed to init GLFW!");
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		m_window = glfwCreateWindow(width, height, m_title.c_str(), nullptr, nullptr);

		if (m_window == nullptr) {
			const char* errorDescription;
			glfwGetError(&errorDescription);
			spdlog::error("Failed to create window! Error: {}", errorDescription ? errorDescription : "Unknown error");
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

	void Window::createSurface(vk::UniqueInstance& instance)
	{
#ifdef VK_USE_PLATFORM_WIN32_KHR
		HWND hWnd = glfwGetWin32Window(m_window);
		HINSTANCE hInstance = GetModuleHandle(nullptr);

		vk::Win32SurfaceCreateInfoKHR createInfo{};
		createInfo.hwnd = hWnd;
		createInfo.hinstance = hInstance;

		auto returnValue = instance->createWin32SurfaceKHRUnique(createInfo);
#else
#endif
		if (returnValue.result != vk::Result::eSuccess) {
			spdlog::error("Failed to create surface! Error code: {}", vk::to_string(returnValue.result));
			throw std::runtime_error("Failed to create surface!");
		}
		else {
			spdlog::debug("Succeed to create surface!");
			m_surface = std::move(returnValue.value);
		}
	}

}