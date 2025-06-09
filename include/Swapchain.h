#pragma once
#include "VKContext.h"

namespace coldwind {
	class SwapChain {
	public:
		explicit SwapChain(VKContext& context, Window& window);
		SwapChain(const SwapChain&) = delete;
		SwapChain& operator=(const SwapChain&) = delete;
		~SwapChain() = default;

		void createSwapchain(VKContext& context, Window& window);

		[[nodiscard]] vk::Extent2D getSwapchainExtent2D() const noexcept { return m_swapChainExtent2D; }
		[[nodiscard]] vk::SurfaceFormatKHR getSurfaceFormat() const noexcept { return m_surfaceFormat; }
		[[nodiscard]] vk::PresentModeKHR getPresentMode() const noexcept { return m_presentMode; }
		[[nodiscard]] auto& getSwapchainImageList() noexcept { return m_swapChainImages; }
		[[nodiscard]] auto& getSwapchainImageViewList() noexcept { return m_swapChainImageViews; }

	private:
		vk::Extent2D m_swapChainExtent2D;
		vk::SurfaceFormatKHR m_surfaceFormat;
		vk::UniqueSwapchainKHR m_swapChain;
		std::vector<vk::Image> m_swapChainImages;
		std::vector<vk::UniqueImageView> m_swapChainImageViews;
		vk::PresentModeKHR m_presentMode;
	};
}