#include <id_generator.h>
#include <math_custom.h>
#include <sparse_set.h>
#include <vulkan/draws/vk_clipping.h>
#include <vulkan/vk_core.h>

namespace Vulkan {

	namespace Draw_Clipping_2D {

		bool is_log = false;

		long long capture_frame = 0;

		struct Storage_Data {
			glm::vec2 half_size{0.f, 0.f}; // 8
			glm::vec2 position{0.f};	   // 8
			glm::vec2 scale{1.f};		   // 8
			float border_radius = 0.f;	   // 4
			float rotation = 0.f;		   // 4
			friend std::ostream& operator<<(std::ostream& os, const Storage_Data& instance) {
				os << "{half_size: " << instance.half_size << ",  position: " << instance.position
				   << ",  scale: " << instance.scale << ", border_radius: " << instance.border_radius
				   << ", rotation: " << instance.rotation << "}";
				return os;
			}
			void make(const Clipping_2D& attributes) {
				glm::vec2 size = attributes.get_content_size();
				glm::vec2 center = (glm::vec2{0.5f} - attributes.get_anchor()) * size;
				float world_rotation = attributes.get_world_rotation();
				glm::vec2 world_scale = attributes.get_world_scale();
				half_size = (size - attributes.get_border_width() * world_scale) * 0.5f;
				border_radius = attributes.get_border_radius();
				position = attributes.get_world_position() + Math::rotate(world_rotation, center * world_scale);
				scale = world_scale;
				rotation = world_rotation;
			}
		};

		struct Clipping_Data {
			uint32_t storage_id = 0;
		};

		Id_Generator unique_id_generator;

		Sparse_Set<Clipping_Data> clippings;

		void init_clipping(Draw_Information& draw_info, const Clipping_2D& attributes) {
			uint32_t unique_id = unique_id_generator.gen_id();
			Storage_Data storage{};
			storage.make(attributes);
			SSBO_Buffer& ssbo = get_ssbo();
			uint32_t storage_id = ssbo.upload_data(&storage, sizeof(Storage_Data), sizeof(Storage_Data));
			Clipping_Data clipping{storage_id};
			clippings.insert(unique_id, clipping);
			draw_info.sub_unique_id = unique_id;
		}

		void update_clipping(Draw_Information& draw_info, const Clipping_2D& attributes) {
			if (!clippings.has(draw_info.sub_unique_id)) {
				return;
			}
			const Clipping_Data& clipping = clippings.get(draw_info.sub_unique_id);
			Storage_Data storage{};
			storage.make(attributes);
			SSBO_Buffer& ssbo = get_ssbo();
			ssbo.update_data(clipping.storage_id, &storage);
		}

		void remove_clipping(uint32_t unique_id) {
			if (!clippings.has(unique_id)) {
				return;
			}
			const Clipping_Data& clipping = clippings.get(unique_id);
			SSBO_Buffer& ssbo = get_ssbo();
			ssbo.remove_data(clipping.storage_id);
			unique_id_generator.release_id(unique_id);
		}

		void debug(uint32_t frame_index) {
			if (!is_log) {
				SSBO_Buffer& ssbo = get_ssbo();
				const auto& clippings = Draw_Clipping_2D::clippings.values();
				if (clippings.size() >= 2 && capture_frame <= 0) {
					capture_frame = frame_count;
					return;
				}
				if (capture_frame <= 0)
					return;
				if (frame_count - capture_frame == Const::MAX_FRAMES_IN_FLIGHT) {
					for (int i = 0; i < 2 && i < clippings.size(); i++) {
						const auto& clipping = clippings[i];
						const Buffer_Range& range = ssbo.view_slot(clipping.storage_id);
						auto storage = ssbo.storage_buffers[frame_index].parse_to<Storage_Data>(
							range.inner_offset(), sizeof(Storage_Data), 1
						)[0];
						Log::info("What is my clipping data", storage);
						is_log = true;
					}
				}
			}
		}

		Clipping_Helper get_clipping_helper(const uint32_t& unique_id) {
			if (!clippings.has(unique_id)) {
				return {};
			}
			const Clipping_Data& clipping = clippings.get(unique_id);
			SSBO_Buffer& ssbo = get_ssbo();
			const Buffer_Range& range = ssbo.view_slot(clipping.storage_id);
			Clipping_Helper helper{};
			helper.storage_offset = range.offset_as<Storage_Data>();
			return helper;
		}

	} // namespace Draw_Clipping_2D

} // namespace Vulkan