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
	private:
		vk::Extent2D m_swapChainExtent2D;
		vk::SurfaceFormatKHR m_surfaceFormat;
		vk::UniqueSwapchainKHR m_swapChain;
		std::vector<vk::Image> m_swapChainImages;
		std::vector<vk::UniqueImageView> m_swapChainImageViews;
	};
}