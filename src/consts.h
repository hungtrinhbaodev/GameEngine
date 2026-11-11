#pragma once
#include <limits>
#include <stdint.h>
#include <string>

namespace Const {
	const inline uint32_t MAX_UINT32 = std::numeric_limits<uint32_t>::max();

	const inline uint32_t MAX_DEPTH_CLIPPING = 3;

	const inline float MAX_FLOAT = std::numeric_limits<float>::max();

	const inline float EPSILON = 0.0000001f;

	enum COLLISION_STATE {
		COLLISION_STATE_FULL_OUT,
		COLLISION_STATE_FULL_IN,
		COLLISION_STATE_INTERSECT
	};
} // namespace Const