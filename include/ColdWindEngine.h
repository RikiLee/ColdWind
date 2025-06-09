#pragma once
#include "VKContext.h"

namespace coldwind
{
	class ColdWindEngine
	{
	public:
		explicit ColdWindEngine(const std::string& appName, uint32_t width, uint32_t height);
		ColdWindEngine(const ColdWindEngine&) = delete;
		ColdWindEngine& operator=(const ColdWindEngine&) = delete;
		~ColdWindEngine() = default;

		inline void run() { mainLoop(); }
	private:
		Instance m_instance;
		Window m_window;
		VKContext m_context;

		void mainLoop();
	};
}
