#include <core.h>
#include <engine/components.h>
#include <engine/engine.h>
#include <engine/scroll_handler.h>
#include <engine/systems.h>
#include <engine/touch_handler.h>
#include <functional>
#include <id_generator.h>
#include <math_custom.h>
#include <sparse_set.h>
#include <stack>
#include <vulkan/draws/vk_draw_texture_2D.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_utils.h>

namespace Engine {

	enum TRAVEL_NODE_MODE {
		TRAVEL_UP,
		TRAVEL_DOWN,
		TRAVEL_ALL,
		TRAVEL_NONE,
	};

	enum PICKED_NODE_MODE {
		JUST_DIRTY,
		PICKED_ALL,
	};

	Registry registry{};

	Handle_Generator entity_generator{};

	Handle root = Handle::make_invalid_handle();

	Sparse_Set<Handle> dirty_entities{};

	std::stack<Handle> travel_stack{};

	/**
	 * @Note: local variable has name start with processing
	 * will be used in case system handle all dirty
	 * in tree graph.
	 */
	std::vector<Handle> processing_traveled_entities{};

	std::vector<Handle> processing_dirty_parent_entities{};

	std::vector<Handle> processing_dirty_entities{};

	Sparse_Set<bool> processing_visited_entities{};

	std::vector<Handle> entities_touch_listener{};

	std::vector<Handle> removed_enitites_touch_listener{};

	std::vector<Handle> enitites_scroll_listener{};

	std::vector<Handle> removed_entites_scroll_listener{};

	std::string get_entity_description_name(const Handle& e) {
		if (!registry.has<Description>(e.id())) {
			return "UNDEFINED";
		}
		const auto& [description] = registry.read<Description>(e.id());
		switch (description.value) {
			case DESCRIPTION_TYPE::NODE_2D: {
				return "NODE_2D";
			}
			case DESCRIPTION_TYPE::LAYOUT_2D: {
				return "LAYOUT_2D";
			}
			case DESCRIPTION_TYPE::LABEL_2D: {
				return "LABEL_2D";
			}
			case DESCRIPTION_TYPE::IMAGE_2D: {
				return "IMAGE_2D";
			}
			default: {
				return "UNDEFINED";
			}
		}
	}

	void assert_handle(const Handle& handle) {
		if (!entity_generator.validate_handle(handle)) {
			throw std::runtime_error("Fail to handle handle in engine!");
		}
	}

	void travel_down(const Handle& root, const std::function<bool(const Handle& parent, const Handle& child)>& fn) {
		if (!registry.has<Tree_Information>(root.id())) {
			fn(Handle::make_invalid_handle(), root);
			return;
		}
		while (travel_stack.size())
			travel_stack.pop();
		travel_stack.push(root);
		while (travel_stack.size()) {
			Handle current = travel_stack.top();
			travel_stack.pop();
			const auto& [tree_info] = registry.read<Tree_Information>(current.id());
			if (fn(tree_info.parent, current))
				continue;
			for (int i = tree_info.children.size() - 1; i > -1; i--) {
				const auto& child = tree_info.children[i];
				travel_stack.push(child);
			}
		}
	}

	void travel_up(Handle child, const std::function<bool(const Handle& parent, const Handle& child)>& fn) {
		if (!registry.has<Tree_Information>(child.id())) {
			return;
		}
		const auto& [tree_info] = registry.read<Tree_Information>(child.id());
		Handle parent = tree_info.parent;
		if (parent.is_invalid()) {
			fn(parent, child);
			return;
		}
		while (!parent.is_invalid() && registry.has<Tree_Information>(parent.id())) {
			fn(parent, child);
			child = parent;
			const auto& [tree_info] = registry.read<Tree_Information>(child.id());
			parent = tree_info.parent;
		}
	}

	void remove_node(const Handle& e) {
		registry.erase<Description>(e.id());
		registry.erase<Transform_2D>(e.id());
		registry.erase<Clipping_Ancestor_Information>(e.id());
		registry.erase<Position_Percent>(e.id());
		registry.erase<Dirty_Flag>(e.id());
		registry.erase<Tree_Information>(e.id());
		registry.erase<World_Transform_2D>(e.id());
	}

	void remove_layout(const Handle& e) {
		remove_node(e);
		const auto& [handles] = registry.read<Renderer_Handles>(e.id());
		for (const auto& handle : handles.value) {
			Vulkan::remove_draw(handle);
		}
		registry.erase<Layout_Renderer_Information>(e.id());
		registry.erase<Renderer_Handles>(e.id());
		registry.erase<Size_Pinning>(e.id());
		registry.erase<Size_Percent>(e.id());
		registry.erase<Touching_Information>(e.id());
	}

	void remove_scroll_layout(const Handle& e) {
		remove_node(e);
		remove_layout(e);
		registry.erase<Scroll_Information>(e.id());
	}

	void remove_image(const Handle& e) {
		remove_node(e);
		const auto& [handles] = registry.read<Renderer_Handles>(e.id());
		for (const auto& handle : handles.value) {
			Vulkan::remove_draw(handle);
		}
		const auto& [nine_scale_handles] = registry.read<Nine_Scale_Renderer_Handles>(e.id());
		for (const auto& handle : nine_scale_handles.value) {
			Vulkan::remove_draw(handle);
		}
		registry.erase<Size>(e.id());
		registry.erase<Image_Renderer_Information>(e.id());
		registry.erase<Renderer_Handles>(e.id());
		registry.erase<Nine_Scale_Information>(e.id());
		registry.erase<Nine_Scale_Renderer_Handles>(e.id());
		registry.erase<Inner_Renderer_Size>(e.id());
		registry.erase<Touching_Information>(e.id());
		registry.erase<Size_Pinning>(e.id());
		registry.erase<Size_Percent>(e.id());
	}

	void remove_label(const Handle& e) {
		remove_node(e);
		const auto& [handles] = registry.read<Renderer_Handles>(e.id());
		for (const auto& handle : handles.value) {
			Vulkan::remove_draw(handle);
		}
		registry.erase<Size>(e.id());
		registry.erase<Image_Renderer_Information>(e.id());
		registry.erase<Renderer_Handles>(e.id());
		registry.erase<Inner_Renderer_Size>(e.id());
		registry.erase<Touching_Information>(e.id());
	}

	void clean_size_components(const Handle& e) {
		if (registry.has<Size_Percent>(e.id())) {
			auto [size_percent] = registry.get<Size_Percent>(e.id());
			size_percent.enabled = false;
		}
		if (registry.has<Size_Pinning>(e.id())) {
			auto [size_pinning] = registry.get<Size_Pinning>(e.id());
			size_pinning.enabled = false;
		}
	}

	void set_dirty(const Handle& e, bool dirty, uint64_t flag) {
		if (!registry.has_all<Dirty_Flag, Tree_Information>(e.id()))
			return;
		if (dirty) {
			const auto& [validation] = registry.read<Tree_Information>(e.id());
			auto [dirty_flag] = registry.get<Dirty_Flag>(e.id());
			dirty_flag.add_flag(flag);
			if (validation.valid_child) {
				dirty_entities.insert(e.id(), e);
			}
		}
	}

	void clean_dirty(const Handle& e, uint64_t flag) {
		if (!registry.has<Dirty_Flag>(e.id()))
			return;
		auto [dirty_flag] = registry.get<Dirty_Flag>(e.id());
		dirty_flag.clean_flag(flag);
		if (dirty_flag.value == 0) {
			dirty_entities.erase(e.id());
		}
	}

	bool is_dirty(const Handle& e, uint64_t flag) {
		if (!registry.has<Dirty_Flag>(e.id()))
			return false;
		const auto& [dirty_flag] = registry.read<Dirty_Flag>(e.id());
		return dirty_flag.is_dirty_at(flag);
	}

	void make_processing_dirty_entities(
		uint64_t dirty_flag, TRAVEL_NODE_MODE travel_mode, PICKED_NODE_MODE picked_mode = PICKED_NODE_MODE::PICKED_ALL
	) {
		processing_dirty_entities.clear();
		processing_traveled_entities.clear();
		auto node_greater = [](const Handle& a, const Handle& b) {
			const auto& [tree_info_a] = registry.get<Tree_Information>(a.id());
			const auto& [tree_info_b] = registry.get<Tree_Information>(b.id());
			return tree_info_a.depth < tree_info_b.depth;
		};
		auto push_item_by_picked_mode = [](uint64_t dirty_flag, PICKED_NODE_MODE picked_mode,
										   std::vector<Handle>& processing_traveled_entities, const Handle& child) {
			switch (picked_mode) {
				case PICKED_NODE_MODE::JUST_DIRTY: {
					if (is_dirty(child, dirty_flag)) {
						processing_traveled_entities.push_back(child);
					}
					break;
				}
				default: {
					processing_traveled_entities.push_back(child);
					break;
				}
			}
			clean_dirty(child, dirty_flag);
		};
		for (const auto& dirty_entity : dirty_entities.values()) {
			if (is_dirty(dirty_entity, dirty_flag)) {
				processing_dirty_entities.push_back(dirty_entity);
			}
		}
		switch (travel_mode) {
			case TRAVEL_NODE_MODE::TRAVEL_NONE: {
				for (const auto& dirty_entity : processing_dirty_entities) {
					processing_traveled_entities.push_back(dirty_entity);
					clean_dirty(dirty_entity, dirty_flag);
				}
				break;
			}
			case TRAVEL_NODE_MODE::TRAVEL_DOWN: {
				std::sort(processing_dirty_entities.begin(), processing_dirty_entities.end(), node_greater);
				for (const auto& dirty_entity : processing_dirty_entities) {
					if (!is_dirty(dirty_entity, dirty_flag))
						continue;
					travel_down(
						dirty_entity,
						[&dirty_flag, &picked_mode, &push_item_by_picked_mode](const Handle&, const Handle& child) {
							push_item_by_picked_mode(dirty_flag, picked_mode, processing_traveled_entities, child);
							return false;
						}
					);
				}
				break;
			}
			case TRAVEL_NODE_MODE::TRAVEL_UP: {
				std::sort(processing_dirty_entities.begin(), processing_dirty_entities.end(), node_greater);
				std::reverse(processing_dirty_entities.begin(), processing_dirty_entities.end());
				for (const auto& dirty_entity : processing_dirty_entities) {
					if (!is_dirty(dirty_entity, dirty_flag))
						continue;
					travel_up(
						dirty_entity,
						[&dirty_flag, &picked_mode, &push_item_by_picked_mode](const Handle&, const Handle& child) {
							push_item_by_picked_mode(dirty_flag, picked_mode, processing_traveled_entities, child);
							return false;
						}
					);
				}
				std::reverse(processing_traveled_entities.begin(), processing_traveled_entities.end());
				break;
			}
			default: {
				std::sort(processing_dirty_entities.begin(), processing_dirty_entities.end(), node_greater);
				std::reverse(processing_dirty_entities.begin(), processing_dirty_entities.end());
				processing_dirty_parent_entities.clear();
				for (const auto& dirty_entity : processing_dirty_entities) {
					if (!registry.has<Tree_Information>(dirty_entity.id()))
						continue;
					const auto& [tree_info] = registry.read<Tree_Information>(dirty_entity.id());
					if (!tree_info.parent.is_invalid()) {
						processing_dirty_parent_entities.push_back(tree_info.parent);
					}
				}
				for (const auto& dirty_entity : processing_dirty_parent_entities) {
					if (!is_dirty(dirty_entity, dirty_flag))
						continue;
					travel_up(
						dirty_entity,
						[&dirty_flag, &picked_mode, &push_item_by_picked_mode](const Handle&, const Handle& child) {
							push_item_by_picked_mode(dirty_flag, picked_mode, processing_traveled_entities, child);
							return false;
						}
					);
				}
				std::reverse(processing_traveled_entities.begin(), processing_traveled_entities.end());
				for (const auto& dirty_entity : processing_dirty_entities) {
					if (!is_dirty(dirty_entity, dirty_flag))
						continue;
					travel_down(
						dirty_entity,
						[&dirty_flag, &picked_mode, &push_item_by_picked_mode](const Handle&, const Handle& child) {
							push_item_by_picked_mode(dirty_flag, picked_mode, processing_traveled_entities, child);
							return false;
						}
					);
				}
				break;
			}
		}
	}

	void resovle_relative_size(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id()))
			return;
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		if (!tree_info.valid_child)
			return;
		processing_traveled_entities.clear();
		travel_up(e, [](const Handle&, const Handle& children) {
			if (is_dirty(children, DIRTY_RELATIVE_SIZE)) {
				processing_traveled_entities.push_back(children);
			}
			return false;
		});
		std::reverse(processing_traveled_entities.begin(), processing_traveled_entities.end());
		handle_dirty_relative_size(processing_traveled_entities);
	}

	void resovle_relative_position(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id()))
			return;
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		if (!tree_info.valid_child)
			return;
		resovle_relative_size(e);
		travel_up(e, [](const Handle&, const Handle& children) {
			if (is_dirty(children, DIRTY_RELATIVE_POSITION)) {
				processing_traveled_entities.push_back(children);
			}
			return false;
		});
		std::reverse(processing_traveled_entities.begin(), processing_traveled_entities.end());
		handle_dirty_relative_position(processing_traveled_entities);
	}

	void update_tree_changing_z_order() {
		processing_traveled_entities.clear();
		travel_down(root, [](const Handle&, const Handle& child) {
			processing_traveled_entities.push_back(child);
			return false;
		});
		for (uint32_t i = 0; i < processing_traveled_entities.size(); i++) {
			const auto& e = processing_traveled_entities[i];
			auto [transform] = registry.get<Transform_2D>(e.id());
			transform.draw_index = i;
			if (registry.has<Touching_Information>(e.id())) {
				auto [touch_info] = registry.get<Touching_Information>(e.id());
				touch_info.touched_order = i;
				set_dirty(e, true, DIRTY_TOUCHING);
			}
		}
	}

	void make_processing_enitites_touched() {
		processing_traveled_entities.clear();
		const auto& handled_touches = get_handled_touches();
		for (const auto& touch_info : handled_touches) {
			const auto& entities = touch_info.entities;
			for (const auto& e : entities) {
				if (!registry.has<Touching_Information>(e.id()))
					continue;
				auto [c_touch_info] = registry.get<Touching_Information>(e.id());
				c_touch_info.touch_state = touch_info.touch_state;
				c_touch_info.touch_world_position = touch_info.touch_world_position;
				processing_traveled_entities.push_back(e);
			}
		}
	}

	void sort_touch_listener_entities() {
		std::sort(entities_touch_listener.begin(), entities_touch_listener.end(), [](const Handle& a, const Handle& b) {
			const auto& [touch_info_a] = registry.read<Touching_Information>(a.id());
			const auto& [touch_info_b] = registry.read<Touching_Information>(b.id());
			return touch_info_a.touched_order > touch_info_b.touched_order;
		});
	}

	void remove_touch_listener_entities(const Handle& e) {
		for (int i = 0; i < entities_touch_listener.size(); i++) {
			if (entities_touch_listener[i].id() == e.id()) {
				entities_touch_listener[i] = entities_touch_listener.back();
				entities_touch_listener.pop_back();
				i--;
			}
		}
	}

	void add_touch_listener_entities(const Handle& e) {
		bool found = false;
		for (int i = 0; i < entities_touch_listener.size(); i++) {
			if (entities_touch_listener[i].id() == e.id()) {
				found = true;
				break;
			}
		}
		if (!found) {
			entities_touch_listener.push_back(e);
		}
	}

	void clean_touch_removed_child_listener_entities(const std::vector<Handle>& entities) {
		for (const auto& e : entities) {
			if (!registry.has<Touching_Information>(e.id()))
				continue;
			remove_touch_listener_entities(e);
		}
		sort_touch_listener_entities();
		discard_handling_touches_entities(entities);
	}

	void handle_update_entities_touch_listener(const std::vector<Handle>& dirty_entities) {
		removed_enitites_touch_listener.clear();
		for (const auto& e : dirty_entities) {
			if (!registry.has<Touching_Information>(e.id()))
				continue;
			const auto& [touch_info] = registry.read<Touching_Information>(e.id());
			if (!touch_info.enabled) {
				remove_touch_listener_entities(e);
				removed_enitites_touch_listener.push_back(e);
			} else {
				add_touch_listener_entities(e);
			}
		}
		sort_touch_listener_entities();
		discard_handling_touches_entities(removed_enitites_touch_listener);
	}

	void sort_scroll_listener_entities() {
		std::sort(
			enitites_scroll_listener.begin(), enitites_scroll_listener.end(), [](const Handle& a, const Handle& b) {
				const auto& [scroll_info_a] = registry.read<Scroll_Information>(a.id());
				const auto& [scroll_info_b] = registry.read<Scroll_Information>(b.id());
				return scroll_info_a.scroll_order > scroll_info_b.scroll_order;
			}
		);
	}

	void add_scroll_listener_entities(const Handle& e) {
		bool found = false;
		for (int i = 0; i < enitites_scroll_listener.size(); i++) {
			if (enitites_scroll_listener[i].id() == e.id()) {
				found = true;
				break;
			}
		}
		if (!found) {
			enitites_scroll_listener.push_back(e);
		}
	}

	void remove_scroll_listener_entities(const Handle& e) {
		for (int i = 0; i < enitites_scroll_listener.size(); i++) {
			if (enitites_scroll_listener[i].id() == e.id()) {
				enitites_scroll_listener[i] = enitites_scroll_listener.back();
				enitites_scroll_listener.pop_back();
				i--;
			}
		}
	}

	void clean_scroll_removed_child_listener_entities(const std::vector<Handle>& entities) {
		for (const auto& e : entities) {
			if (!registry.has<Scroll_Information>(e.id()))
				continue;
			remove_scroll_listener_entities(e);
		}
		sort_scroll_listener_entities();
	}

	void handle_update_entities_scroll_listener(const std::vector<Handle>& dirty_entities) {
		for (const auto& e : dirty_entities) {
			if (!registry.has<Scroll_Information>(e.id()))
				continue;
			const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
			if (!scroll_info.enabled) {
				remove_scroll_listener_entities(e);
			} else {
				add_scroll_listener_entities(e);
			}
		}
		sort_touch_listener_entities();
	}

	void init_root() {
		root = create_layout();
		auto [tree_info] = registry.get<Tree_Information>(root.id());
		tree_info.valid_child = true;
		tree_info.depth = 0;
		set_content_size(root, get_window_size());
		set_layout_background_enabled(root, true);
		set_layout_background_color(root, {0.5f, 0.5f, 0.5f});
	}

	Handle create_node(DESCRIPTION_TYPE type) {
		Handle entity = entity_generator.gen_handle();
		registry.add(entity.id(), Description{type});
		registry.add(entity.id(), Transform_2D{});
		registry.add(entity.id(), Position_Percent{});
		registry.add(entity.id(), Dirty_Flag{});
		registry.add(entity.id(), Clipping_Ancestor_Information{});
		registry.add(entity.id(), Tree_Information{});
		registry.add(entity.id(), World_Transform_2D{});
		return entity;
	}

	void init(GLFWwindow* window) {
		Vulkan::init_vulkan(window, []() { set_content_size(root, get_window_size()); });
		init_touch_handler(window);
		init_scroll_handler(window);
		init_root();
	}

	void process_frame(double dt) {

		handle_touches_logic();
		make_processing_enitites_touched();
		handle_enities_touched(processing_traveled_entities);

		if (has_scrolling()) {
				}

		make_processing_dirty_entities(DIRTY_DRAW_ORDER, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_dirty_z_order(processing_traveled_entities);
		if (processing_traveled_entities.size() > 0) {
			update_tree_changing_z_order();
		}

		make_processing_dirty_entities(DIRTY_RENDERER, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_dirty_renderer(processing_traveled_entities);

		make_processing_dirty_entities(
			DIRTY_RELATIVE_SIZE, TRAVEL_NODE_MODE::TRAVEL_DOWN, PICKED_NODE_MODE::PICKED_ALL
		);
		handle_dirty_relative_size(processing_traveled_entities);

		make_processing_dirty_entities(
			DIRTY_RELATIVE_POSITION, TRAVEL_NODE_MODE::TRAVEL_DOWN, PICKED_NODE_MODE::PICKED_ALL
		);
		handle_dirty_relative_position(processing_traveled_entities);

		make_processing_dirty_entities(DIRTY_TRANSFORM, TRAVEL_NODE_MODE::TRAVEL_DOWN, PICKED_NODE_MODE::PICKED_ALL);
		handle_dirty_transform(processing_traveled_entities);

		make_processing_dirty_entities(DIRTY_CLIPPING, TRAVEL_NODE_MODE::TRAVEL_DOWN, PICKED_NODE_MODE::PICKED_ALL);
		handle_dirty_clipping(processing_traveled_entities);

		make_processing_dirty_entities(DIRTY_TOUCHING, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_update_entities_touch_listener(processing_traveled_entities);

		make_processing_dirty_entities(DIRTY_SCROLLING, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_update_entities_scroll_listener(processing_traveled_entities);

		dirty_entities.clear();
		Vulkan::draw_frame(dt);
	}

	void destroy() {
		Vulkan::destroy_vulkan();
	}

	glm::vec2 get_window_size() {
		return Vulkan::Utils::get_window_size(Vulkan::device);
	}

	Registry& get_registry() {
		return registry;
	}

	Handle create_node() {
		return create_node(DESCRIPTION_TYPE::NODE_2D);
	}

	Handle create_layout() {
		Handle layout = create_node(DESCRIPTION_TYPE::LAYOUT_2D);
		registry.add(layout.id(), Size{{0.f, 0.f}});
		registry.add(layout.id(), Layout_Renderer_Information{});
		registry.add(layout.id(), Renderer_Handles{});
		registry.add(layout.id(), Size_Pinning{});
		registry.add(layout.id(), Size_Percent{});
		registry.add(layout.id(), Clipping_Information{});
		registry.add(layout.id(), Touching_Information{});
		return layout;
	}

	Handle create_image() {
		Handle image = create_node(DESCRIPTION_TYPE::IMAGE_2D);
		registry.add(image.id(), Size{{0.f, 0.f}});
		registry.add(image.id(), Image_Renderer_Information{});
		registry.add(image.id(), Renderer_Handles{});
		registry.add(image.id(), Nine_Scale_Information{});
		registry.add(image.id(), Nine_Scale_Renderer_Handles{});
		registry.add(image.id(), Inner_Renderer_Size{});
		registry.add(image.id(), Touching_Information{});
		registry.add(image.id(), Size_Pinning{});
		registry.add(image.id(), Size_Percent{});
		return image;
	}

	Handle create_label() {
		Handle label = create_node(DESCRIPTION_TYPE::LABEL_2D);
		registry.add(label.id(), Size{{0.f, 0.f}});
		registry.add(label.id(), Label_Renderer_Information{});
		registry.add(label.id(), Renderer_Handles{});
		registry.add(label.id(), Inner_Renderer_Size{});
		registry.add(label.id(), Touching_Information{});
		return label;
	}

	Handle create_scroll_layout() {
		Handle inner_container = create_layout();
		Scroll_Information container{};
		container.container_handle = inner_container;
		Handle scroll_layout = create_layout();
		registry.add(scroll_layout.id(), container);
		add_child(scroll_layout, inner_container);
		set_touch_enabled(inner_container, true);
		set_touching_caller(
			inner_container, [](Handle inner_container, TOUCH_STATE touch_state, glm::vec2 world_position) {
				const auto& [tree_info] = registry.read<Tree_Information>(inner_container.id());
				auto [scroll_info] = registry.get<Scroll_Information>(tree_info.parent.id());
				switch (touch_state) {
					case TOUCH_STATE::TOUCH_BEGAN: {
						// TODO: handle stop all scroll action here!
						scroll_info.start_touch = world_position;
						break;
					}
					case TOUCH_STATE::TOUCH_MOVE: {
						glm::vec2 delta_position = world_position - scroll_info.start_touch;
						bool is_moving = false;
						glm::vec2 moving_distance{0.f, 0.f};
						switch (scroll_info.scroll_direction) {
							case SCROLL_DIRECTION::SCROLL_HORIZONTAL: {
								if (abs(delta_position.x) >= 7.5) {
									is_moving = true;
									moving_distance.x = delta_position.x;
								}
								break;
							}
							default: {
								if (abs(delta_position.y) >= 7.5) {
									is_moving = true;
									moving_distance.y = delta_position.y;
								}
								break;
							}
						}
						if (is_moving) {
							auto [transform] = registry.get<Transform_2D>(inner_container.id());
							transform.position += moving_distance;
							set_dirty(inner_container, true, DIRTY_TRANSFORM);
						}
						scroll_info.start_touch = world_position;
						break;
					}
					case TOUCH_STATE::TOUCH_ENDED: {
						// TODO: handle bounce here when has action!
						break;
					}
				}
			}
		);
		registry.add(scroll_layout.id(), Description{DESCRIPTION_TYPE::SCROLL_LAYOUT});
		registry.add(inner_container.id(), Description{DESCRIPTION_TYPE::INNER_CONTAINER});
		return scroll_layout;
	}

	void add_child(const Handle& e_parent, const Handle& e_child) {
		assert_handle(e_parent);
		assert_handle(e_child);
		if (!registry.has<Tree_Information>(e_parent.id())) {
			return;
		}
		if (!registry.has_all<Tree_Information, Dirty_Flag>(e_child.id())) {
			return;
		}
		bool is_recurse = false;
		/**
		 * @Note: before add child
		 * we need confirm that child
		 * don't contain it parent.
		 */
		travel_down(e_child, [&is_recurse, &e_parent](const Handle&, const Handle& child) {
			if (child.id() == e_parent.id()) {
				is_recurse = true;
			}
			return is_recurse;
		});
		if (is_recurse) {
			return;
		}
		auto [tree_info] = registry.get<Tree_Information>(e_child.id());
		auto [parent_tree_info] = registry.get<Tree_Information>(e_parent.id());
		if (!tree_info.parent.is_invalid()) {
			remove_child(e_child);
		}
		tree_info.parent = e_parent;
		tree_info.valid_child = parent_tree_info.valid_child;
		if (tree_info.valid_child) {
			uint64_t all_dirty = DIRTY_TRANSFORM | DIRTY_RENDERER;
			travel_down(e_child, [&all_dirty](const Handle& parent, const Handle& child) {
				if (!registry.has<Tree_Information>(parent.id()) ||
					!registry.has_all<Tree_Information, Dirty_Flag>(child.id())) {
					return true;
				}
				const auto& [parent_tree_info] = registry.read<Tree_Information>(parent.id());
				auto [child_tree_info] = registry.get<Tree_Information>(child.id());
				auto [dirty_flag] = registry.read<Dirty_Flag>(child.id());
				child_tree_info.valid_child = parent_tree_info.valid_child;
				if (registry.has<Touching_Information>(child.id())) {
					all_dirty |= DIRTY_TOUCHING;
				}
				if (registry.has<Scroll_Information>(child.id())) {
					all_dirty |= DIRTY_SCROLLING;
				}
				set_dirty(child, true, dirty_flag.value | all_dirty);
				child_tree_info.depth = parent_tree_info.depth + 1;
				return false;
			});
		}
		parent_tree_info.children.push_back(e_child);
		set_dirty(e_parent, true, DIRTY_DRAW_ORDER);
	}

	void remove_child(const Handle& e) {
		assert_handle(e);
		if (!registry.has_all<Tree_Information>(e.id()))
			return;
		auto [tree_info] = registry.get<Tree_Information>(e.id());
		/**
		 * @Note root don't have parent we don't need remove it.
		 */
		if (tree_info.parent.is_invalid()) {
			return;
		}
		std::vector<Handle> removed_children;
		auto [parent_tree_info] = registry.get<Tree_Information>(tree_info.parent.id());
		auto& children = parent_tree_info.children;
		for (int i = 0; i < children.size(); i++) {
			if (children[i].id() == e.id()) {
				children.erase(children.begin() + i);
				i--;
			}
		}
		set_dirty(tree_info.parent, true, DIRTY_DRAW_ORDER);
		travel_down(e, [&removed_children](const Handle&, const Handle& child) {
			removed_children.push_back(child);
			return false;
		});
		handle_removed_children(removed_children);
		tree_info.parent = Handle::make_invalid_handle();

		removed_enitites_touch_listener.clear();
		for (const auto& e : removed_children) {
			if (registry.has<Touching_Information>(e.id())) {
				removed_enitites_touch_listener.push_back(e);
			}
		}
		clean_touch_removed_child_listener_entities(removed_enitites_touch_listener);

		removed_entites_scroll_listener.clear();
		for (const auto& e : removed_children) {
			if (registry.has<Scroll_Information>(e.id())) {
				removed_entites_scroll_listener.push_back(e);
			}
		}
		clean_scroll_removed_child_listener_entities(removed_entites_scroll_listener);
	}

	void clean_up(const Handle& e) {
		assert_handle(e);
		remove_child(e);
		std::vector<Handle> nodes;
		travel_down(e, [&nodes](const Handle&, const Handle& child) {
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
				case DESCRIPTION_TYPE::LAYOUT_2D: {
					remove_layout(node);
					break;
				}
				case DESCRIPTION_TYPE::IMAGE_2D: {
					remove_image(node);
					break;
				}
				case DESCRIPTION_TYPE::LABEL_2D: {
					remove_label(node);
					break;
				}
				default: {
					break;
				}
			}
			entity_generator.release_handle(node);
		}
	}

	std::vector<Handle> find_entites_listen_touch_at_(glm::vec2 world_touch_position) {
		std::vector<Handle> found_entities{};
		for (const auto& e : entities_touch_listener) {
			const auto& [touch_info] = registry.read<Touching_Information>(e.id());
			if (Math::point_in_rect(touch_info.transformed_rect, world_touch_position)) {
				found_entities.push_back(e);
				if (touch_info.swallow_touches) {
					break;
				}
			}
		}
		return found_entities;
	}

	const Handle& get_root() {
		return root;
	}

	void set_scale(const Handle& e, const glm::vec2& scale) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.scale != scale, DIRTY_TRANSFORM);
		transform.scale = scale;
	}

	glm::vec2 get_scale(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return {0.f, 0.f};
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.scale;
	}

	void set_position(const Handle& e, const glm::vec2& position) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.position != position, DIRTY_TRANSFORM);
		transform.position = position;
		/**
		 * @Note: when use position we turn off position percent mode.
		 */
		if (registry.has<Position_Percent>(e.id())) {
			auto [position_percent] = registry.get<Position_Percent>(e.id());
			position_percent.enabled = false;
			clean_dirty(e, DIRTY_RELATIVE_POSITION);
		}
	}

	glm::vec2 get_position(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return {0.f, 0.f};
		if (is_dirty(e, DIRTY_RELATIVE_POSITION)) {
			resovle_relative_position(e);
		}
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.position;
	}

	void set_rotation(const Handle& e, float rotation) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.rotation != rotation, DIRTY_TRANSFORM);
		transform.rotation = rotation;
	}

	float get_rotation(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return 0.f;
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.rotation;
	}

	void set_opacity(const Handle& e, float opacity) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.opacity != opacity, DIRTY_TRANSFORM);
		transform.opacity = std::max(std::min(opacity, 1.f), 0.f);
	}

	float get_opacity(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return 0.f;
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.opacity;
	}

	void set_color(const Handle& e, const glm::vec3& color) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.color != color, DIRTY_TRANSFORM);
		transform.color = color;
	}

	glm::vec3 get_color(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return {1.f, 1.f, 1.f};
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.color;
	}

	void set_anchor(const Handle& e, const glm::vec2& anchor) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.anchor != anchor, DIRTY_TRANSFORM);
		transform.anchor = anchor;
	}

	glm::vec2 get_anchor(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return {0.f, 0.f};
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.anchor;
	}

	void set_visible(const Handle& e, bool is_visible) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.is_visible != is_visible, DIRTY_TRANSFORM);
		transform.is_visible = is_visible;
	}

	bool is_visible(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return false;
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.is_visible;
	}

	void set_z_order(const Handle& e, uint32_t z_order) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return;
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.z_order != z_order, DIRTY_TRANSFORM);
		transform.z_order = z_order;
	}

	uint32_t get_z_order(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return 0;
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.z_order;
	}

	void set_touch_enabled(const Handle& e, bool touch_enabled) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id())) {
			return;
		}
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		set_dirty(e, touch_info.enabled != touch_enabled, DIRTY_TOUCHING);
		touch_info.enabled = touch_enabled;
	}

	bool is_touch_enabled(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id()))
			return false;
		const auto& [touch_info] = registry.read<Touching_Information>(e.id());
		return touch_info.enabled;
	}

	void set_swallow_touches(const Handle& e, bool swallows) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id())) {
			return;
		}
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		touch_info.swallow_touches = swallows;
	}

	bool is_swallow_touches(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id()))
			return false;
		const auto& [touch_info] = registry.read<Touching_Information>(e.id());
		return touch_info.swallow_touches;
	}

	void set_touching_caller(const Handle& e, const std::function<void(Handle, TOUCH_STATE, glm::vec2)> caller) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id())) {
			return;
		}
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		touch_info.caller = caller;
	}

	void set_nine_scale_enabled(const Handle& e, bool enabled) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return;
		}
		auto [nine_scale] = registry.get<Nine_Scale_Information>(e.id());
		set_dirty(e, nine_scale.enabled != enabled, DIRTY_RENDERER);
		nine_scale.enabled = enabled;
	}

	bool is_nine_scale_enabled(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return false;
		}
		const auto& [nine_scale] = registry.read<Nine_Scale_Information>(e.id());
		return nine_scale.enabled;
	}

	void set_nine_scale_rect(const Handle& e, const Geometry::Rect_2D& capset) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return;
		}
		auto [nine_scale] = registry.get<Nine_Scale_Information>(e.id());
		set_dirty(e, nine_scale.use_rect_ratio && nine_scale.capset_rect != capset, DIRTY_RENDERER);
		nine_scale.use_rect_ratio = false;
		nine_scale.capset_rect = capset;
	}

	Geometry::Rect_2D get_nine_scale_rect(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return {};
		}
		const auto& [nine_scale] = registry.read<Nine_Scale_Information>(e.id());
		return nine_scale.capset_rect;
	}

	void set_nine_scale_rect_ratio(const Handle& e, const Geometry::Texture_Rect_2D& capset_ratio) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return;
		}
		auto [nine_scale] = registry.get<Nine_Scale_Information>(e.id());
		set_dirty(e, !nine_scale.use_rect_ratio && nine_scale.capset_rect_ratio != capset_ratio, DIRTY_RENDERER);
		nine_scale.use_rect_ratio = true;
		nine_scale.capset_rect_ratio = capset_ratio;
	}

	Geometry::Texture_Rect_2D get_nine_scale_rect_ratio(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return {};
		}
		const auto& [nine_scale] = registry.read<Nine_Scale_Information>(e.id());
		return nine_scale.capset_rect_ratio;
	}

	void set_content_size(const Handle& e, const glm::vec2& size) {
		assert_handle(e);
		if (!registry.has_all<Size, Description>(e.id()))
			return;
		clean_size_components(e);
		auto [c_size] = registry.get<Size>(e.id());
		set_dirty(e, c_size.value != size, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE);
		c_size.value = size;
	}

	glm::vec2 get_content_size(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Size>(e.id()))
			return {1.f, 1.f};
		if (registry.has<Description>(e.id())) {
			const auto& [description] = registry.read<Description>(e.id());
			switch (description.value) {
				/**
				 * @Note: with type need calculate size
				 * by render we call update once when dirty
				 * to user have size to use.
				 */
				case DESCRIPTION_TYPE::IMAGE_2D: {
					if (is_dirty(e, DIRTY_RENDERER)) {
						update_image_renderer(e);
						set_dirty(e, false, DIRTY_RENDERER);
					}
					break;
				}
				case DESCRIPTION_TYPE::LABEL_2D: {
					if (is_dirty(e, DIRTY_RENDERER)) {
						update_label_renderer(e);
						set_dirty(e, false, DIRTY_RENDERER);
					}
					break;
				}
				default: {
					break;
				}
			}
		}
		if (is_dirty(e, DIRTY_RELATIVE_SIZE)) {
			resovle_relative_size(e);
		}
		const auto& [c_size] = registry.read<Size>(e.id());
		return c_size.value;
	}

	glm::vec2 get_inner_renderer_size(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Inner_Renderer_Size>(e.id())) {
			return {0.f, 0.f};
		}
		const auto& [inner_size] = registry.read<Inner_Renderer_Size>(e.id());
		return inner_size.value;
	}

	void set_position_percent(const Handle& e, const glm::vec2& ratio_position) {
		assert_handle(e);
		if (!registry.has<Position_Percent>(e.id()))
			return;
		glm::vec2 valid_ratio_position = glm::max(glm::min(ratio_position, 1.f), 0.f);
		auto [position_percent] = registry.get<Position_Percent>(e.id());
		set_dirty(
			e, !position_percent.enabled && position_percent.ratio_position != valid_ratio_position, DIRTY_TRANSFORM
		);
		set_dirty(e, true, DIRTY_RELATIVE_POSITION);
		position_percent.enabled = true;
		position_percent.ratio_position = valid_ratio_position;
	}

	glm::vec2 get_position_percent(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Position_Percent>(e.id()))
			return {0.f, 0.f};
		const auto& [position_percent] = registry.read<Position_Percent>(e.id());
		return position_percent.enabled ? position_percent.ratio_position : glm::vec2{0.f, 0.f};
	}

	void set_size_percent(const Handle& e, const glm::vec2& ratio_size) {
		assert_handle(e);
		if (!registry.has<Size_Percent>(e.id()))
			return;
		clean_size_components(e);
		auto [size_percent] = registry.get<Size_Percent>(e.id());
		glm::vec2 valid_ratio_size = glm::max(glm::min(ratio_size, 1.f), 0.f);
		bool is_dirty = !size_percent.enabled && size_percent.ratio_size != valid_ratio_size;
		set_dirty(e, is_dirty, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE);
		size_percent.enabled = true;
		size_percent.ratio_size = valid_ratio_size;
	}

	glm::vec2 get_size_percent(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Size_Percent>(e.id()))
			return {0.f, 0.f};
		const auto& [size_percent] = registry.read<Size_Percent>(e.id());
		return size_percent.enabled ? size_percent.ratio_size : glm::vec2{0.f, 0.f};
	}

	void set_size_pinning(const Handle& e, const Size_Pinning_Data& pinning) {
		assert_handle(e);
		if (!registry.has<Size_Pinning>(e.id()))
			return;
		clean_size_components(e);
		auto [size_pinning] = registry.get<Size_Pinning>(e.id());
		bool is_dirty = !size_pinning.enabled && size_pinning.is_changing(pinning);
		set_dirty(e, is_dirty, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE);
		size_pinning.enabled = true;
		size_pinning.update(pinning);
	}

	Size_Pinning_Data get_size_pinning(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Size_Pinning>(e.id()))
			return {};
		const auto& [size_pinning] = registry.read<Size_Pinning>(e.id());
		return size_pinning.enabled ? size_pinning.get_size_pinning() : Size_Pinning_Data{};
	}

	void set_layout_background_enabled(const Handle& e, bool enabled) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return;
		}
		auto [layout_renderer] = registry.get<Layout_Renderer_Information>(e.id());
		set_dirty(e, layout_renderer.enabled != enabled, DIRTY_RENDERER);
		layout_renderer.enabled = enabled;
	}

	bool is_layout_background_enabled(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return false;
		}
		const auto& [layout_renderer] = registry.read<Layout_Renderer_Information>(e.id());
		return layout_renderer.enabled;
	}

	void set_layout_background_color(const Handle& e, const glm::vec3& color) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return;
		}
		auto [layout_renderer] = registry.get<Layout_Renderer_Information>(e.id());
		set_dirty(e, layout_renderer.background_color != color, DIRTY_RENDERER);
		layout_renderer.background_color = color;
	}

	glm::vec3 get_layout_background_color(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return {1.f, 1.f, 1.f};
		}
		const auto& [layout_renderer] = registry.read<Layout_Renderer_Information>(e.id());
		return layout_renderer.background_color;
	}

	void set_layout_border_radius(const Handle& e, float border_radius) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return;
		}
		auto [layout_renderer] = registry.get<Layout_Renderer_Information>(e.id());
		set_dirty(e, layout_renderer.border_radius != border_radius, DIRTY_RENDERER);
		layout_renderer.border_radius = border_radius;
	}

	float get_layout_border_radius(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return 0.f;
		}
		const auto& [layout_renderer] = registry.read<Layout_Renderer_Information>(e.id());
		return layout_renderer.border_radius;
	}

	void set_layout_border_width(const Handle& e, float border_width) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return;
		}
		auto [layout_renderer] = registry.get<Layout_Renderer_Information>(e.id());
		set_dirty(e, layout_renderer.border_width != border_width, DIRTY_RENDERER);
		layout_renderer.border_width = border_width;
	}

	float get_layout_border_width(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return 0.f;
		}
		const auto& [layout_renderer] = registry.read<Layout_Renderer_Information>(e.id());
		return layout_renderer.border_width;
	}

	void set_layout_border_color(const Handle& e, const glm::vec3& color) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return;
		}
		auto [layout_renderer] = registry.get<Layout_Renderer_Information>(e.id());
		set_dirty(e, layout_renderer.border_color != color, DIRTY_RENDERER);
		layout_renderer.border_color = color;
	}

	glm::vec3 get_layout_border_color(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Layout_Renderer_Information>(e.id())) {
			return {1.f, 1.f, 1.f};
		}
		const auto& [layout_renderer] = registry.read<Layout_Renderer_Information>(e.id());
		return layout_renderer.border_color;
	}

	void set_layout_clipping_enabled(const Handle& e, bool enabled) {
		assert_handle(e);
		if (!registry.has<Clipping_Information>(e.id())) {
			return;
		}
		auto [clipping] = registry.get<Clipping_Information>(e.id());
		set_dirty(e, clipping.enabled != enabled, DIRTY_CLIPPING | DIRTY_RENDERER);
		clipping.enabled = enabled;
	}

	bool is_layout_clipping_enabled(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Clipping_Information>(e.id())) {
			return false;
		}
		const auto& [clipping] = registry.read<Clipping_Information>(e.id());
		return clipping.enabled;
	}

	void set_image_path(const Handle& e, const std::string& image_path) {
		assert_handle(e);
		if (!registry.has<Image_Renderer_Information>(e.id())) {
			return;
		}
		auto [image_renderer] = registry.get<Image_Renderer_Information>(e.id());
		set_dirty(e, image_renderer.image_path != image_path, DIRTY_RENDERER);
		image_renderer.image_path = image_path;
	}

	std::string get_image_path(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Image_Renderer_Information>(e.id())) {
			return "";
		}
		const auto& [image_renderer] = registry.read<Image_Renderer_Information>(e.id());
		return image_renderer.image_path;
	}

	void set_image_rect(const Handle& e, const Geometry::Texture_Rect_2D& image_rect) {
		assert_handle(e);
		if (!registry.has<Image_Renderer_Information>(e.id())) {
			return;
		}
		auto [image_renderer] = registry.get<Image_Renderer_Information>(e.id());
		set_dirty(e, image_renderer.image_rect != image_rect, DIRTY_RENDERER);
		image_renderer.image_rect = image_rect;
	}

	Geometry::Texture_Rect_2D get_image_rect(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Image_Renderer_Information>(e.id())) {
			return {};
		}
		const auto& [image_renderer] = registry.read<Image_Renderer_Information>(e.id());
		return image_renderer.image_rect;
	}

	void set_label_font_size(const Handle& e, int font_size) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.font_size != font_size, DIRTY_RENDERER);
		label_renderer.font_size = font_size;
	}

	int get_label_font_size(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return 0;
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.font_size;
	}

	void set_label_font_path(const Handle& e, const std::string& font_path) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.font_path != font_path, DIRTY_RENDERER);
		label_renderer.font_path = font_path;
	}

	std::string get_label_font_path(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return "";
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.font_path;
	}

	void set_label_string(const Handle& e, const std::string& text) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.text != text, DIRTY_RENDERER);
		label_renderer.text = text;
	}

	std::string get_label_string(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return "";
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.text;
	}

	void set_label_stroke_size(const Handle& e, float stroke_size) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.stroke_size != stroke_size, DIRTY_RENDERER);
		label_renderer.stroke_size = stroke_size;
	}

	float get_label_stroke_size(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return 0.f;
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.stroke_size;
	}

	void set_label_stroke_color(const Handle& e, const glm::vec3& color) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.stroke_color != color, DIRTY_RENDERER);
		label_renderer.stroke_color = color;
	}

	glm::vec3 get_label_stroke_color(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return {1.f, 1.f, 1.f};
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.stroke_color;
	}

	void set_scroll_enabled(const Handle& e, bool enabled) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		set_dirty(e, scroll_info.enabled != enabled, DIRTY_SCROLLING);
		scroll_info.enabled = enabled;
	}

	bool is_scroll_enabled(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return false;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		return scroll_info.enabled;
	}

	void set_inner_container_size(const Handle& e, const glm::vec2& size) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		set_dirty(e, scroll_info.size != size, DIRTY_RENDERER);
		set_dirty(scroll_info.container_handle, scroll_info.size != size, DIRTY_TRANSFORM);
		scroll_info.size = size;
	}

	glm::vec2 get_inner_container_size(const Handle& e) {
		assert_handle(e);
		if (!registry.has_all<Scroll_Information, Description, Tree_Information>(e.id())) {
			return {0.f, 0.f};
		}
		const auto& [scroll_info, description, tree_info] =
			registry.read<Scroll_Information, Description, Tree_Information>(e.id());
		if (!tree_info.valid_child) {
			return scroll_info.size;
		}
		if (is_dirty(e, DIRTY_RENDERER)) {
			switch (description.value) {
				case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
					update_scroll_layout_renderer(e);
					resovle_relative_size(e);
					break;
				}
				default: {
					break;
				}
			}
		}
		const auto& [c_size] = registry.read<Size>(e.id());
		return glm::vec2{std::max(scroll_info.size.x, c_size.value.x), std::max(scroll_info.size.y, c_size.value.y)};
	}

	void set_scroll_direction(const Handle& e, SCROLL_DIRECTION direction) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		set_dirty(e, scroll_info.scroll_direction != direction, DIRTY_RENDERER);
		scroll_info.scroll_direction = direction;
	}

	SCROLL_DIRECTION get_scroll_direction(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return SCROLL_DIRECTION::SCROLL_HORIZONTAL;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		return scroll_info.scroll_direction;
	}

	void set_scroll_speed(const Handle& e, float speed) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		scroll_info.scroll_speed = speed;
	}

	float get_scroll_speed(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return SCROLL_DIRECTION::SCROLL_HORIZONTAL;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		return scroll_info.scroll_speed;
	}

	void set_touch_layout_scroll_enabled(const Handle& e, bool enabled) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		auto [touch_info] = registry.get<Touching_Information>(scroll_info.container_handle.id());
		set_dirty(scroll_info.container_handle, touch_info.enabled != enabled, DIRTY_TOUCHING);
		touch_info.enabled = enabled;
	}

	bool is_touch_layout_scroll_enabled(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return false;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		const auto& [touch_info] = registry.read<Touching_Information>(scroll_info.container_handle.id());
		return touch_info.enabled;
	}

	void set_swallow_scrolling(const Handle& e, bool swallows) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		set_dirty(scroll_info.container_handle, scroll_info.swallow_scroll != swallows, DIRTY_TOUCHING);
		scroll_info.enabled = swallows;
	}

	bool is_swallow_scrolling(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return false;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		return scroll_info.swallow_scroll;
	}

} // namespace Engine