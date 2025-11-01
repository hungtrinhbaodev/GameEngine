#include <graphic/vulkan_implementation/vulkan_descriptor.h>

/**
 * Main class field
 */

void Graphic::Vulkan_Descriptor::set_descriptors_types(const std::vector<VkDescriptorType>& vk_descriptor_types) {
    _vk_descriptor_types = vk_descriptor_types;
}

void Graphic::Vulkan_Descriptor::set_descriptors_shader_flags(const std::vector<VkShaderStageFlags>& descriptor_shader_flags) {
    _vk_descriptor_shader_flags = descriptor_shader_flags;
}

void Graphic::Vulkan_Descriptor::set_vk_device(VkDevice vk_device) {
    _vk_device = vk_device;
}

VkDescriptorSetLayoutBinding Graphic::Vulkan_Descriptor::make_descriptor_set_layout_binding(
    uint32_t binding, 
    VkDescriptorType vk_descriptor_type,
    VkShaderStageFlags vk_stageFlags
) {
    VkDescriptorSetLayoutBinding desciptor_binding{};
    desciptor_binding.binding = binding;
    desciptor_binding.descriptorType = vk_descriptor_type;
    desciptor_binding.descriptorCount = 1;
    desciptor_binding.pImmutableSamplers = nullptr;
    desciptor_binding.stageFlags = vk_stageFlags;
    return desciptor_binding;
}

void Graphic::Vulkan_Descriptor::init_descriptor_set_layouts() {
    _vk_descriptor_set_layouts.resize(_vk_descriptor_types.size());
    for (int i = 0;i < _vk_descriptor_types.size();i++) {
        VkDescriptorSetLayoutBinding binding = make_descriptor_set_layout_binding(
            0,
            _vk_descriptor_types[i],
            _vk_descriptor_shader_flags[i]
        );
        VkDescriptorSetLayoutCreateInfo descriptor_info{};
        descriptor_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_info.bindingCount = 1;
        descriptor_info.pBindings = &binding;
        Vulkan_Utility::vk_check_action(
            vkCreateDescriptorSetLayout(_vk_device, &descriptor_info, nullptr, &_vk_descriptor_set_layouts[i]),
            "fail to create descriptor layout set!"
        );
    }
}

void Graphic::Vulkan_Descriptor::init_descriptor_pool() {
    /**
     * Make descriptor pool
     */
    std::vector<VkDescriptorPoolSize> pool_sizes(_vk_descriptor_types.size());
    for (int i = 0; i < _vk_descriptor_types.size(); i++) {
        switch (_vk_descriptor_types[i]) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                pool_sizes[i].descriptorCount = static_cast<uint32_t>(Vulkan_Constants::MAX_FRAMES_IN_FLIGHT);
                pool_sizes[i].type = _vk_descriptor_types[i];
                break;
            }
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                pool_sizes[i].descriptorCount = Vulkan_Constants::MAX_FRAMES_IN_FLIGHT * Vulkan_Constants::MAX_NUMBER_DESCRIPTOR_TEXTURE;
                pool_sizes[i].type = _vk_descriptor_types[i];
                break;
            }
            default: {
                break;
            }
        }
    }

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = static_cast<uint32_t>(
        Vulkan_Constants::MAX_FRAMES_IN_FLIGHT 
        * Vulkan_Constants::MAX_NUMBER_DESCRIPTOR_TEXTURE
        + Vulkan_Constants::MAX_FRAMES_IN_FLIGHT
    );

    Vulkan_Utility::vk_check_action(
        vkCreateDescriptorPool(_vk_device, &pool_info, nullptr, &_vk_descriptor_pool),
        "fail to create pool descriptor!"
    );
    Utility::Log::get()->log_info("create pool descriptor success!");

    std::vector<VkDescriptorSetLayout> layouts;
    for (int i = 0;i < _vk_descriptor_types.size();i++) {
        switch (_vk_descriptor_types[i]) {
            /**
             * Make descriptor sets uniform with Vulkan_Constants::MAX_FRAMES_IN_FLIGHT first set
             */
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                for (int j = 0;j < Vulkan_Constants::MAX_FRAMES_IN_FLIGHT;j++) {
                    layouts.push_back(_vk_descriptor_set_layouts[i]);
                }
                break;
            }
            /**
             * Make descriptor sets texture sampler Vulkan_Constants::MAX_FRAMES_IN_FLIGHT * Vulkan_Constants::MAX_NUMBER_DESCRIPTOR_TEXTURE second set
             */
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                for (int j = 0;j < Vulkan_Constants::MAX_FRAMES_IN_FLIGHT * Vulkan_Constants::MAX_NUMBER_DESCRIPTOR_TEXTURE;j++) {
                    layouts.push_back(_vk_descriptor_set_layouts[i]);
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    _vk_descriptor_sets.resize(layouts.size());
    Utility::Log::get()->log_info("What is size", layouts.size());
    VkDescriptorSetAllocateInfo layout_allocator_info{};
    layout_allocator_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    layout_allocator_info.descriptorPool = _vk_descriptor_pool;
    layout_allocator_info.descriptorSetCount = layouts.size();
    layout_allocator_info.pSetLayouts = layouts.data();
    Vulkan_Utility::vk_check_action(
        vkAllocateDescriptorSets(_vk_device, &layout_allocator_info, _vk_descriptor_sets.data()),
        "fail to allocate descriptor sets samplers!"
    );
}

void Graphic::Vulkan_Descriptor::update_uniform_descriptor(
    std::vector<Vulkan_Buffer>& uniform_buffers
) {
    for (int i = 0; i < Vulkan_Constants::MAX_FRAMES_IN_FLIGHT; i++) {
        Vulkan_Buffer& buffer = uniform_buffers[i];
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer.get();
        buffer_info.offset = 0;
        buffer_info.range = buffer.get_size(); 

        VkWriteDescriptorSet uniform_write{};
        uniform_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniform_write.dstSet = _vk_descriptor_sets[i];
        uniform_write.dstBinding = 0;
        uniform_write.dstArrayElement = 0;
        uniform_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_write.descriptorCount = 1;
        uniform_write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(_vk_device, 1, &uniform_write, 0, nullptr);
    }
}

void Graphic::Vulkan_Descriptor::update_texture_descriptor(
    std::shared_ptr<Vulkan_Texture> vk_texture
) {

    int texture_index = key_to_index_generater.gen_index_by_key(vk_texture->get_key());
    int start_index = Vulkan_Constants::MAX_FRAMES_IN_FLIGHT + texture_index * Vulkan_Constants::MAX_FRAMES_IN_FLIGHT;

    for (int i = start_index;i < start_index + Vulkan_Constants::MAX_FRAMES_IN_FLIGHT;i++) {
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = vk_texture->get_vk_imageview();
        image_info.sampler = vk_texture->get_vk_sampler();

        VkWriteDescriptorSet texture_write{};
        texture_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        texture_write.dstSet = _vk_descriptor_sets[i];
        texture_write.dstBinding = 0;
        texture_write.dstArrayElement = 0;
        texture_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texture_write.descriptorCount = 1;
        texture_write.pImageInfo = &image_info;

        vkUpdateDescriptorSets(_vk_device, 1, &texture_write, 0, nullptr);
    }
}

const std::vector<VkDescriptorSetLayout>& Graphic::Vulkan_Descriptor::get_descriptor_set_layouts() {
    return _vk_descriptor_set_layouts;
}

VkDescriptorSet Graphic::Vulkan_Descriptor::get_uniform_descriptor_set(uint32_t current_frame) {
    if (current_frame < 0 || current_frame > Vulkan_Constants::MAX_FRAMES_IN_FLIGHT) {
        throw std::runtime_error("Fail to get descriptor set uniform: invalid frame id!");
    }

    return _vk_descriptor_sets[current_frame];
}

VkDescriptorSet Graphic::Vulkan_Descriptor::get_sampler_descriptor_set(uint32_t current_frame, std::string texture_key) {
    if (current_frame < 0 || current_frame > Vulkan_Constants::MAX_FRAMES_IN_FLIGHT) {
        throw std::runtime_error("Fail to get descriptor set sampler: invalid frame id!");
    }

    int index_texture = key_to_index_generater.get_index(texture_key);
    if (index_texture <= -1) {
        throw std::runtime_error("Fail to get descriptor set sampler: not found texture key!");
    }

    int index = Vulkan_Constants::MAX_FRAMES_IN_FLIGHT + index_texture * Vulkan_Constants::MAX_FRAMES_IN_FLIGHT + current_frame;

    return _vk_descriptor_sets[index];
}

void Graphic::Vulkan_Descriptor::destroy() {
    for (const auto& vk_descriptor_set_layout : _vk_descriptor_set_layouts) {
        vkDestroyDescriptorSetLayout(_vk_device, vk_descriptor_set_layout, nullptr);
    }
    Utility::Log::get()->log_info("Destroy descriptor set layout success!");

    vkDestroyDescriptorPool(_vk_device, _vk_descriptor_pool, nullptr);
    Utility::Log::get()->log_info("Destroy descriptor pool success!");
}

/**
 * Builder field
 */

Graphic::Vulkan_Descriptor_Builder& Graphic::Vulkan_Descriptor_Builder::add_descriptor_layout_type(VkDescriptorType vk_descriptor_type) {
    _vk_descriptor_types.push_back(vk_descriptor_type);
    return *this;
}


Graphic::Vulkan_Descriptor_Builder& Graphic::Vulkan_Descriptor_Builder::add_descriptor_shader_flag(VkShaderStageFlags vk_descriptor_shader_flag){
    _vk_descriptor_shader_flags.push_back(vk_descriptor_shader_flag);
    return *this;
}

Graphic::Vulkan_Descriptor_Builder& Graphic::Vulkan_Descriptor_Builder::add_uniform_buffer(std::vector<Vulkan_Buffer>* uniform_buffes) {
    _vk_uniform_buffes = uniform_buffes;
    return *this;
}

 Graphic::Vulkan_Descriptor* Graphic::Vulkan_Descriptor_Builder::build() {
    _instance = new Vulkan_Descriptor();
    _instance->set_descriptors_types(_vk_descriptor_types);
    _instance->set_descriptors_shader_flags(_vk_descriptor_shader_flags);
    _instance->set_vk_device(_vk_device);
    _instance->init_descriptor_set_layouts();
    _instance->init_descriptor_pool();
    _instance->update_uniform_descriptor(*_vk_uniform_buffes);
    return _instance;
 }