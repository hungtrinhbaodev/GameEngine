#pragma once
#include <array>
#include <iostream>
#include <vulkan/vk_consts.h>
#include <vulkan/vulkan.h>

namespace Vulkan {
	struct Resource_Synchronize_Information {

		std::array<bool, Const::MAX_FRAMES_IN_FLIGHT> blocked{true};

		void release_frame(uint32_t frame_index);

		bool can_remove();
	};

	struct Buffer_Range {

		uint32_t offset = 0;

		uint32_t align = 0;

		uint32_t size = 0;

		template <typename T> inline uint32_t size_as() const { return size / sizeof(T); }

		template <typename T> inline uint32_t offset_as() const { return (offset + align) / sizeof(T); }

		uint32_t total_size() const;

		bool is_suitable_with(uint32_t size, uint32_t element_stride = 1) const;

		Buffer_Range fit_with(uint32_t size, uint32_t element_stride = 1);

		uint32_t inner_offset();

		friend std::ostream& operator<<(std::ostream& os, const Buffer_Range& range) {
			os << "Static_Buffer_Range: {offset: " << range.offset << " align: " << range.align
			   << ", size: " << range.size << "}";
			return os;
		}
	};

	struct Buffer_Range_Compare {
		bool operator()(const Buffer_Range& a, const Buffer_Range& b) const;
	};
} // namespace Vulkan