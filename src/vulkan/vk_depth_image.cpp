#include <vulkan/vk_core.h>
#include <vulkan/vk_depth_image.h>
#include <vulkan/vk_utils.h>
#include <log.h>

namespace Vulkan {

	namespace Init {

		void _init_depth_image() {

			depth_image.make_image(
				swapchain_extent.width,
				swapchain_extent.height,
				Utils::find_depth_format(physical_device),
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);
			Log::log_info("Vulkan init depth image successfully!");

		}

	}

	namespace Destroy {


		void _destroy_depth_image() {
			
			depth_image.destroy();
			Log::log_info("Vulkan destroy depth image successfully!");

		}

	}


}