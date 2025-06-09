#pragma once
#include <string>
#include <vulkan/vulkan.hpp>
#include <map>
#include <cstdint>

namespace coldwind 
{
	static const uint32_t ENGINE_VERION = VK_MAKE_VERSION(1, 0, 0);
	static const char* ENGINE_NAME = "ColdWind Engine";
	static const uint32_t USING_VK_API_VERSION = VK_API_VERSION_1_3;

	enum class RequirementType : uint8_t {
		Required = 2,
		Optional = 3
	};
	enum class SupportStatus : uint8_t {
		Supported = 0,
		Unsupported = 1
	};

	class Instance {
	public:
		Instance(const std::string& appName, uint8_t versionMajor = 1, uint8_t versionMinor = 0, uint8_t versionPatch = 0);
		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;
		~Instance() = default;

		[[nodiscard]] vk::UniqueInstance& getVKInstance() noexcept { return m_instance; }

#ifdef NDEBUG
		bool m_validationLayersEnabled = false;
#else
		bool m_validationLayersEnabled = true;
#endif

	private:
		void createInstance();
		void checkInstanceLayerSupport(std::map<const char*, uint8_t>&);
		void checkInstanceExtensionSupport(std::map<const char*, uint8_t>&);

	private:
		vk::UniqueInstance m_instance;
		vk::detail::DispatchLoaderDynamic m_dynamicLoader;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> m_debugMessenger;
		std::string m_AppName;
		uint32_t m_AppVersion;
	};
}