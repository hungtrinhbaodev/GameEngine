#pragma once
#include <iostream>
#include <map>

#include <graphic/vulkan_implementation/vulkan_dynamic_buffer.h>

namespace Graphic {

    struct Vulkan_Mesh_Buffer_Offset {
        uint32_t offset = 0;
        uint32_t size = 0;
    };

    template<typename Key>
    class Vulkan_Mesh_Buffer : public Vulkan_Dynamic_Buffer {

        private:

        uint32_t _using_size = 0;

        std::map<Key, Vulkan_Mesh_Buffer_Offset> _offset_data;

        std::vector<Vulkan_Mesh_Buffer_Offset> _unused_memory;

        int _find_suitable_unused_offset_index(uint32_t size) {
            uint32_t min_diff_size = -1;
            int found_index = -1;
            for (int i = 0; i < _unused_memory.size();i++) {
                if (_unused_memory[i].size >= size && min_diff_size > (_unused_memory[i].size - size)) {
                    min_diff_size = _unused_memory[i].size - size;
                    found_index = i;
                }
            }
            return found_index;
        }

        void _update_unused_offset_at_index(int index, uint32_t size) {
            if (index >= _unused_memory.size()) {
                throw std::runtime_error("fail to update unused offset with index: index out about!");
            }

            if (_unused_memory[index].size < size) {
                throw std::runtime_error("fail to update unused offset with index: size need greater than size has!");
            }

            if (_unused_memory[index].size == size) {
                Utility::Func_Utils::swap(_unused_memory[index], _unused_memory[_unused_memory.size() - 1]);
                _unused_memory.pop_back();
                return;
            }

            _unused_memory[index].offset += size;
            _unused_memory[index].size -= size;
        }

        void _add_unused_offset_data(Vulkan_Mesh_Buffer_Offset unused_offset) {

            // add unused chunk memory
            _unused_memory.push_back(unused_offset);

            // sort chunk unused memory
            std::sort(_unused_memory.begin(), _unused_memory.end(), [] (const auto& a, const auto& b) {
                return b.offset > a.offset;
            });

            // merge chunks memory if it overlap with each other
            std::vector<Vulkan_Mesh_Buffer_Offset> tmp_unsed_memory;
            tmp_unsed_memory.emplace_back(_unused_memory[0]);
            int current_merge_index = 0;
            for (int i = 1;i < _unused_memory.size();i++) {
                auto& last_chunk = tmp_unsed_memory[current_merge_index];
                const auto& current_chunk = _unused_memory[i];
                if (last_chunk.offset + last_chunk.size >= current_chunk.offset) {
                    last_chunk.size += (current_chunk.offset + current_chunk.size) - (last_chunk.offset + last_chunk.size);
                }
                else {
                    tmp_unsed_memory.push_back(current_chunk);
                    current_merge_index++;
                }
            }
            _unused_memory = std::move(tmp_unsed_memory);
        }

        void _add_offset_data(const Key& key, uint32_t offset, uint32_t size) {
            _offset_data[key] = {
                offset, 
                size
            };
        }

        void _log_buffer_data(const Key& key) {
            map_memory();
            const auto& parse_data = Utility::Func_Utils::parse_data<int>(_map_ptr, 0, _size);
            Utility::Log::get()->log_info("append data", key, parse_data, "buffer propertices flag:", _property_flags);
            unmap_memory();

            for (const auto& [key, offset] : _offset_data) {
                Utility::Log::get()->log_info("Offset info at key", key, "offset: ", offset.offset, "size: ", offset.size);
            }

            for (const auto& unused_offset : _unused_memory) {
                Utility::Log::get()->log_info("Unused offset info", "offset: ", unused_offset.offset, "size: ", unused_offset.size);
            }
        }
        
        public:

        void push_data(const Key& key, void* data, uint32_t offset, uint32_t size) {

            // If push already exist key we will throw a exception
            if (_offset_data.find(key) != _offset_data.end()) {
                throw std::runtime_error("fail to push data to dynamic buffer: key push already in!");
            }

            // Find the size suitable unused memory if has
            int available_unsed_offset_index = _find_suitable_unused_offset_index(size);
            if (available_unsed_offset_index != -1) {
                // make a copy data to buffer offset
                uint32_t dst_offset = _unused_memory[available_unsed_offset_index].offset;
                // copy data using to available offset
                _copy_data_to_offset(data, offset, dst_offset, size);
                // add offset info of new chunk data
                _add_offset_data(key, dst_offset, size);
                // update remain offset unused when take some memory from it
                _update_unused_offset_at_index(available_unsed_offset_index, size);
                // log to test byte is sent
                _log_buffer_data(key);
                return;
            }

            // Find the size if buffer need resize
            uint32_t size_need_append = 0;
            if (_size < _using_size + size) {
                size_need_append = _using_size + size - _size;
            }

            // Resize the buffer to push data
            if (size_need_append > 0) {
                _on_resize(size_need_append);
            } 

            // Copy the using data to last offset using
            _copy_data_to_offset(data, offset, _using_size, size);

            // add offset info of new chunk data
            _add_offset_data(key, _using_size, size);

            // log to test byte is sent
            _log_buffer_data(key);

            // update using size to next using push data
            _using_size += size;
        }

        void delete_data(const Key& key) {
            // if not have key do notthing
            if (_offset_data.find(key) == _offset_data.end()) {
                return;
            }

            // add offset data of key to unused memory
            _add_unused_offset_data(_offset_data[key]);

            // clear key from data to un tracking
            _offset_data.erase(key);
        }

        Vulkan_Mesh_Buffer_Offset get_offset(const Key& key) {
            if (_offset_data.find(key) == _offset_data.end()) {
                return {
                    0, 0
                };
            }
            else {
                return _offset_data[key];
            }
        }
    };
 
}