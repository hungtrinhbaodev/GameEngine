#include <log.h>
#include <vulkan/vk_depth_image.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	namespace Init {
		void _init_depth_image(
			Image& depth_image, VkPhysicalDevice physical_device, VkDevice device, VkExtent2D swapchain_extent
		) {
			depth_image.make_image(
				swapchain_extent.width, swapchain_extent.height, Utils::find_depth_format(physical_device),
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, physical_device, device
			);
			Log::info("Vulkan init depth image successfully!");
		}

	} // namespace Init

	namespace Process {
		void _recreate_depth_image(
			Image& depth_image, VkPhysicalDevice physical_device, VkDevice device, VkExtent2D swapchain_extent
		) {
			Destroy::_destroy_depth_image(depth_image);
			Init::_init_depth_image(depth_image, physical_device, device, swapchain_extent);
		}
	} // namespace Process

	namespace Destroy {
		void _destroy_depth_image(Image depth_image) {
			depth_image.destroy();
			Log::info("Vulkan destroy depth image successfully!");
		}
	} // namespace Destroy

} // namespace Vulkan