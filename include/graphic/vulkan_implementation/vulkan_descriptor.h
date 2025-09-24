#include <vulkan/vulkan.h>

#include <graphic/vulkan_implementation/vulkan_builder.h>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_buffer.h>

namespace Graphic {

    class Vulkan_Descriptor {
        
        private:
        
        std::vector<VkDescriptorType> _vk_descriptor_types;

        std::vector<VkShaderStageFlags> _vk_descriptor_shader_flags;

        VkDevice _vk_device;

        VkDescriptorSetLayout _vk_descriptor_set_layout;

        VkDescriptorPool _vk_descriptor_pool;

        std::vector<VkDescriptorSet> _vk_descriptor_sets;

        public:

        void set_descriptors_types(const std::vector<VkDescriptorType>& descriptor_types);

        void set_descriptors_shader_flags(const std::vector<VkShaderStageFlags>& descriptor_shader_flags);

        void set_vk_device(VkDevice vk_device);

        VkDescriptorSetLayoutBinding make_descriptor_set_layout_binding(
            uint32_t binding, 
            VkDescriptorType vk_descriptor_type,
            VkShaderStageFlags vk_stageFlags
        );

        void init_descriptor_set_layout();

        void init_descriptor_pool();

        VkDescriptorSetLayout get_descriptor_set_layout();

        void update_uniform_descriptor(
            std::vector<Vulkan_Buffer>& uniform_buffers
        );

        void update_texture_descriptor(

        );

        void destroy();

    };

    class Vulkan_Descriptor_Builder : public Vulkan_Builder<Vulkan_Descriptor_Builder> {

        private:

        std::vector<VkDescriptorType> _vk_descriptor_types;

        std::vector<VkShaderStageFlags> _vk_descriptor_shader_flags;

        std::vector<Vulkan_Buffer>* _vk_uniform_buffes;

        Vulkan_Descriptor* _instance;

        public:

        Vulkan_Descriptor_Builder() : Vulkan_Builder<Vulkan_Descriptor_Builder>() {}

        Vulkan_Descriptor_Builder& add_descriptor_layout_type(VkDescriptorType vk_descriptor_type);

        Vulkan_Descriptor_Builder& add_descriptor_shader_flag(VkShaderStageFlags vk_descriptor_shader_flag);

        Vulkan_Descriptor_Builder& add_uniform_buffer(std::vector<Vulkan_Buffer>* uniform_buffes);

        Vulkan_Descriptor* build();
    };

}