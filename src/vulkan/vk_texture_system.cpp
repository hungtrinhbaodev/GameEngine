#include <vulkan/vk_texture_system.h>
#include <stb_image.h>
#include <stdexcept>

namespace Vulkan {

	void Texture_System::init(std::vector<uint32_t> bucket_sizes, std::vector<uint32_t> number_texture_per_buckets) {

		if (bucket_sizes.size() != number_texture_per_buckets.size()) {
			throw std::runtime_error("Vulkan fail to init texture system: number base sizes config need to equal number textures in buckets config size!");
		}

		texture_buckets.resize(bucket_sizes.size());
		for (int i = 0; i < bucket_sizes.size(); i++) {
			uint32_t number_texture_per_bucket = number_texture_per_buckets[i];
			uint32_t texture_size = bucket_sizes[i];
			texture_buckets[i].init(number_texture_per_bucket, texture_size, texture_size);
			texture_size *= 2;
		}

	}

	bool Texture_System::can_use_bucket(uint32_t width, uint32_t height) {
		if (texture_buckets.size() <= 0) {
			return false;
		}
		Texture_Array& last_bucket = texture_buckets[texture_buckets.size() - 1];
		uint32_t max_size_width = last_bucket.inner_image.width;
		uint32_t max_size_height = last_bucket.inner_image.height;
		return (max_size_width >= width) && (max_size_height >= height);
	}

	uint32_t Texture_System::load_texture(std::string file) {

		// this texture was loaded success!
		if (files_to_ids.find(file) != files_to_ids.end()) {
			return files_to_ids[file];
		}

		// find suitable id
		int id = -1;
		if (available_ids.size() > 0) {
			id = available_ids.top();
			available_ids.pop();
		}
		else {
			id = ++counter_id;
		}

		// load texture from disk
		int width = 0, height = 0, channels = 0;
		stbi_uc* pixels = nullptr;
		pixels = stbi_load(
			file.data(),
			&width,
			&height,
			&channels,
			STBI_rgb_alpha
		);
		if (!pixels) {
			throw std::runtime_error("Vulkan fail to load texture from file!");
		}

		// TODO: we will implement async load texture later ?
		Texture_Storage_Info storage_info{0, 0, Const::TEXTURE_STORAGE_MODE::BUCKET, Const::ASSETS_LOAD_STATE::LOADED};
		Image used_image{};

		// If can't find suitable slot or can use bucket we fall back into using individual texture!
		bool need_use_individual_texture = false;
		if (can_use_bucket(width, height)) {

			// find suitalbe slot bucket
			uint32_t bucket_index = -1;
			for (int i = 0; i < texture_buckets.size(); i++) {
				Texture_Array& texture_array = texture_buckets[i];
				if (width <= texture_array.inner_image.width && height <= texture_array.inner_image.height) {
					bucket_index = i;
					break;
				}
			}
			if (bucket_index < 0) {
				throw std::runtime_error("Vulkan fail to find suitable bucket texture array!");
			}

			// find suitable slot index in texture bucket
			Texture_Array using_bucket = texture_buckets[bucket_index];
			int availble_slot = using_bucket.find_availale_slot();

			if (availble_slot < 0) {
				need_use_individual_texture = true;
			}
			else {
				// fill padding into image to fix with bucket
				uint32_t texture_size_width = using_bucket.inner_image.width;
				uint32_t texture_size_height = using_bucket.inner_image.height;
				std::vector<char> pixels_padded(texture_size_width * texture_size_height * 4, 0);
				memcpy(pixels_padded.data(), pixels, static_cast<size_t>(width * height * 4));
				using_bucket.upload_data(availble_slot, pixels_padded.data());

				// save storage info with mode load buckets
				storage_info.bucket_index = bucket_index;
				storage_info.slot_index = availble_slot;

				// save using image to view query
				used_image = using_bucket.inner_image;
			}
		}
		else {
			need_use_individual_texture = true;
		}

		if (need_use_individual_texture) {
			// load single individual texture
			Texture texture{};
			texture.init(width, height);
			texture.upload_data(pixels);
			ids_to_individual_textures[id] = texture;

			// save storage info with mode load individual
			storage_info.storage_mode = Const::TEXTURE_STORAGE_MODE::INDIVIDUAL;

			// save using image to view query
			used_image = texture.inner_image;
		}

		// add id of texture to tracking
		Texture_View view{ file, used_image };
		ids_to_views[id] = view;
		ids_to_files[id] = file;
		files_to_ids[file] = id;
		ids_to_storages[id] = storage_info;

		// release texture loaded on ram
		delete(pixels);

		return static_cast<uint32_t>(id);
	}

	Texture_View Texture_System::view_texture(uint32_t id) {
		if (ids_to_views.find(id) == ids_to_views.end()) {
			return { "" };
		}
		return ids_to_views[id];
	}

	Texture_Storage_Info Texture_System::view_texture_storage_info(uint32_t id) {
		if (ids_to_storages.find(id) == ids_to_storages.end()) {
			return {0, 0, Const::TEXTURE_STORAGE_MODE::INVALID};
		}
		return ids_to_storages[id];
	}

	void Texture_System::remove_texture(uint32_t id) {
		if (ids_to_files.find(id) == ids_to_files.end()) {
			return;
		}
		if (ids_to_storages.find(id) == ids_to_storages.end()) {
			throw std::runtime_error("Vulkan fail to remove texture in texture system: not found storage info, can't handle remove!");
		}
		Texture_Storage_Info storage_info = ids_to_storages[id];
		switch (storage_info.storage_mode) {
			case Const::TEXTURE_STORAGE_MODE::INDIVIDUAL: {
				if (ids_to_individual_textures.find(id) == ids_to_individual_textures.end()) {
					throw std::runtime_error("Vulkan fail to remove texture in texture system: not found individual texture to remove!");
				}
				Texture texture = ids_to_individual_textures[id];
				texture.destroy();
				break;
			}
		}
		std::string& file = ids_to_files[id];
		ids_to_views.erase(id);
		files_to_ids.erase(file);
		ids_to_files.erase(id);
		ids_to_storages.erase(id);
	}

	void Texture_System::destroy() const {
		for (auto& texture_array : texture_buckets) {
			texture_array.destroy();
		}
		for (auto& [_, texture] : ids_to_individual_textures) {
			texture.destroy();
		}
	}

}