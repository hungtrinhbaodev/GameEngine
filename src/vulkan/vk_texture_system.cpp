#include <log.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_texture_system.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
#include <stdexcept>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	const std::string Texture_System::TEXTURE_DEFAULT_PATH = "Texture_System::TEXTURE_DEFAULT_PATH/**.png";

	void Texture_System::init(
		std::vector<uint32_t> bucket_sizes, std::vector<uint32_t> number_texture_per_buckets, VkDevice device,
		std::vector<VkDescriptorPool> descriptor_pools, VkFormat format
	) {
		this->device = device;
		this->descriptor_pools = descriptor_pools;
		this->format = format;

		/**
		 * @Note: Add texture default using to binding with the texture bucket or draw
		 * 3D need draw for model with empty texture.
		 */
		int number_channel = Utils::get_number_channel_by(format);
		std::vector<uint8_t> texture_default_bytes(number_channel);
		for (int i = 0; i < number_channel; i++) {
			texture_default_bytes[i] = 255;
		}
		default_texture_id = load_texture(TEXTURE_DEFAULT_PATH, texture_default_bytes.data(), 1, 1, number_channel);

		// @note: From now we disabled texture bucket to have full flow texture to test program first!
		if (!Const::ENABLED_TEXTURE_BUCKETS)
			return;

		if (bucket_sizes.size() != number_texture_per_buckets.size()) {
			throw std::runtime_error(
				"Vulkan fail to init texture system: number base sizes config need to equal "
				"number textures in buckets config size!"
			);
		}

		texture_buckets.resize(bucket_sizes.size());
		for (int i = 0; i < bucket_sizes.size(); i++) {
			uint32_t number_texture_per_bucket = number_texture_per_buckets[i];
			uint32_t texture_size = bucket_sizes[i];
			uint32_t mip_level =
				Const::ENEABLED_IMAGE_MIPMAP ? Utils::calculate_mip_level(texture_size, texture_size) : 1;
			texture_buckets[i].init(number_texture_per_bucket, texture_size, texture_size, format, mip_level);
		}
	}

	VkDescriptorSetLayout Texture_System::get_bucket_descriptor_set_layout() {
		Descriptor_Set_Layout_Builder layout_builder{};
		for (int i = 0; i < texture_buckets.size(); i++) {
			layout_builder.add_binding(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
		}
		return layout_builder.build();
	}

	std::vector<VkDescriptorSet> Texture_System::make_bucket_descriptor_sets(VkDescriptorSetLayout layout) {
		std::vector<VkDescriptorSet> descriptor_sets;
		std::vector<VkDescriptorImageInfo> bucket_descriptors;
		for (int i = 0; i < texture_buckets.size(); i++) {
			bucket_descriptors.push_back(texture_buckets[i].inner_image.get_descriptor_info());
		}
		for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorSet descriptor_set = Structs::make_descriptor_set(descriptor_pools[i], 1, &layout, device)[0];
			Descriptor_Set_Writer writer{};
			for (int j = 0; j < texture_buckets.size(); j++) {
				writer.add_image_write(j, 1, &bucket_descriptors[j], descriptor_set).write();
			}
			descriptor_sets.push_back(descriptor_set);
		}
		return descriptor_sets;
	}

	bool Texture_System::can_use_bucket(uint32_t width, uint32_t height) {
		if (texture_buckets.size() <= 0 || !Const::ENABLED_TEXTURE_BUCKETS) {
			return false;
		}
		Texture_Array& last_bucket = texture_buckets[texture_buckets.size() - 1];
		uint32_t max_size_width = last_bucket.inner_image.width;
		uint32_t max_size_height = last_bucket.inner_image.height;
		return (max_size_width >= width) && (max_size_height >= height);
	}

	uint32_t Texture_System::load_texture(
		std::string file, void* pixels, int width, int height, int channels, bool use_bucket
	) {
		// this texture was loaded success!
		if (files_to_ids.find(file) != files_to_ids.end()) {
			return files_to_ids[file];
		}

		// find suitable id
		int id = -1;
		if (available_ids.size() > 0) {
			id = available_ids.top();
			available_ids.pop();
		} else {
			id = ++counter_id;
		}

		// TODO: we will implement async load texture later ?
		Image used_image{};
		Const::TEXTURE_STORAGE_MODE storage_mode = Const::TEXTURE_STORAGE_MODE::INDIVIDUAL;
		int bucket_index = -1;
		int slot_index = 0;

		// If can't find suitable slot or can use bucket we fall back into using individual texture!
		bool need_use_individual_texture = false;
		if (use_bucket && can_use_bucket(width, height)) {

			// find suitalbe slot bucket
			int found_bucket_index = -1;
			for (int i = 0; i < texture_buckets.size(); i++) {
				Texture_Array& texture_array = texture_buckets[i];
				if (width <= texture_array.inner_image.width && height <= texture_array.inner_image.height) {
					found_bucket_index = i;
					break;
				}
			}
			if (found_bucket_index < 0) {
				need_use_individual_texture = true;
			} else {
				// find suitable slot index in texture bucket
				Texture_Array using_bucket = texture_buckets[found_bucket_index];
				int available_slot = using_bucket.find_availale_slot();

				if (available_slot < 0) {
					need_use_individual_texture = true;
					storage_mode = Const::TEXTURE_STORAGE_MODE::INDIVIDUAL;
				} else {
					// fill padding into image to fix with bucket
					uint32_t texture_size_width = using_bucket.inner_image.width;
					uint32_t texture_size_height = using_bucket.inner_image.height;
					std::vector<uint8_t> pixels_sized(texture_size_width * texture_size_height * channels);
					void* result = nullptr;
					if (channels == 1) {
						result = stbir_resize_uint8_linear(
							(const unsigned char*)pixels, width, height, 0, pixels_sized.data(), texture_size_width,
							texture_size_height, 0, STBIR_1CHANNEL
						);
					} else {
						result = stbir_resize(
							pixels, width, height, 0, pixels_sized.data(), texture_size_width, texture_size_height, 0,
							STBIR_RGBA, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT
						);
					}
					if (result == nullptr) {
						throw std::runtime_error("Failt to resize texture to bucket: " + file + "!");
					}
					using_bucket.upload_data(available_slot, pixels_sized.data());

					// save storage info with mode load buckets
					slot_index = available_slot;
					bucket_index = found_bucket_index;
					used_image = using_bucket.inner_image;
					storage_mode = Const::TEXTURE_STORAGE_MODE::BUCKET;
				}
			}
		} else {
			need_use_individual_texture = true;
		}

		if (need_use_individual_texture) {
			// load single individual texture
			Texture texture{};
			uint32_t mip_level = Const::ENEABLED_IMAGE_MIPMAP ? Utils::calculate_mip_level(width, height) : 1;
			texture.init(width, height, format, mip_level);
			texture.upload_data(pixels);
			ids_to_individual_textures[id] = texture;

			// save using image to view query
			used_image = texture.inner_image;
		}

		// add id of texture to tracking
		Texture_View view{file, used_image, (float)width, (float)height, storage_mode, bucket_index, slot_index};
		ids_to_views[id] = view;
		ids_to_files[id] = file;
		files_to_ids[file] = id;

		return static_cast<uint32_t>(id);
	}

	uint32_t Texture_System::load_texture(std::string file, bool use_bucket) {
		// this texture was loaded success!
		if (files_to_ids.find(file) != files_to_ids.end()) {
			return files_to_ids[file];
		}

		// load texture from disk
		int width = 0, height = 0, channels = 0;
		stbi_uc* pixels = nullptr;
		pixels = stbi_load(file.data(), &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels) {
			throw std::runtime_error("Vulkan fail to load texture from file: " + file + "!");
		}

		// load texture with raw pixels
		uint32_t texture_id = load_texture(file, pixels, width, height, channels, use_bucket);

		// release texture loaded on ram
		delete (pixels);

		return static_cast<uint32_t>(texture_id);
	}

	Texture_View Texture_System::view_texture(uint32_t id) {
		if (ids_to_views.find(id) == ids_to_views.end()) {
			return {""};
		}
		return ids_to_views[id];
	}

	uint32_t Texture_System::get_default_texture_id() {
		return default_texture_id;
	}

	void Texture_System::remove_texture(uint32_t id) {
		if (ids_to_files.find(id) == ids_to_files.end()) {
			return;
		}
		if (ids_to_views.find(id) == ids_to_views.end()) {
			throw std::runtime_error(
				"Vulkan fail to remove texture in texture system: not found storage info, can't handle remove!"
			);
		}
		Texture_View view_info = ids_to_views[id];
		switch (view_info.storage_mode) {
			case Const::TEXTURE_STORAGE_MODE::INDIVIDUAL: {
				if (ids_to_individual_textures.find(id) == ids_to_individual_textures.end()) {
					throw std::runtime_error(
						"Vulkan fail to remove texture in texture system: not found individual texture to remove!"
					);
				}
				Texture texture = ids_to_individual_textures[id];
				texture.destroy();
				break;
			}
			case Const::TEXTURE_STORAGE_MODE::BUCKET: {
				// In case bucket we don't need to erase because next load will replace all data
				break;
			}
			default: {
				break;
			}
		}
		std::string& file = ids_to_files[id];
		ids_to_views.erase(id);
		files_to_ids.erase(file);
		ids_to_files.erase(id);
		ids_to_individual_textures.erase(id);
	}

	void Texture_System::destroy() const {
		for (auto& texture_array : texture_buckets) {
			texture_array.destroy();
		}
		for (auto& [_, texture] : ids_to_individual_textures) {
			texture.destroy();
		}
	}

} // namespace Vulkan