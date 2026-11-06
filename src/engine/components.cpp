#include <engine/components.h>

namespace Engine {

	bool Size_Pinning::is_changing(const Size_Pinning_Data& pinning) const {
		return left != pinning.left && right != pinning.right && top != pinning.top && bottom != pinning.bottom;
	}

	void Size_Pinning::update(const Size_Pinning_Data& pinning) {
		top = pinning.top;
		bottom = pinning.bottom;
		left = pinning.left;
		right = pinning.right;
	}

	Size_Pinning_Data Size_Pinning::get_size_pinning() const {
		Size_Pinning_Data data{};
		data.top = top;
		data.bottom = bottom;
		data.left = left;
		data.right = right;
		return data;
	}

	void Dirty_Flag::add_flag(uint64_t flag) {
		value |= flag;
	}

	void Dirty_Flag::clean_flag(uint64_t flag) {
		value &= ~flag;
	}

	bool Dirty_Flag::is_dirty_at(uint64_t flag) const {
		return value & flag;
	}

	void Clipping_Ancestor_Information::add_clipping_ancestor(const Handle& ancestor) {
		if (::Const::MAX_DEPTH_CLIPPING <= 0)
			return;
		bool need_remove = false;
		for (int i = 0; i < number_clipping; i++) {
			if (clipping_ancesors[i].id() == ancestor.id()) {
				need_remove = true;
				break;
			}
		}
		if (need_remove) {
			remove_clipping_ancestor(ancestor);
		}
		if (number_clipping >= ::Const::MAX_DEPTH_CLIPPING) {
			remove_clipping_ancestor(clipping_ancesors[0]);
		}
		clipping_ancesors[number_clipping++] = ancestor;
	}

	void Clipping_Ancestor_Information::remove_clipping_ancestor(const Handle& ancestor) {
		if (::Const::MAX_DEPTH_CLIPPING <= 0)
			return;
		int remove_index = -1;
		for (int i = 0; i < ::Const::MAX_DEPTH_CLIPPING; i++) {
			if (clipping_ancesors[i].id() == ancestor.id()) {
				remove_index = i;
				break;
			}
		}
		if (remove_index < 0)
			return;
		for (int i = remove_index + 1; i < ::Const::MAX_DEPTH_CLIPPING; i++) {
			clipping_ancesors[i - 1] = clipping_ancesors[i];
		}
		number_clipping--;
	}

} // namespace Engine