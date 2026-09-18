#include <vulkan/vk_core.h>
#include <vulkan/vk_frame_buffers.h>
#include <vulkan/vk_utils.h>
#include <log.h>

namespace Vulkan {

	namespace Init {

		void _init_frame_buffers() {

			frame_buffers.resize(swapchain_image_views.size());

			for (size_t i = 0; i < swapchain_image_views.size(); i++) {

				std::vector<VkImageView> attachments{swapchain_image_views[i], depth_image.view};

				VkFramebufferCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				create_info.renderPass = render_pass;
				create_info.attachmentCount = attachments.size();
				create_info.pAttachments = attachments.data();
				create_info.width = swapchain_extent.width;
				create_info.height = swapchain_extent.height;
				create_info.layers = 1;

				Utils::vk_check_result(vkCreateFramebuffer(device, &create_info, nullptr, &frame_buffers[i]), "",
									   "Vulkan failed to create framebuffer!");
			}

			Log::log_info("Vulkan create frame buffers success!");
		}

	} // namespace Init

	namespace Process {
		void _recreate_frame_buffers() {
			Destroy::_destroy_frame_buffers();
			Init::_init_frame_buffers();
		}
	} // namespace Process

	namespace Destroy {

		void _destroy_frame_buffers() {

			for (auto& frame_buffer : frame_buffers) {
				vkDestroyFramebuffer(device, frame_buffer, nullptr);
			}

			Log::log_info("Vulkan destroy swap chain frame buffers success!");
		}

	} // namespace Destroy

} // namespace Vulkan