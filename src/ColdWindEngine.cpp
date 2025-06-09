#include "ColdWindEngine.h"
#include <spdlog/spdlog.h>

namespace coldwind
{
    ColdWindEngine::ColdWindEngine(const std::string& appName, uint32_t width, uint32_t height)
        : m_instance(appName), m_window(width, height, appName), m_context(m_instance, m_window)
    {
        spdlog::info("Cold wind initialized");
    }

    void ColdWindEngine::mainLoop()
    {
        while (!m_window.shouldClose()) {
            m_window.pollEvents();
        }
    }
}