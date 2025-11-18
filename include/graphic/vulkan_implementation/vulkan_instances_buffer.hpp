#pragma once

#include <map>
#include <queue>

#include <graphic/vulkan_implementation/vulkan_dynamic_buffer.h>

namespace Graphic {

    template<typename ID, typename Mat_Transfrom>
    class Vulkan_Instances_Buffer : public Vulkan_Dynamic_Buffer {

        private:

        std::map<ID, uint32_t> _id_to_index;

        std::map<uint32_t, ID> _index_to_id;

        uint32_t _number_element = 0;

        const static uint32_t DEFAULT_APPEND_SIZE = 10 * sizeof(Mat_Transfrom);

        public:

        uint32_t get_using_size() {
            return sizeof(Mat_Transfrom) * _number_element;
        }

        uint32_t get_number_instance() {
            return _number_element;
        }

        void add_data(const ID& id, void* data, uint32_t offset) {

            std::unique_lock<std::mutex> lock(_buffer_lock);

            // constant size of one transform data
            uint32_t size = sizeof(Mat_Transfrom);

            // if current size of buffer does not
            // could add we resize buffer
            if (get_using_size() + size > _size) {
                _on_resize(DEFAULT_APPEND_SIZE);
            }

            // make a copy data to next offset of buffer
            _copy_data_to_offset(
                data,
                offset,
                get_using_size(),
                size
            );

            // update tracking data map id to index
            _id_to_index[id] = _number_element;
            _index_to_id[_number_element] = id;

            // update number element in buffer
            _number_element++;

            log_buffer_data("instance buffer add: ");
        }

        void add_data(const ID& id, const Mat_Transfrom& data) {
            Utility::Log::get()->log_info("instances buffer add data", id, data, get_using_size());
            Mat_Transfrom* non_const_ptr = const_cast<Mat_Transfrom*>(&data);
            add_data(id, static_cast<void*>(non_const_ptr), 0);
        }

        void update_data(const ID& id, void* data, uint32_t offset) {

            std::unique_lock<std::mutex> lock(_buffer_lock);

            if (_id_to_index.find(id) == _id_to_index.end()) {
                throw std::runtime_error("fail to update data in instances buffer: buffer doesn't have element id!");
            }

            // constant size of one transform data
            uint32_t size = sizeof(Mat_Transfrom);

            int dst_offset = _id_to_index[id] * size;

            _copy_data_to_offset(
                data,
                offset,
                dst_offset,
                size
            );
        }

        void update_data(const ID& id, const Mat_Transfrom& data) {
            Utility::Log::get()->log_info("instances buffer update data", id, data);
            Mat_Transfrom* non_const_ptr = const_cast<Mat_Transfrom*>(&data);
            update_data(id, static_cast<void*>(non_const_ptr), 0);
        }

        void delete_data(const ID& id) {

            std::unique_lock<std::mutex> lock(_buffer_lock);

            if (_id_to_index.find(id) == _id_to_index.end()) {
                throw std::runtime_error("fail to delete data in instances buffer: buffer doesn't have element with id!");
            }

            // constant size of one transform data
            uint32_t size = sizeof(Mat_Transfrom);

            Vulkan_Buffer* staging_buffer = new Vulkan_Buffer();
            staging_buffer->make(
                static_cast<VkDeviceSize>(size),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                _property_flags
            );

            uint32_t last_index = _number_element - 1;
            Vulkan_Buffer::copy_buffer(
                Vulkan_Commands_Mode::COMMANDS_MODE_SYNC,
                this,
                staging_buffer,
                {{last_index * size, 0, static_cast<VkDeviceSize>(size)}}
            );

            uint32_t index_remove = _id_to_index[id];
            uint32_t dst_offset = index_remove * size;
            Vulkan_Buffer::copy_buffer(
                Vulkan_Commands_Mode::COMMANDS_MODE_SYNC,
                staging_buffer,
                this,
                {{0, dst_offset, static_cast<VkDeviceSize>(size)}},
                [staging_buffer] () {
                    staging_buffer->destroy();
                }
            );

            const ID& id_swap_index = _index_to_id[last_index];
            _id_to_index[id_swap_index] = index_remove;
            _index_to_id[index_remove] = id_swap_index;
            _id_to_index.erase(id);
            _index_to_id.erase(last_index);

            _number_element--;
        }

        std::vector<Mat_Transfrom> get_data_in_buffer() {
            std::vector<Mat_Transfrom> data = Utility::Func_Utils::revert_data<Mat_Transfrom>(
                Utility::Func_Utils::parse_data<uint8_t>(_map_ptr, 0, get_using_size())
            );
            return data;
        }

        Mat_Transfrom get_data(const ID& id) {
            if (_id_to_index.find(id) == _id_to_index.end()) {
                throw std::runtime_error("fail to get data in instances buffer: buffer doesn't have element with id!");
            }
            int index = _id_to_index[id];
            return get_data_in_buffer()[index];
        }
    };

}