#include "ColdWindEngine.h"
#include <spdlog/spdlog.h>


namespace coldwind
{
    ColdWindEngine::ColdWindEngine(const std::string& appName, uint32_t width, uint32_t height)
        : m_instance(appName), m_window(m_instance, width, height, appName), 
        m_context(m_instance, m_window), m_swapChain(m_context, m_window)
    {
        spdlog::info("Engine coldwind initialized");

        glfwSetWindowUserPointer(m_window.getWindowPtr(), this);
        glfwSetWindowSizeCallback(m_window.getWindowPtr(), windowResizeCallback);
    }

    ColdWindEngine::~ColdWindEngine()
    {
    }

    void ColdWindEngine::mainLoop()
    {
        while (!m_window.shouldClose()) {
            m_window.pollEvents();
            if (m_window.isMinimized()) continue;
        }
    }

    void ColdWindEngine::onWindowResize()
    {
        m_swapChain.createSwapchain(m_context, m_window);
    }

    void ColdWindEngine::windowResizeCallback(GLFWwindow* window, int width, int height)
    {
        if (width != 0 && height != 0) {
            spdlog::debug("Window resized to : {}x{}", width, height);
            ColdWindEngine* app = static_cast<ColdWindEngine*>(glfwGetWindowUserPointer(window));
            app->onWindowResize();
        }
        else {
            spdlog::debug("Window minimized");
        }
    }
}