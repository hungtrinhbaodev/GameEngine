#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vk_texture.h>
#include <vulkan/vk_texture_array.h>
#include <vulkan/vk_consts.h>
#include <vector>
#include <stack>
#include <string>
#include <map>

namespace Vulkan {

	struct Texture_View {

		std::string file = "";

		Image image;

	};

	struct Texture_Storage_Info {

		uint32_t bucket_index = 0;
		
		uint32_t slot_index = 0;

		Const::TEXTURE_STORAGE_MODE storage_mode = Const::TEXTURE_STORAGE_MODE::BUCKET;

		Const::ASSETS_LOAD_STATE load_state = Const::ASSETS_LOAD_STATE::LOADED;

	};
	
	struct Texture_System {

		std::vector<Texture_Array> texture_buckets;

		std::map<uint32_t, Texture> ids_to_individual_textures;

		uint32_t counter_id = 0;

		std::stack<int> available_ids;

		std::map<std::string, uint32_t> files_to_ids;

		std::map<uint32_t, std::string> ids_to_files;
		
		std::map<uint32_t, Texture_View> ids_to_views;

		std::map<uint32_t, Texture_Storage_Info> ids_to_storages;

		void init(std::vector<uint32_t> bucket_sizes, std::vector<uint32_t> number_texture_per_buckets);

		bool can_use_bucket(uint32_t width, uint32_t heihgt);

		uint32_t load_texture(std::string file);

		Texture_View view_texture(uint32_t id);

		Texture_Storage_Info view_texture_storage_info(uint32_t id);

		void remove_texture(uint32_t id);

		void destroy() const;

	};

}
