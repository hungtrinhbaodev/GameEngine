#pragma once
#include <limits>
#include <stdint.h>
#include <string>

namespace Const {
	const inline uint32_t MAX_UINT32 = std::numeric_limits<uint32_t>::max();

	const inline uint32_t MAX_DEPTH_CLIPPING = 3;
} // namespace Const