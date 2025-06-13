#define VMA_IMPLEMENTATION
#include "VKContext.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <set>

namespace coldwind
{
	VKContext::VKContext(Instance& instance, Window& window)
	{
		init(instance, window);
	}

	VKContext::~VKContext()
	{
		vmaDestroyAllocator(m_vmaAllocator);
	}

	void VKContext::selectPhysicalDevice(Instance& instance, Window& window)
	{
		auto [result, physicalDeviceList] = instance.getVKInstance()->enumeratePhysicalDevices();
		if (result == vk::Result::eSuccess) {
			if (physicalDeviceList.empty()) {
				spdlog::error("Failed to find any available physical device!");
				throw std::runtime_error("Failed to find any available physical device!");
			}
		}
		else {
			spdlog::error("Failed to enumerate physical devices! Error code: {}", vk::to_string(result));
			throw std::runtime_error("Failed to enumerate physical devices!");
		}

		std::map<const char*, uint8_t> requiredDeviceExtensions;
		requiredDeviceExtensions.emplace(VK_KHR_SWAPCHAIN_EXTENSION_NAME, static_cast<uint8_t>(RequirementType::Required));

		std::vector<const char*> deviceExtensions;
		struct PhysicalDeviceAndQueueFamilyIndex {
			vk::PhysicalDevice physicalDevice;
			std::optional<uint32_t> graphicsQueue;
			std::optional<uint32_t> presentQueue;
			vk::PhysicalDeviceProperties  physicalDeviceProperties;
		};

		std::multimap<uint64_t, PhysicalDeviceAndQueueFamilyIndex, std::greater<uint64_t>> usableDevices;
		for (const auto& physicalDevice : physicalDeviceList) {
			auto physicalDeviceProperties = physicalDevice.getProperties();
			spdlog::debug("Found device {}: {}, made by vendor {}", 
				getDeviceTypeString(physicalDeviceProperties.deviceType), 
				physicalDeviceProperties.deviceName.data(),
				physicalDeviceProperties.vendorID
			);
			spdlog::debug("Device driver version: {}", physicalDeviceProperties.driverVersion);
			spdlog::debug("Supported newest API version: {}.{}.{}", 
				VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion), VK_VERSION_MINOR(physicalDeviceProperties.apiVersion), VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));
			size_t score = getDeviceScore(physicalDeviceProperties);

			/// check device extension support
			std::map<const char*, uint8_t> deviceExtensionsMap = requiredDeviceExtensions;
			checkDeviceExtensionSupport(physicalDevice, deviceExtensionsMap);
			deviceExtensions.clear();
			bool usable = true;
			for (const auto& [extension, stats] : deviceExtensionsMap) {
				if ((stats & static_cast<uint8_t>(SupportStatus::Supported)) != static_cast<uint8_t>(SupportStatus::Supported)) {
					if ((stats & static_cast<uint8_t>(RequirementType::Optional)) == static_cast<uint8_t>(RequirementType::Optional)) {
						spdlog::debug("\tOptional device extension {} not support!", extension);
					}
					else {
						spdlog::warn("\tRequired device extension {} not support!", extension);
						usable = false;
					}
				}
				else {
					deviceExtensions.push_back(extension);
				}
			}
			if (!usable) continue;

			auto& surface = window.getSurface();
			const auto formats = physicalDevice.getSurfaceFormatsKHR(surface.get());
			if (formats.result != vk::Result::eSuccess) {
				spdlog::warn("\tFailed to get surface formats! Error code: {}", vk::to_string(formats.result));
				continue;
			}
			if (formats.value.empty()) {
				spdlog::warn("\tSurface format is empty!");
				continue;
			}

			const auto presentMode = physicalDevice.getSurfacePresentModesKHR(surface.get());
			if (presentMode.result != vk::Result::eSuccess) {
				spdlog::warn("\tFailed to get surface present modes! Error code: {}", vk::to_string(presentMode.result));
				continue;
			} 
			if (presentMode.value.empty()) {
				spdlog::warn("\tSurface present mode is empty!");
				continue;
			}

			/// check device feature support
			auto physicalDeviceFeatures = physicalDevice.getFeatures();

			// required feature
			if (physicalDeviceFeatures.geometryShader != VK_TRUE) {
				spdlog::warn("\tDevice not support feature: geometryShader!");
				continue;
			}
			if (physicalDeviceFeatures.tessellationShader != VK_TRUE) {
				spdlog::warn("\tDevice not support feature: tessellationShader!");
				continue;
			}

			// optional feature
			if (physicalDeviceFeatures.samplerAnisotropy != VK_TRUE) {
				spdlog::debug("\tDevice not support feature: samplerAnisotropy!");
			}

			PhysicalDeviceAndQueueFamilyIndex physicalDeviceAndQueueFamily{};
			physicalDeviceAndQueueFamily.physicalDevice = physicalDevice;
			physicalDeviceAndQueueFamily.physicalDeviceProperties = physicalDeviceProperties;

			const auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
			for (size_t i = 0, queueFamilyCount = queueFamilyProperties.size(); i < queueFamilyCount; ++i) {
				if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
					physicalDeviceAndQueueFamily.graphicsQueue = i;
				}
				auto presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface.get());
				if (presentSupport.result == vk::Result::eSuccess && presentSupport.value == VK_TRUE) {
					physicalDeviceAndQueueFamily.presentQueue = i;
				}
				if (physicalDeviceAndQueueFamily.graphicsQueue.has_value() &&
					physicalDeviceAndQueueFamily.presentQueue.has_value()) {
					break;
				}
			}

			if (physicalDeviceAndQueueFamily.graphicsQueue.has_value() &&
				physicalDeviceAndQueueFamily.presentQueue.has_value()) {
				usableDevices.emplace(score, physicalDeviceAndQueueFamily);
			}
		}

		if (usableDevices.empty()) {
			spdlog::error("Failed to find any suitable physical device!");
			throw std::runtime_error("Failed to find any suitable physical device!");
		}

		m_physicalDevice = usableDevices.begin()->second.physicalDevice;
		m_graphicsAndComputeQueueFamilyIndex = usableDevices.begin()->second.graphicsQueue.value();
		m_presentQueueFamilyIndex = usableDevices.begin()->second.presentQueue.value();
		const auto& physicalDeviceProperties = usableDevices.begin()->second.physicalDeviceProperties;

		spdlog::info("Using device {}: {}, made by vendor {}",
			getDeviceTypeString(physicalDeviceProperties.deviceType),
			physicalDeviceProperties.deviceName.data(),
			physicalDeviceProperties.vendorID
		);
	}

	void VKContext::checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice, std::map<const char*, uint8_t>& requiredDeviceExtensions)
	{
		auto [result, physicalDeviceExtensionProperties] = physicalDevice.enumerateDeviceExtensionProperties();
		if (result == vk::Result::eSuccess) {
			for (const auto& supportExtension : physicalDeviceExtensionProperties) {
				auto iter = requiredDeviceExtensions.find(supportExtension.extensionName);
				if (iter != requiredDeviceExtensions.end()) {
					iter->second |= static_cast<uint8_t>(SupportStatus::Supported);
					spdlog::debug("Found supported extension {}!", supportExtension.extensionName.data());
				}
			}
		}
		else {
			spdlog::error("Failed to enumerate physical device extension properties! Error code: {}", vk::to_string(result));
			throw std::runtime_error("Failed to enumerate physical device extension properties!");
		}
	}

	uint64_t VKContext::getDeviceScore(const vk::PhysicalDeviceProperties& deviceProperties) const noexcept
	{
		uint64_t score = 0;
		if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 1600;
		else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 800;
		else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eVirtualGpu) score += 400;
		else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu) score += 200;
		else score += 100;

		score += deviceProperties.limits.maxComputeWorkGroupInvocations;
		score += deviceProperties.limits.maxImageDimension2D;
		score += deviceProperties.limits.maxBoundDescriptorSets;

		return score;
	}

	void VKContext::createDevice()
	{
		vk::DeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;

		float queuePriority = 1.0f;
		std::set<uint32_t> uniqueQueueFamilies = { m_graphicsAndComputeQueueFamilyIndex, m_presentQueueFamilyIndex };

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			vk::DeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

		std::vector<const char*> enableDeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enableDeviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = enableDeviceExtensions.data();

		vk::PhysicalDeviceFeatures enableDeviceFeatures;
		enableDeviceFeatures.geometryShader = VK_TRUE;
		enableDeviceFeatures.tessellationShader = VK_TRUE;
		enableDeviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceCreateInfo.pEnabledFeatures = &enableDeviceFeatures;

		auto [result, device] = m_physicalDevice.createDeviceUnique(deviceCreateInfo);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to create logical device! Error code: {}", vk::to_string(result));
			throw std::runtime_error("Failed to create logical device!");
		}
		else {
			m_device = std::move(device);
			spdlog::debug("Succeed to create device!");
		}

		m_graphicsAndComputeQueue = m_device->getQueue(m_graphicsAndComputeQueueFamilyIndex, 0);
		m_presentQueue = m_device->getQueue(m_presentQueueFamilyIndex, 0);
	}

	inline void VKContext::initVmaAllocator(Instance& instance)
	{
		VmaAllocatorCreateInfo vmaAllocatorCreateInfo{};
		vmaAllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		vmaAllocatorCreateInfo.physicalDevice = m_physicalDevice;
		vmaAllocatorCreateInfo.device = m_device.get();
		vmaAllocatorCreateInfo.instance = instance.getVKInstance().get();
		VkResult result = vmaCreateAllocator(&vmaAllocatorCreateInfo, &m_vmaAllocator);
		if (result == VK_SUCCESS) {
			spdlog::info("Succeed to create vma allocator!");
		}
		else {
			spdlog::error("Failed to create vma allocator, error code: {}", vk::to_string(vk::Result(result)));
			throw std::runtime_error("Failed to create vma allocator!");
		}
	}
}

