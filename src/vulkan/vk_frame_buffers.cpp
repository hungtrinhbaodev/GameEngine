#include <log.h>
#include <vulkan/vk_frame_buffers.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	namespace Init {

		void _init_frame_buffers(
			std::vector<VkFramebuffer>& frame_buffers, VkRenderPass render_pass, VkDevice device,
			const std::vector<VkImageView>& swapchain_image_views, Image depth_image, VkExtent2D swapchain_extent
		) {

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

				Utils::vk_check_result(
					vkCreateFramebuffer(device, &create_info, nullptr, &frame_buffers[i]), "",
					"Vulkan failed to create framebuffer!"
				);
			}

			Log::info("Vulkan create frame buffers success!");
		}

	} // namespace Init

	namespace Process {
		void _recreate_frame_buffers(
			std::vector<VkFramebuffer>& frame_buffers, VkRenderPass render_pass, VkDevice device,
			const std::vector<VkImageView>& swapchain_image_views, Image depth_image, VkExtent2D swapchain_extent
		) {
			Destroy::_destroy_frame_buffers(frame_buffers, device);
			Init::_init_frame_buffers(
				frame_buffers, render_pass, device, swapchain_image_views, depth_image, swapchain_extent
			);
		}
	} // namespace Process

	namespace Destroy {

		void _destroy_frame_buffers(const std::vector<VkFramebuffer>& frame_buffers, VkDevice device) {

			for (auto& frame_buffer : frame_buffers) {
				vkDestroyFramebuffer(device, frame_buffer, nullptr);
			}

			Log::info("Vulkan destroy swap chain frame buffers success!");
		}

	} // namespace Destroy

} // namespace Vulkan