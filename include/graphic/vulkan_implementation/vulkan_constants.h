#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

#include <vector>

namespace Graphic {

    enum Vulkan_Draw_ID {

        OBJECT_DEFAULT = 0, // draw 2D, 3D object without texture

        OBJECT_WITH_TEXTURE = 1 // draw 2D, 3D object with texture

    };

    enum Vulkan_Commands_Mode {

        COMMANDS_MODE_ASYNC, // don't block cpu when do task is submited

        COMMANDS_MODE_SYNC // block cpu until task is finish 

    };

    enum Vulkan_Queue_Submit_Mode {

        SUBMIT_MODE_ASYNC, // don't block cpu when do task is submited

        SUBMIT_MODE_SYNC // block cpu until task is finish 

    };

    class Vulkan_Constants {

        public:

        static bool IS_ENABLED_VALIDATION_LAYER;

        static const char* VALIDATION_LAYER_NAME;

        static std::vector<const char*> INSTANCE_EXTENSION;

        static std::vector<const char*> REQUIRED_PHYSICAL_DEVICE_EXTENSIONS;
        
        static int MAX_FRAMES_IN_FLIGHT;

        static std::vector<VkDynamicState> PIPELINE_DYNAMIC_STATES;

        static std::string DEFAULT_PATH_SHADER_DRAW_DEFAULT;

        static std::string DEFAULT_PATH_SHADER_DRAW_WITH_TEXTURE;
    };

}