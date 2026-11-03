#pragma once
#include <consts.h>
#include <glm/glm.hpp>
#include <handle.h>

namespace Engine {

	enum DESCRIPTION_TYPE {
		NODE_2D,
	};

	struct Transform_2D {
		glm::vec2 position{0.f};
		glm::vec2 scale{1.f};
		float rotation = 0.f;
		float opacity = 1.f;
	};

	struct Parent {
		Handle value = Handle::make_invalid_handle();
	};

	struct Children {
		std::vector<Handle> value;
	};

	struct Description {
		DESCRIPTION_TYPE value;
	};

} // namespace Engine