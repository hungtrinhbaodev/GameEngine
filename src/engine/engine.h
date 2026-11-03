#pragma once
#include <handle.h>

namespace Engine {

	Handle create_node();

	void add_child(const Handle& e_parent, const Handle& e_children);

	void remove_child(const Handle& e_child);

	void clean_up(const Handle& e);

} // namespace Engine