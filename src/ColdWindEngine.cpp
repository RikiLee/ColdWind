#define VMA_IMPLEMENTATION
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

        VmaAllocatorCreateInfo vmaAllocatorCreateInfo{};
        vmaAllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        vmaAllocatorCreateInfo.physicalDevice = m_context.getPhysicalDevice();
        vmaAllocatorCreateInfo.device = m_context.getDevice().get();
        vmaAllocatorCreateInfo.instance = m_instance.getVKInstance().get();
        VkResult result = vmaCreateAllocator(&vmaAllocatorCreateInfo, &m_vmaAllocator);
        if (result == VK_SUCCESS) {
            spdlog::info("Succeed to create vma allocator!");
        }
        else {
            spdlog::error("Failed to create vma allocator, error code: {}", vk::to_string(vk::Result(result)));
            throw std::runtime_error("Failed to create vma allocator!");
        }
    }

    ColdWindEngine::~ColdWindEngine()
    {
        vmaDestroyAllocator(m_vmaAllocator);
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