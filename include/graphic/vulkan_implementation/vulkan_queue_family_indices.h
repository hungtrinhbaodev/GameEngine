#pragma once
#include <optional>

namespace Graphic {

    class Vulkan_Queue_Family_Indices {

        public:

        std::optional<uint32_t> graphic_family;

        std::optional<uint32_t> present_family;

        bool is_complete();
    };
}