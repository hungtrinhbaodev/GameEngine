#include <algorithm>
#include <map>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw_2D.h>
#include <vulkan/vk_ssbo_buffer.h>

namespace Vulkan {

	namespace Draw_2D {

		std::map<uint32_t, Draw_2D_Information> draws;

		std::vector<uint32_t> sorted_draws;

		Buffer instance_buffer{};

		SSBO_Buffer ssbo_buffer{};

		void sort_draws() {
			sorted_draws.clear();
			for (const auto& [draw_id, draw_information] : draws) {
				if (!draw_information.visible) {
					continue;
				}
				sorted_draws.push_back(draw_id);
			}
			std::stable_sort(sorted_draws.begin(), sorted_draws.end(), [](uint32_t a, uint32_t b) {
				return draws[a].draw_index < draws[b].draw_index;
			});
		}

		void setup_instance_buffer() {
			uint32_t size_reqiure = 0;
			for (int i = 0; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				Draw_2D_Information draw_info = draws[draw_id];
				SSBO_Buffer_Range range = ssbo_buffer.view_slot(draw_info.instance_id);
				size_reqiure += range.size;
			}
			if (instance_buffer.size < size_reqiure) {
				uint32_t size = (uint32_t)(size_reqiure * 1.5f);
				instance_buffer.resize(size);
			}
			uint32_t offset = 0;
			for (int i = 0; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				Draw_2D_Information draw_info = draws[draw_id];
				SSBO_Buffer_Range range = ssbo_buffer.view_slot(draw_info.instance_id);
				ssbo_buffer.transfer_data_to(draw_info.instance_id, instance_buffer.buffer, offset);
				offset += range.size;
			}
		}

		void init() {
			ssbo_buffer.init(Const::INITIALIZE_SIZE_STAGING_BUFFER);
			instance_buffer.make_buffer(
				Const::INITIALIZE_SIZE_INSTANCING_BUFFER, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
		}

		void draw_2D() {
			sort_draws();
			setup_instance_buffer();
		}

	} // namespace Draw_2D

} // namespace Vulkan