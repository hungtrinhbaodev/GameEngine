#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <map>

#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_builder.h>
#include <utility/log_utils.h>
#include <utility/file_utils.h>

namespace Graphic {
    
    struct Vulkan_Pipeline_Config {

        std::string path_vert_shader;

        std::string path_frag_shader;

        std::vector<VkVertexInputBindingDescription> vertex_descriptions;

        std::vector<VkVertexInputAttributeDescription> vertex_attributes;

        VkDescriptorSetLayout descriptor_set_layout;
    };

    class Vulkan_Pipeline {

        private:

        VkPipeline _vk_pipeline = VK_NULL_HANDLE;

        VkPipelineLayout _vk_pipeline_layout = VK_NULL_HANDLE;

        VkShaderModule _create_shader_module(
            VkDevice vk_device,
            const std::vector<char>& code
        );

        VkPipelineShaderStageCreateInfo _create_stage_pipeline(
            const VkShaderStageFlagBits& stage, 
            VkShaderModule& module, 
            const char* entry
        );

        VkPipelineVertexInputStateCreateInfo _create_vertex_input_info(
            const Vulkan_Pipeline_Config& pipeline_config
        );

        VkPipelineInputAssemblyStateCreateInfo _create_input_assembly_info();

        VkPipelineViewportStateCreateInfo _create_viewport_info();

        VkPipelineRasterizationStateCreateInfo _create_rasterize_info();

        VkPipelineMultisampleStateCreateInfo _create_multi_sampling_info();

        VkPipelineColorBlendAttachmentState _create_color_blend_attachment_info();

        VkPipelineColorBlendStateCreateInfo _create_color_blending_info(VkPipelineColorBlendAttachmentState &attachment_color);

        VkPipelineDynamicStateCreateInfo _create_dynamic_state_info();

        VkPipelineLayoutCreateInfo _create_pipeline_layout_info(const Vulkan_Pipeline_Config& pipeline_config);

        public:

        void init(
            VkDevice vk_device,
            VkRenderPass vk_render_pass,
            const Vulkan_Pipeline_Config& pipeline_config
        );

        void destroy(VkDevice vk_device);
    };

    class Vulkan_Pipeline_Builder : public Vulkan_Builder<Vulkan_Pipeline_Builder> {

        private:

        Vulkan_Pipeline_Config _pipeline_config;

        Vulkan_Pipeline* _instance;

        public:

        Vulkan_Pipeline_Builder() : Vulkan_Builder<Vulkan_Pipeline_Builder>() {}

        Vulkan_Pipeline_Builder& add_pipeline_config(const Vulkan_Pipeline_Config& pipeline_config);

        Vulkan_Pipeline* build();
    };
}