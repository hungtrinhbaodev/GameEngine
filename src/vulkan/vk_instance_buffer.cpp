#include <vulkan/vk_instance_buffer.h>
#include <stdexcept>
#include <log.h>

namespace Vulkan {

	Instance_Buffer::Instance_Buffer() {}

	void Instance_Buffer::init(Ring_Buffer* global_staging_buffer, uint32_t initialize_buffer_size,
							   uint32_t instance_size) {
		stride = instance_size;
		available_size = initialize_buffer_size;
		staging_buffer = global_staging_buffer;
		inner_buffer.make_buffer(available_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
								 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		instances_data.resize(available_size);
		counter_id = 0;
		number_instance = 0;
	}

	uint32_t Instance_Buffer::add_data(void* data) {
		int id = -1, index = number_instance++;
		if (available_id.size() > 0) {
			id = available_id.top();
			available_id.pop();
		}
		if (id < 0) {
			id = counter_id++;
		}
		uint32_t total_size = number_instance * stride;
		if (available_size < total_size) {
			available_size = (uint32_t)(total_size * 1.5f);
			inner_buffer.resize(available_size);
			instances_data.resize(available_size);
		}
		ids_to_indices[id] = index;
		indices_to_ids[index] = id;

		char* dst_data = instances_data.data() + index * stride;
		memcpy(dst_data, data, stride);
		update_indices.insert(index);

		return static_cast<uint32_t>(id);
	}

	void Instance_Buffer::update_data(uint32_t id, void* update_data) {
		if (ids_to_indices.find(id) == ids_to_indices.end()) {
			return;
		}
		uint32_t index = ids_to_indices[id];
		char* dst_data = instances_data.data() + index * stride;
		memcpy(dst_data, update_data, stride);
		update_indices.insert(index);
	}

	void Instance_Buffer::remove_data(uint32_t id) {
		if (ids_to_indices.find(id) == ids_to_indices.end()) {
			return;
		}
		uint32_t index_need_remove = ids_to_indices[id];
		uint32_t replaced_index = --number_instance;
		if (indices_to_ids.find(replaced_index) == indices_to_ids.end()) {
			throw std::runtime_error("Fail to remove replaced id on instance buffer!");
		}
		uint32_t replaced_id = indices_to_ids[replaced_index];
		char* dst_data = instances_data.data() + index_need_remove * stride;
		char* src_data = instances_data.data() + replaced_index * stride;
		memcpy(src_data, dst_data, stride);
		ids_to_indices[replaced_id] = index_need_remove;
		indices_to_ids.erase(replaced_index);
		available_id.push(id);
		update_indices.insert(index_need_remove);
	}

	void Instance_Buffer::flush_data() {
		char* src_data = instances_data.data();
		for (auto& index : update_indices) {
			staging_buffer->upload_data(inner_buffer.buffer, index * stride, stride, src_data + index * stride);
		}
		update_indices.clear();
	}

	void Instance_Buffer::destroy() const {
		inner_buffer.destroy();
	}

} // namespace Vulkan