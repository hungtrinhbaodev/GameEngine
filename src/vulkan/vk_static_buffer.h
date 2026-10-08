#pragma once
#include <vulkan/vulkan.h>

#include <id_generator.h>
#include <sparse_set.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_ring_buffer.h>
#include <vulkan/vk_ssbo_buffer.h>

#include <iostream>
#include <map>
#include <queue>
#include <stack>
#include <vector>

namespace Vulkan {

	struct Static_Buffer_Range {

		uint32_t offset;

		uint32_t size;

		template <typename T> uint32_t size_as() { return size / sizeof(T); }

		template <typename T> uint32_t offset_as() { return offset / sizeof(T); }

		inline friend std::ostream& operator<<(std::ostream& os, const Static_Buffer_Range& range) {
			os << "Offset: " << range.offset << " Size: " << range.size;
			return os;
		}
	};

	struct Static_Buffer_Range_Compare {
		inline bool operator()(const Static_Buffer_Range& a, const Static_Buffer_Range& b) const {
			return a.size > b.size;
		}
	};

	/*
		Using to storage all static data upload once use many
		like vertex, mesh, indices,... of model
	*/
	struct Static_Buffer {

		Buffer inner_buffer;

		std::stack<int> available_ids;

		std::priority_queue<Static_Buffer_Range, std::vector<Static_Buffer_Range>, Static_Buffer_Range_Compare>
			available_ranges;

		std::map<uint32_t, Static_Buffer_Range> ranges_by_id;

		uint32_t available_size;

		uint32_t id_counter;

		uint32_t current_offset;

		Ring_Buffer* staging_buffer;

		bool track_log = false;

		void init(
			Ring_Buffer* global_staging_buffer, uint32_t initialize_size, VkBufferUsageFlags usage_flags,
			VkPhysicalDevice physical_device, VkDevice device
		);

		uint32_t upload_data(uint32_t size, void* data);

		Static_Buffer_Range view_slot_info(uint32_t id);

		bool remove_data(uint32_t id);

		void destroy();
	};

	struct Static_Buffer_Range_2 {

		uint32_t offset = 0;

		uint32_t align = 0;

		uint32_t size = 0;

		template <typename T> inline uint32_t size_as() const { return size / sizeof(T); }

		template <typename T> inline uint32_t offset_as() const { return (offset + align) / sizeof(T); }

		inline uint32_t total_size() const { return align + size; }

		inline bool is_suitable_with(uint32_t size, uint32_t element_stride) const {
			element_stride = std::max(uint32_t(1), element_stride);
			uint32_t align = element_stride - (offset % element_stride);
			return total_size() >= size + align;
		}

		inline Static_Buffer_Range_2 fit_with(uint32_t size, uint32_t element_stride) {
			if (!is_suitable_with(size, element_stride)) {
				throw std::runtime_error("Fail to fix static data in to current range!");
			}
			uint32_t new_align = 0;
			uint32_t new_size = size;
			element_stride = std::max(uint32_t(1), element_stride);
			new_align = element_stride - (offset % element_stride);
			uint32_t remain_offset = offset + (new_align + new_size);
			uint32_t remain_size = total_size() - (new_align + new_size);
			this->align = new_align;
			this->size = new_size;
			return {remain_offset, 0, remain_size};
		}

		inline uint32_t inner_offset() { return offset + align; }

		inline friend std::ostream& operator<<(std::ostream& os, const Static_Buffer_Range_2& range) {
			os << "Static_Buffer_Range: {offset: " << range.offset << " align: " << range.align
			   << ", size: " << range.size << "}";
			return os;
		}
	};

	struct Static_Buffer_Range_Compare_2 {
		inline bool operator()(const Static_Buffer_Range_2& a, const Static_Buffer_Range_2& b) const {
			return a.total_size() > b.total_size();
		}
	};

	struct Static_Buffer_2 {

		Buffer inner_buffer{};

		Id_Generator range_id_generator{};

		Sparse_Set<Static_Buffer_Range_2> id_to_ranges{};

		Sparse_Set<uint32_t> ssbo_id_to_id{};

		Sparse_Set<uint32_t> id_to_ssbo_id{};

		SSBO_Buffer staging_buffer{};

		std::priority_queue<Static_Buffer_Range_2, std::vector<Static_Buffer_Range_2>, Static_Buffer_Range_Compare_2>
			available_ranges;

		uint32_t current_offset = 0;

		uint32_t size_required = 0;

		void init(
			uint32_t initialize_size, uint32_t initialize_staging_size, VkBufferUsageFlags usage_flags,
			VkPhysicalDevice physical_device, VkDevice device
		);

		uint32_t upload_data(void* data, uint32_t size, uint32_t element_stride = 1);

		const Static_Buffer_Range_2& view_slot_info(uint32_t id);

		bool remove_data(uint32_t id);

		void flush_data();

		void clear();

		void destroy();
	};

} // namespace Vulkan