#pragma once
#include <vulkan/vulkan.hpp>

#include <vector>
#include <map>

#include "Instance.h"
#include "Window.h"

namespace coldwind
{
	class VKContext
	{
	public:
		VKContext(Instance& instance, Window& window);
		VKContext(const VKContext&) = delete;
		VKContext& operator=(const VKContext&) = delete;
		~VKContext() = default;

		vk::PhysicalDevice getPhysicalDevice() const noexcept { return m_physicalDevice; }
		const vk::UniqueDevice& getDevice() noexcept { return m_device; }

	private:
		inline void init() 
		{
			createSurface();
			selectPhysicalDevice();
			createDevice();
			createSwapchain();
		}

	private:
		Instance& m_instanceRef;
		Window& m_windowRef;

		vk::UniqueSurfaceKHR m_surface;
		void createSurface();

		vk::PhysicalDevice m_physicalDevice;
		uint32_t m_graphicsAndComputeQueueFamilyIndex = 0;
		uint32_t m_presentQueueFamilyIndex = 0;
		void selectPhysicalDevice();
		void checkDeviceExtensionSupport(vk::PhysicalDevice, std::map<const char*, uint8_t>&);
		const char* getDeviceTypeString(vk::PhysicalDeviceType deviceType) const noexcept
		{
			if (deviceType == vk::PhysicalDeviceType::eDiscreteGpu) return "Discrete GPU";
			if (deviceType == vk::PhysicalDeviceType::eIntegratedGpu) return "Integrate GPU";
			if (deviceType == vk::PhysicalDeviceType::eVirtualGpu) return "Virtual GPU";
			if (deviceType == vk::PhysicalDeviceType::eCpu) return "CPU";
			return "unknow device type";
		}
		uint64_t getDeviceScore(const vk::PhysicalDeviceProperties& deviceProperties) const noexcept;

		vk::UniqueDevice m_device;
		vk::Queue m_graphicsAndComputeQueue;
		vk::Queue m_presentQueue;
		void createDevice();

		vk::Extent2D m_swapChainExtent2D;
		vk::SurfaceFormatKHR m_surfaceFormat;
		vk::UniqueSwapchainKHR m_swapChain;
		std::vector<vk::Image> m_swapChainImages;
		std::vector<vk::UniqueImageView> m_swapChainImageViews;
		void createSwapchain();
	};
}