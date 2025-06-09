#include "ColdWindEngine.h"
#include <spdlog/spdlog.h>

namespace coldwind
{
    ColdWindEngine::ColdWindEngine(const std::string& appName, uint32_t width, uint32_t height)
        : m_instance(appName), m_window(m_instance, width, height, appName), 
        m_context(m_instance, m_window), m_swapChain(m_context, m_window)
    {
        spdlog::info("Cold wind initialized");
    }

    void ColdWindEngine::mainLoop()
    {
        while (!m_window.shouldClose()) {
            if (m_window.isMinimized()) continue;

            m_window.pollEvents();
        }
    }
}