#pragma once
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_texture.h>
#include <vulkan/vk_texture_array.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Texture_View {

		std::string file = "";

		Image image;

		float width;

		float height;

		Const::TEXTURE_STORAGE_MODE storage_mode = Const::TEXTURE_STORAGE_MODE::BUCKET;

		int bucket_index;

		int slot_index;
	};

	struct Texture_System {

		VkDevice device = VK_NULL_HANDLE;

		std::vector<VkDescriptorPool> descriptor_pools;

		std::vector<Texture_Array> texture_buckets;

		std::map<uint32_t, Texture> ids_to_individual_textures;

		uint32_t counter_id = 0;

		std::stack<int> available_ids;

		std::map<std::string, uint32_t> files_to_ids;

		std::map<uint32_t, std::string> ids_to_files;

		std::map<uint32_t, Texture_View> ids_to_views;

		void init(
			std::vector<uint32_t> bucket_sizes, std::vector<uint32_t> number_texture_per_buckets, VkDevice device,
			std::vector<VkDescriptorPool> descriptor_pools
		);

		VkDescriptorSetLayout get_bucket_descriptor_set_layout();

		std::vector<VkDescriptorSet> make_bucket_descriptor_sets(VkDescriptorSetLayout layout);

		bool can_use_bucket(uint32_t width, uint32_t heihgt);

		uint32_t load_texture(std::string file, bool use_bucket = false);

		uint32_t load_texture(
			std::string file, void* pixels, int width, int height, int channels, bool use_bucket = false
		);

		Texture_View view_texture(uint32_t id);

		void remove_texture(uint32_t id);

		void destroy() const;
	};

} // namespace Vulkan
