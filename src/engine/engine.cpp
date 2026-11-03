#include <ecs.h>
#include <engine/components.h>
#include <engine/engine.h>
#include <functional>
#include <id_generator.h>

namespace Engine {
	Registry registry{};

	Handle_Generator entity_generator{};

	void assert_handle(const Handle& handle) {
		if (!entity_generator.validate_handle(handle)) {
			throw std::runtime_error("Fail to handle handle in engine!");
		}
	}

	bool travel(
		const Handle& root, const std::function<bool(const Handle& parent, const Handle& child)>& fn,
		const Handle& parent = Handle::make_invalid_handle()
	) {
		if (!registry.has<Children>(root.id())) {
			fn(parent, root);
			return false;
		}
		bool end = false;
		if (parent.is_invalid()) {
			end = fn(parent, root);
		}
		const auto& [children] = registry.read<Children>(root.id());
		for (int i = 0; i < children.value.size(); i++) {
			if (end) {
				break;
			}
			const Handle& child = children.value.at(i);
			end = fn(root, child);
			if (!end) {
				end = travel(child, fn, root);
			}
		}
		return end;
	}

	void remove_node(const Handle& e) {
		if (!registry.has<Description>(e.id())) {
			return;
		}
		const auto& [description] = registry.read<Description>(e.id());
		if (description.value != DESCRIPTION_TYPE::NODE_2D) {
			return;
		}
		registry.erase<Description>(e.id());
		registry.erase<Transform_2D>(e.id());
		registry.erase<Children>(e.id());
		registry.erase<Parent>(e.id());
	}

	Handle create_node() {
		Handle entity = entity_generator.gen_handle();
		registry.add(entity.id(), Description{DESCRIPTION_TYPE::NODE_2D});
		registry.add(entity.id(), Transform_2D{});
		registry.add(entity.id(), Children{});
		registry.add(entity.id(), Parent{});
		return entity;
	}

	void add_child(const Handle& e_parent, const Handle& e_child) {
		assert_handle(e_parent);
		assert_handle(e_child);
		if (!registry.has<Children>(e_parent.id())) {
			return;
		}
		if (!registry.has<Parent>(e_child.id())) {
			return;
		}
		bool is_recurse = false;
		travel(e_child, [&is_recurse, &e_parent](const Handle& parent, const Handle& child) {
			if (child.id() == e_parent.id()) {
				is_recurse = true;
			}
			return is_recurse;
		});
		if (is_recurse) {
			return;
		}
		auto [parent] = registry.get<Parent>(e_child.id());
		if (!parent.value.is_invalid()) {
			remove_child(e_child);
		}
		parent.value = e_parent;
		auto [children] = registry.get<Children>(e_parent.id());
		children.value.push_back(e_child);
	}

	void remove_child(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Parent>(e.id()))
			return;
		auto [parent] = registry.get<Parent>(e.id());
		/**
		 * @Note root don't have parent we don't need remove it.
		 */
		if (parent.value.is_invalid()) {
			return;
		}
		auto [parent_children] = registry.get<Children>(parent.value.id());
		auto& children = parent_children.value;
		for (int i = 0; i < children.size(); i++) {
			if (children[i].id() == e.id()) {
				children.erase(children.begin() + i);
				i--;
			}
		}
		parent.value = Handle::make_invalid_handle();
	}

	void clean_up(const Handle& e) {
		assert_handle(e);
		remove_child(e);
		std::vector<Handle> nodes;
		travel(e, [&nodes](const Handle& parent, const Handle& child) {
			nodes.push_back(child);
			return false;
		});
		for (int i = nodes.size() - 1; i > -1; i--) {
			const auto& node = nodes[i];
			if (!registry.has<Description>(node.id())) {
				entity_generator.release_handle(node);
				continue;
			}
			const auto& [description] = registry.read<Description>(node.id());
			switch (description.value) {
				case DESCRIPTION_TYPE::NODE_2D: {
					remove_node(node);
					break;
				}
				default: {
					break;
				}
			}
			entity_generator.release_handle(node);
		}
	}

} // namespace Engine