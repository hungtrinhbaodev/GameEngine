#include <vulkan/vk_core.h>
#include <vulkan/vk_swapchain.h>
#include <vulkan/vk_utils.h>
#include <log.h>

namespace Vulkan {

	VkSurfaceFormatKHR _choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
		for (const VkSurfaceFormatKHR& available_format : available_formats) {
			if (available_format.format == VK_FORMAT_B8G8R8_SRGB &&
				available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return available_format;
			}
		}
		return available_formats[0];
	}

	VkPresentModeKHR _choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_presents) {
		for (const VkPresentModeKHR& available_present : available_presents) {
			if (available_present == VK_PRESENT_MODE_MAILBOX_KHR) {
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D _choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilites, GLFWwindow* window) {
		if (capabilites.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilites.currentExtent;
		}
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
		actual_extent.width =
			std::clamp(actual_extent.width, capabilites.currentExtent.width, capabilites.maxImageExtent.width);
		actual_extent.height =
			std::clamp(actual_extent.height, capabilites.currentExtent.height, capabilites.maxImageExtent.height);
		return actual_extent;
	}

	void _create_swapchain_image_views(const std::vector<VkImage>& swapchain_images, const VkFormat& format,
									   VkDevice vk_device) {
		swapchain_image_views.resize(swapchain_images.size());

		for (size_t i = 0; i < swapchain_images.size(); i++) {
			swapchain_image_views[i] =
				Utils::create_imageview_from_image(swapchain_images[i], format, VK_IMAGE_ASPECT_COLOR_BIT, device);
		}
	}

	namespace Init {

		void _init_swapchain() {
			Swapchain_Support_Detail swapchain_detail = Utils::query_swapchain_support_detail(physical_device, surface);
			Log::log_info("Swapchain detail info:");
			swapchain_detail.log_info();

			VkSurfaceFormatKHR format = _choose_swapchain_format(swapchain_detail.formats);
			VkPresentModeKHR present = _choose_swapchain_present_mode(swapchain_detail.present_modes);
			VkExtent2D extent = _choose_swapchain_extent(swapchain_detail.capabilities, _window);

			// log mode present is choosen
			switch (present) {
			case VK_PRESENT_MODE_MAILBOX_KHR: {
				Log::log_info("Swapchain choose mode present: VK_PRESENT_MODE_MAILBOX_KHR");
				break;
			}
			default: {
				Log::log_info("Swapchain choose mode present: VK_PRESENT_MODE_FIFO_KHR");
				break;
			}
			}

			uint32_t image_count = swapchain_detail.capabilities.minImageCount + 1;
			if (swapchain_detail.capabilities.maxImageCount > 0 &&
				image_count > swapchain_detail.capabilities.maxImageCount) {
				image_count = swapchain_detail.capabilities.maxImageCount;
			}

			Log::log_info("Vulkan_Swapchain::init", swapchain_detail.capabilities.maxImageCount,
						  swapchain_detail.capabilities.minImageCount);

			VkSwapchainCreateInfoKHR create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			create_info.surface = surface;

			create_info.minImageCount = image_count;
			create_info.imageFormat = format.format;
			create_info.imageColorSpace = format.colorSpace;
			create_info.imageExtent = extent;
			create_info.imageArrayLayers = 1;
			create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			Queue_Family_Indices indices = Utils::query_suitable_queue_family_indices(physical_device, surface);
			uint32_t queue_indices[] = {indices.graphic_family.value(), indices.present_family.value()};
			if (indices.graphic_family != indices.present_family) {
				create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				create_info.queueFamilyIndexCount = 2;
				create_info.pQueueFamilyIndices = queue_indices;
				Log::log_info("Swapchain choose image sharing mode: VK_SHARING_MODE_CONCURRENT");
			} else {
				create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				Log::log_info("Swapchain choose image sharing mode: VK_SHARING_MODE_EXCLUSIVE");
			}

			create_info.preTransform = swapchain_detail.capabilities.currentTransform;
			create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			create_info.presentMode = present;
			create_info.clipped = VK_TRUE;

			create_info.oldSwapchain = VK_NULL_HANDLE;

			Utils::vk_check_result(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain), "",
								   "failed to create swap chain!");
			Log::log_info("Create swap chain successfully!");

			vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
			swapchain_images.resize(image_count);
			vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());
			Log::log_info("swapchain image count", image_count);

			swapchain_format = format.format;
			swapchain_extent = extent;

			_create_swapchain_image_views(swapchain_images, swapchain_format, device);
		}

	} // namespace Init

	namespace Destroy {

		void _destroy_swapchain() {
			for (auto& image_view : swapchain_image_views) {
				vkDestroyImageView(device, image_view, nullptr);
			}
			Log::log_info("Destroy swap chain image view success!");

			vkDestroySwapchainKHR(device, swapchain, nullptr);
			Log::log_info("Destroy swap chain success!");
		}

	} // namespace Destroy

} // namespace Vulkan