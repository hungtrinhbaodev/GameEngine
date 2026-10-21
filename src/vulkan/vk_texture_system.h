#pragma once
#include <id_generator.h>
#include <map>
#include <sparse_set.h>
#include <stack>
#include <string>
#include <vector>
#include <vulkan/vk_common.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_texture.h>
#include <vulkan/vk_texture_array.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Texture_System {

		static const std::string TEXTURE_DEFAULT_PATH;

		VkPhysicalDevice physical_device = VK_NULL_HANDLE;

		VkDevice device = VK_NULL_HANDLE;

		std::vector<VkDescriptorPool> descriptor_pools;

		std::array<VkDescriptorPool, Const::MAX_FRAMES_IN_FLIGHT> descriptor_pools_2;

		std::vector<Texture_Array> texture_buckets;

		std::map<uint32_t, Texture> ids_to_individual_textures;

		std::map<std::string, uint32_t> files_to_ids;

		std::map<uint32_t, std::string> ids_to_files;

		Sparse_Set<Texture_View> ids_to_views{};

		uint32_t default_texture_id = 0;

		VkFormat format;

		Id_Generator id_generator{};

		bool can_gpu_blit = false;

		void init(
			std::vector<uint32_t> bucket_sizes, std::vector<uint32_t> number_texture_per_buckets, VkDevice device,
			std::vector<VkDescriptorPool> descriptor_pools,
			std::array<VkDescriptorPool, Const::MAX_FRAMES_IN_FLIGHT> descriptor_pools_2,
			VkPhysicalDevice physical_device, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB
		);

		VkDescriptorSetLayout make_bucket_descriptor_set_layout();

		std::vector<VkDescriptorSet> make_bucket_descriptor_sets(VkDescriptorSetLayout layout);

		std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> make_bucket_descriptor_sets_2(
			VkDescriptorSetLayout layout
		);

		std::vector<VkDescriptorSet> make_default_texture_descriptor_sets(VkDescriptorSetLayout layout);

		std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> make_default_texture_descriptor_sets_2(
			VkDescriptorSetLayout layout
		);

		bool can_use_bucket(uint32_t width, uint32_t heihgt);

		uint32_t load_texture(std::string file, bool use_bucket = false);

		uint32_t load_texture(
			std::string file, void* pixels, int width, int height, int channels, bool use_bucket = false
		);

		const Texture_View& view_texture(uint32_t id);

		uint32_t get_default_texture_id();

		void remove_texture(uint32_t id);

		void destroy() const;
	};

} // namespace Vulkan
