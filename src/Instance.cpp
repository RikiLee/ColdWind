#include "Instance.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <stdexcept>
#include <vector>

namespace coldwind 
{
	Instance::Instance(const std::string& appName, uint8_t versionMajor, uint8_t versionMinor, uint8_t versionPatch)
		: m_AppName(appName), m_AppVersion(VK_MAKE_VERSION(versionMajor, versionMinor, versionPatch))
	{
		spdlog::set_default_logger(spdlog::stdout_color_mt("console", spdlog::color_mode::automatic));
#ifdef NDEBUG
		spdlog::set_level(spdlog::level::info);
#else
		spdlog::set_level(spdlog::level::debug);
#endif
		spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");

		createInstance();
	}

	void Instance::createInstance()
	{
		vk::ApplicationInfo appInfo = {
			m_AppName.c_str(), ENGINE_VERION, ENGINE_NAME, m_AppVersion, USING_VK_API_VERSION
		};

		auto instanceVersionResult = vk::enumerateInstanceVersion();
		if (instanceVersionResult.result != vk::Result::eSuccess) {
			spdlog::error("Failed to enumerateInstanceVersion! Error code: {}", vk::to_string(instanceVersionResult.result));
			throw std::runtime_error("Failed to enumerateInstanceVersion");
		}
		else if (instanceVersionResult.value < USING_VK_API_VERSION) {
			spdlog::error("Vulkan instance version too old, Clod Wind Engine require Vulkan version greater or equal to {}.{}.{}!",
				VK_VERSION_MAJOR(USING_VK_API_VERSION), VK_VERSION_MINOR(USING_VK_API_VERSION), VK_VERSION_PATCH(USING_VK_API_VERSION));
			throw std::runtime_error("Vulkan instance version too old");
		}
		else {
			spdlog::info("Vulkan instance version: {}.{}.{}",
				VK_VERSION_MAJOR(instanceVersionResult.value), VK_VERSION_MINOR(instanceVersionResult.value), VK_VERSION_PATCH(instanceVersionResult.value));
		}

		std::map<const char*, uint8_t> requiredLayers;
		if (m_validationLayersEnabled) {
			requiredLayers["VK_LAYER_KHRONOS_validation"] = static_cast<uint8_t>(RequirementType::Optional);
		}
		checkInstanceLayerSupport(requiredLayers);
		std::vector<const char*> instanceLayers;
		for (const auto& [layer, stats] : requiredLayers) {
			if ((stats & static_cast<uint8_t>(SupportStatus::Supported)) != static_cast<uint8_t>(SupportStatus::Supported)) {
				if ((stats & static_cast<uint8_t>(RequirementType::Optional)) == static_cast<uint8_t>(RequirementType::Optional)) {
					spdlog::warn("Optional instance layer {} not support!", layer);
				}
				else {
					spdlog::error("Required instance layer {} not support!", layer);
					throw std::runtime_error("Required instance layer not support!");
				}
				if (strcmp(layer, "VK_LAYER_KHRONOS_validation") == 0) {
					m_validationLayersEnabled = false;
				}
			}
			else {
				instanceLayers.push_back(layer);
			}
		}

		std::map<const char*, uint8_t> requiredInstanceExtensions;
		requiredInstanceExtensions.emplace(VK_KHR_SURFACE_EXTENSION_NAME, static_cast<uint8_t>(RequirementType::Required));
#ifdef VK_USE_PLATFORM_WIN32_KHR
		requiredInstanceExtensions.emplace(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, static_cast<uint8_t>(RequirementType::Required));
#else
#endif
		if (m_validationLayersEnabled) {
			requiredInstanceExtensions.emplace(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, static_cast<uint8_t>(RequirementType::Optional));
		}
		checkInstanceExtensionSupport(requiredInstanceExtensions);
		std::vector<const char*> instanceExtensions;
		for (const auto& [extension, stats] : requiredInstanceExtensions) {
			if ((stats & static_cast<uint8_t>(SupportStatus::Supported)) != static_cast<uint8_t>(SupportStatus::Supported)) {
				if ((stats & static_cast<uint8_t>(RequirementType::Optional)) == static_cast<uint8_t>(RequirementType::Optional)) {
					spdlog::warn("Optional instance extension {} not support!", extension);
				}
				else {
					spdlog::error("Required instance extension {} not support!", extension);
					throw std::runtime_error("Required instance extension not support!");
				}
				if (strcmp(extension, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
					m_validationLayersEnabled = false;
				}
			}
			else {
				instanceExtensions.push_back(extension);
			}
		}

		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.messageSeverity = 
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | 
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		debugCreateInfo.messageType = 
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | 
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		debugCreateInfo.pfnUserCallback = [](
			vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT messageType,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) -> vk::Bool32 
		{
			if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
				spdlog::error("validation layer: {}", pCallbackData->pMessage);
			else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
				spdlog::warn("validation layer: {}", pCallbackData->pMessage);
			else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
				spdlog::debug("validation layer: {}", pCallbackData->pMessage);
			else
				spdlog::trace("validation layer: {}", pCallbackData->pMessage);
			return vk::Bool32(VK_FALSE);
		};

		vk::InstanceCreateInfo instanceCreateInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			static_cast<uint32_t>(instanceLayers.size()),
			instanceLayers.data(),
			static_cast<uint32_t>(instanceExtensions.size()),
			instanceExtensions.data(),
			nullptr
		);
		if (m_validationLayersEnabled) instanceCreateInfo.pNext = &debugCreateInfo;

		auto instanceCreateResult = vk::createInstanceUnique(instanceCreateInfo);
		if (instanceCreateResult.result == vk::Result::eSuccess) {
			spdlog::debug("Succeed to create Vulkan instance!");
			m_instance = std::move(instanceCreateResult.value);
		}
		else {
			spdlog::error("Failed to create Vulkan instance!");
			throw std::runtime_error("Failed to create Vulkan instance!");
		}

		m_dynamicLoader = vk::detail::DispatchLoaderDynamic(m_instance.get(), vkGetInstanceProcAddr);

		if (m_validationLayersEnabled) {
			auto debugMessengerResult = m_instance->createDebugUtilsMessengerEXTUnique(debugCreateInfo, nullptr, m_dynamicLoader);
			if (debugMessengerResult.result != vk::Result::eSuccess) {
				spdlog::warn("Failed tp create debug utils messenger!");
			}
			else {
				m_debugMessenger = std::move(debugMessengerResult.value);
			}
		}
	}

	void Instance::checkInstanceLayerSupport(std::map<const char*, uint8_t>& requiredLayers)
	{
		auto instanceLayer = vk::enumerateInstanceLayerProperties();
		if (instanceLayer.result == vk::Result::eSuccess) {
			for (const auto& supportLayer : instanceLayer.value) {
				auto iter = requiredLayers.find(supportLayer.layerName);
				if (iter != requiredLayers.end()) {
					iter->second |= static_cast<uint8_t>(SupportStatus::Supported);
					spdlog::debug("Found supported layer {}!", supportLayer.layerName.data());
				}
			}
		}
		else {
			spdlog::error("Failed to enumerate instance layer properties! Error code: {}", vk::to_string(instanceLayer.result));
			throw std::runtime_error("Failed to enumerate instance layer properties!");
		}
	}

	void Instance::checkInstanceExtensionSupport(std::map<const char*, uint8_t>& requiredExtensions)
	{
		auto instanceExtension = vk::enumerateInstanceExtensionProperties();
		if (instanceExtension.result == vk::Result::eSuccess) {
			for (const auto& supportExtension : instanceExtension.value) {
				auto iter = requiredExtensions.find(supportExtension.extensionName);
				if (iter != requiredExtensions.end()) {
					iter->second |= static_cast<uint8_t>(SupportStatus::Supported);
					spdlog::debug("Found supported extension {}!", supportExtension.extensionName.data());
				}
			}
		}
		else {
			spdlog::error("Failed to enumerate instance extension properties! Error code: {}", vk::to_string(instanceExtension.result));
			throw std::runtime_error("Failed to enumerate instance extension properties!");
		}
	}
}
