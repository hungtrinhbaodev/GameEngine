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

void Graphic::Vulkan_Descriptor::init_descriptor_set_layout() {
    std::vector<VkDescriptorSetLayoutBinding> descriptor_bindings(_vk_descriptor_types.size());
    for (uint32_t i = 0; i < _vk_descriptor_types.size(); i++) {
        descriptor_bindings[i] = make_descriptor_set_layout_binding(
            i,
            _vk_descriptor_types[i],
            _vk_descriptor_shader_flags[i]
        );
    }

    VkDescriptorSetLayoutCreateInfo descriptor_info{};
    descriptor_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_info.bindingCount = descriptor_bindings.size();
    descriptor_info.pBindings = descriptor_bindings.data();

    Vulkan_Utility::vk_check_action(
        vkCreateDescriptorSetLayout(_vk_device, &descriptor_info, nullptr, &_vk_descriptor_set_layout),
        "fail to create descriptor layout set!"
    );
    Utility::Log::get()->log_info("Crate descriptor layout set successfully!");
}

void Graphic::Vulkan_Descriptor::init_descriptor_pool() {

    /**
     * Make descriptor pool
     */
    std::vector<VkDescriptorPoolSize> pool_sizes(_vk_descriptor_types.size());
    for (int i = 0; i < _vk_descriptor_types.size(); i++) {
        pool_sizes[i].descriptorCount = static_cast<uint32_t>(Vulkan_Constants::MAX_FRAMES_IN_FLIGHT);
        pool_sizes[i].type = _vk_descriptor_types[i];
    }

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = static_cast<uint32_t>(Vulkan_Constants::MAX_FRAMES_IN_FLIGHT);

    Vulkan_Utility::vk_check_action(
        vkCreateDescriptorPool(_vk_device, &pool_info, nullptr, &_vk_descriptor_pool),
        "fail to create pool descriptor!"
    );
    Utility::Log::get()->log_info("create pool descriptor success!");

    /**
     * Make descriptor set in pool
     */
    std::vector<VkDescriptorSetLayout> layouts(size_t(Vulkan_Constants::MAX_FRAMES_IN_FLIGHT), _vk_descriptor_set_layout);
    VkDescriptorSetAllocateInfo allocator_info{};
    allocator_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocator_info.descriptorPool = _vk_descriptor_pool;
    allocator_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocator_info.pSetLayouts = layouts.data();

    _vk_descriptor_sets.resize(Vulkan_Constants::MAX_FRAMES_IN_FLIGHT);
    Vulkan_Utility::vk_check_action(
        vkAllocateDescriptorSets(_vk_device, &allocator_info, _vk_descriptor_sets.data()),
        "fail to allocate descriptor sets"
    );
    Utility::Log::get()->log_info("allocate descriptor sets success!");
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

VkDescriptorSetLayout Graphic::Vulkan_Descriptor::get_descriptor_set_layout() {
    return _vk_descriptor_set_layout;
}

void Graphic::Vulkan_Descriptor::destroy() {
    vkDestroyDescriptorSetLayout(_vk_device, _vk_descriptor_set_layout, nullptr);
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
    _instance->init_descriptor_set_layout();
    _instance->init_descriptor_pool();
    _instance->update_uniform_descriptor(*_vk_uniform_buffes);
    return _instance;
 }