#pragma once
#include <vulkan/vulkan.hpp>

#include <vector>
#include <map>

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

		[[nodiscard]] vk::PhysicalDevice getPhysicalDevice() const noexcept { return m_physicalDevice; }
		[[nodiscard]] const vk::UniqueDevice& getDevice() noexcept { return m_device; }

		[[nodiscard]] uint32_t getGraphicQueueFamilyIndex() const noexcept { return m_graphicsAndComputeQueueFamilyIndex; }
		[[nodiscard]] uint32_t getPresentQueueFamilyIndex() const noexcept { return m_presentQueueFamilyIndex; }

	private:
		void init(Instance& instance, Window& window)
		{
			selectPhysicalDevice(instance, window);
			createDevice();
		}

	private:

		vk::PhysicalDevice m_physicalDevice;
		uint32_t m_graphicsAndComputeQueueFamilyIndex = 0;
		uint32_t m_presentQueueFamilyIndex = 0;
		void selectPhysicalDevice(Instance& instance, Window& window);
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
	};
}