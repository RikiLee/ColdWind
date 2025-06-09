#include "Swapchain.h"
#include <spdlog/spdlog.h>

namespace coldwind {
	SwapChain::SwapChain(VKContext& context, Window& window)
	{
		createSwapchain(context, window);
	}

	void SwapChain::createSwapchain(VKContext& context, Window& window)
	{
		if (window.getWindowExtent2D() == m_swapChainExtent2D) return;

		vk::PhysicalDevice physicalDevice = context.getPhysicalDevice();
		auto& surface = window.getSurface();
		if (m_swapChainExtent2D == vk::Extent2D()) {
			auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface.get());
			if (surfaceFormats.result != vk::Result::eSuccess) {
				spdlog::error("Failed to get surface formats!");
				throw std::runtime_error("Failed to get surface formats!");
			}

			if (surfaceFormats.value.size() == 1 && surfaceFormats.value[0].format == vk::Format::eUndefined) {
				m_surfaceFormat = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
			}
			else {
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

			auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());
			if (presentModes.result != vk::Result::eSuccess) {
				spdlog::error("Failed to get surface present modes!");
				throw std::runtime_error("Failed to get surface present modes!");
			}

			m_presentMode = vk::PresentModeKHR::eFifo;
			for (const auto& mode : presentModes.value) {
				if (mode == vk::PresentModeKHR::eMailbox) {
					m_presentMode = mode;
					break;
				}
			}
			spdlog::info("Selected present mode: {}", vk::to_string(m_presentMode));
		}

		auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
		if (surfaceCapabilities.result != vk::Result::eSuccess) {
			spdlog::error("Failed to get surface capabilities!");
			throw std::runtime_error("Failed to get surface capabilities!");
		}
		if (surfaceCapabilities.value.currentExtent.width != UINT32_MAX) {
			m_swapChainExtent2D = surfaceCapabilities.value.currentExtent;
		}
		else {
			vk::Extent2D actualExtent = window.getWindowExtent2D();
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
		uint32_t queueFamilyIndices[] = { context.getGraphicQueueFamilyIndex(), context.getPresentQueueFamilyIndex() };

		vk::SwapchainCreateInfoKHR swapchainCreateInfo;
		swapchainCreateInfo.surface = surface.get();
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = m_surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = m_swapChainExtent2D;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = imageUsage;
		if (context.getGraphicQueueFamilyIndex() != context.getPresentQueueFamilyIndex()) {
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
		swapchainCreateInfo.presentMode = m_presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = nullptr;

		auto& device = context.getDevice();
		auto returnValue = device->createSwapchainKHRUnique(swapchainCreateInfo);

		if (returnValue.result == vk::Result::eSuccess) {
			spdlog::debug("Succeed to create swapchain!");
			m_swapChain = std::move(returnValue.value);
		}
		else {
			spdlog::error("Failed to create swapchain!");
			throw std::runtime_error("Failed to create swapchain!");
		}

		auto swapchainImages = device->getSwapchainImagesKHR(m_swapChain.get());
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
				auto imageViewCreateResult = device->createImageViewUnique(viewInfo);
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
