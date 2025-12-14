#pragma once
#include <optional>

namespace Vulkan {

	struct Queue_Family_Indices {

    public:

        std::optional<uint32_t> graphic_family;

        std::optional<uint32_t> present_family;

        bool is_complete() const;

    };

    inline bool Queue_Family_Indices::is_complete() const {
        return graphic_family.has_value() && present_family.has_value();
    }
}