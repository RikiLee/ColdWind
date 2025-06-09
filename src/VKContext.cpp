#include "VKContext.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <glfw/glfw3.h>
#include <GLFW/glfw3native.h>
#include <set>

namespace coldwind
{
	VKContext::VKContext(Instance& instance, Window& window)
		: m_instanceRef(instance), m_windowRef(window)
	{
		init();
	}


	void VKContext::createSurface()
	{
#ifdef VK_USE_PLATFORM_WIN32_KHR
		HWND hWnd = glfwGetWin32Window(m_windowRef.getWindowPtr());
		HINSTANCE hInstance = GetModuleHandle(nullptr);

		vk::Win32SurfaceCreateInfoKHR createInfo{};
		createInfo.hwnd = hWnd;
		createInfo.hinstance = hInstance;

		auto returnValue = m_instanceRef.getVKInstance()->createWin32SurfaceKHRUnique(createInfo);
#else
#endif
		if (returnValue.result == vk::Result::eSuccess) {
			spdlog::debug("Succeed to create surface!");
			m_surface = std::move(returnValue.value);
		}
		else {
			spdlog::error("Failed to create surface!");
			throw std::runtime_error("Failed to create surface!");
		}
	}

	void VKContext::selectPhysicalDevice()
	{
		auto [result, physicalDeviceList] = m_instanceRef.getVKInstance()->enumeratePhysicalDevices();
		if (result == vk::Result::eSuccess) {
			if (physicalDeviceList.empty()) {
				spdlog::error("Failed to find any available physical device!");
				throw std::runtime_error("Failed to find any available physical device!");
			}
		}
		else {
			spdlog::error("Failed to enumerate physical devices!");
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

			const auto formats = physicalDevice.getSurfaceFormatsKHR(m_surface.get());
			if (formats.result != vk::Result::eSuccess) {
				spdlog::warn("\tFailed to get surface formats!");
				continue;
			}
			if (formats.value.empty()) {
				spdlog::warn("\tSurface format is empty!");
				continue;
			}

			const auto presentMode = physicalDevice.getSurfacePresentModesKHR(m_surface.get());
			if (presentMode.result != vk::Result::eSuccess) {
				spdlog::warn("\tFailed to get surface present modes!");
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
				auto presentSupport = physicalDevice.getSurfaceSupportKHR(i, m_surface.get());
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
			spdlog::error("Failed to enumerate physical device extension properties!");
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
			spdlog::error("Failed to create logical device!");
			throw std::runtime_error("Failed to create logical device!");
		}
		else {
			m_device = std::move(device);
			spdlog::debug("Succeed to create device!");
		}

		m_graphicsAndComputeQueue = m_device->getQueue(m_graphicsAndComputeQueueFamilyIndex, 0);
		m_presentQueue = m_device->getQueue(m_presentQueueFamilyIndex, 0);
	}

	void VKContext::createSwapchain()
	{
		auto surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
		if (surfaceCapabilities.result != vk::Result::eSuccess) {
			spdlog::error("Failed to get surface capabilities!");
			throw std::runtime_error("Failed to get surface capabilities!");
		}

		auto surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(m_surface.get());
		if (surfaceFormats.result != vk::Result::eSuccess) {
			spdlog::error("Failed to get surface formats!");
			throw std::runtime_error("Failed to get surface formats!");
		}

		auto presentModes = m_physicalDevice.getSurfacePresentModesKHR(m_surface.get());
		if (presentModes.result != vk::Result::eSuccess) {
			spdlog::error("Failed to get surface present modes!");
			throw std::runtime_error("Failed to get surface present modes!");
		}

		if (surfaceFormats.value.size() == 1 && surfaceFormats.value[0].format == vk::Format::eUndefined) {
			m_surfaceFormat = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
		} else {
			bool foundSuitableFormat = false;
			for (const auto& format : surfaceFormats.value) {
				if (format.format == vk::Format::eR8G8B8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
					m_surfaceFormat = format;
					foundSuitableFormat = true;
					break;
				}
			}
			if (!foundSuitableFormat) m_surfaceFormat = surfaceFormats.value[0];
		}
		spdlog::info("Selected surface format: {}, color space: {}", vk::to_string(m_surfaceFormat.format), vk::to_string(m_surfaceFormat.colorSpace));


		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
		for (const auto& mode : presentModes.value) {
			if (mode == vk::PresentModeKHR::eMailbox) {
				presentMode = mode;
				break;
			}
		}
		spdlog::info("Selected present mode: {}", vk::to_string(presentMode));

		if (surfaceCapabilities.value.currentExtent.width != UINT32_MAX) {
			m_swapChainExtent2D = surfaceCapabilities.value.currentExtent;
		} else {
			vk::Extent2D actualExtent = m_windowRef.getWindowExtent2D();
			actualExtent.width = std::max(surfaceCapabilities.value.minImageExtent.width,
				std::min(surfaceCapabilities.value.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(surfaceCapabilities.value.minImageExtent.height,
				std::min(surfaceCapabilities.value.maxImageExtent.height, actualExtent.height));
			m_swapChainExtent2D = actualExtent;
		}

		uint32_t imageCount = surfaceCapabilities.value.minImageCount + 1;
		if (surfaceCapabilities.value.maxImageCount > 0 && imageCount > surfaceCapabilities.value.maxImageCount) {
			imageCount = surfaceCapabilities.value.maxImageCount;
		}

		vk::ImageUsageFlags imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		uint32_t queueFamilyIndices[] = { m_graphicsAndComputeQueueFamilyIndex, m_presentQueueFamilyIndex };

		vk::SwapchainCreateInfoKHR swapchainCreateInfo;
		swapchainCreateInfo.surface = m_surface.get();
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = m_surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = m_swapChainExtent2D;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = imageUsage;
		if (m_graphicsAndComputeQueueFamilyIndex != m_presentQueueFamilyIndex) {
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
			swapchainCreateInfo.queueFamilyIndexCount = 0;
			swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}
		swapchainCreateInfo.preTransform = surfaceCapabilities.value.currentTransform;
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = nullptr;
		
		auto returnValue = m_device->createSwapchainKHRUnique(swapchainCreateInfo);

		if (returnValue.result == vk::Result::eSuccess) {
			spdlog::debug("Succeed to create swapchain!");
			m_swapChain = std::move(returnValue.value);
		} else {
			spdlog::error("Failed to create swapchain!");
			throw std::runtime_error("Failed to create swapchain!");
		}

		auto swapchainImages = m_device->getSwapchainImagesKHR(m_swapChain.get());
		if (swapchainImages.result != vk::Result::eSuccess) {
			spdlog::error("Failed to get swapchain images!");
			throw std::runtime_error("Failed to get swapchain images!");
		}
		else {
			spdlog::debug("Succeed to get swapchain images!");

			vk::ImageViewCreateInfo viewInfo{};
			viewInfo.viewType = vk::ImageViewType::e2D;
			viewInfo.format = m_surfaceFormat.format;
			viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			size_t swapchainImageSize = swapchainImages.value.size();
			spdlog::info("Swapchain image count: {}", swapchainImageSize);
			m_swapChainImages.resize(swapchainImageSize);
			m_swapChainImageViews.resize(swapchainImageSize);
			for (size_t i = 0; i < swapchainImageSize; ++i) {
				m_swapChainImages[i] = swapchainImages.value[i];
				viewInfo.image = m_swapChainImages[i];
				auto imageViewCreateResult = m_device->createImageViewUnique(viewInfo);
				if (imageViewCreateResult.result != vk::Result::eSuccess) {
					spdlog::error("Failed to create image view!");
					throw std::runtime_error("Failed to create image view!");
				}
				else {
					m_swapChainImageViews[i] = std::move(imageViewCreateResult.value);
					spdlog::debug("Succeed to create swapchain image view!");
				}
			}
		}
	}
}

