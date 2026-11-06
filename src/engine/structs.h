#pragma once
#include <handle.h>

namespace Engine {
	struct Size_Pinning_Data {
		float left = 0.f, right = 0.f, top = 0.f, bottom = 0.f;
	};

	struct Handle_By_Tree_Depth {
		Handle handle;
		uint32_t tree_depth = 0;
		bool operator<(const Handle_By_Tree_Depth& other) { return tree_depth < other.tree_depth; }
	};
} // namespace Engine