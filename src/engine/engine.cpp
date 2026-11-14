#include <core.h>
#include <engine/components.h>
#include <engine/engine.h>
#include <engine/scroll_handler.h>
#include <engine/systems.h>
#include <engine/touch_handler.h>
#include <engine/utils.h>
#include <functional>
#include <id_generator.h>
#include <math_custom.h>
#include <queue>
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

	enum TRAVEL_TYPE {
		TRAVEL_DFS,
		TRAVEL_BFS
	};

	const float RATE_CANCEL_MOVE = 0.5f;

	Registry registry{};

	Handle_Generator entity_generator{};

	Handle root = Handle::make_invalid_handle();

	Sparse_Set<Handle> dirty_entities{};

	std::vector<Handle> entities_touch_listener{};

	std::vector<Handle> entities_scroll_listener{};

	std::vector<Handle> clean_up_entities{};

	glm::vec2 cursor_world_position = {0.f, 0.f};

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

	void travel_down_dfs(
		const Handle& root, const std::function<bool(const Handle& parent, const Handle& child)>& fn,
		bool reverse = false
	) {
		if (!registry.has<Tree_Information>(root.id())) {
			fn(Handle::make_invalid_handle(), root);
			return;
		}
		std::stack<Handle> stack{};
		stack.push(root);
		while (stack.size()) {
			Handle current = stack.top();
			stack.pop();
			const auto& [tree_info] = registry.read<Tree_Information>(current.id());
			if (fn(tree_info.parent, current))
				break;

			if (reverse) {
				for (int i = 0; i < tree_info.children.size(); i++) {
					const auto& child = tree_info.children[i];
					stack.push(child);
				}
			} else {
				for (int i = tree_info.children.size() - 1; i > -1; i--) {
					const auto& child = tree_info.children[i];
					stack.push(child);
				}
			}
		}
	}

	void travel_down_bfs(const Handle& root, const std::function<bool(const Handle& parent, const Handle& child)>& fn) {
		if (!registry.has<Tree_Information>(root.id())) {
			fn(Handle::make_invalid_handle(), root);
			return;
		}
		std::queue<Handle> queue{};
		queue.push(root);
		while (queue.size()) {
			Handle current = queue.front();
			queue.pop();
			const auto& [tree_info] = registry.read<Tree_Information>(current.id());
			if (fn(tree_info.parent, current))
				break;
			for (int i = tree_info.children.size() - 1; i > -1; i--) {
				const auto& child = tree_info.children[i];
				queue.push(child);
			}
		}
	}

	void travel_down(
		const Handle& root, const std::function<bool(const Handle& parent, const Handle& child)>& fn,
		TRAVEL_TYPE travel_type = TRAVEL_TYPE::TRAVEL_DFS, bool reverse = false
	) {
		switch (travel_type) {
			case TRAVEL_TYPE::TRAVEL_DFS: {
				travel_down_dfs(root, fn, reverse);
				break;
			}
			default: {
				travel_down_bfs(root, fn);
				break;
			}
		}
	}

	void travel_up(Handle child, const std::function<bool(const Handle& parent, const Handle& child)>& fn) {
		if (!registry.has<Tree_Information>(child.id())) {
			return;
		}
		const auto& [tree_info] = registry.read<Tree_Information>(child.id());
		Handle parent = tree_info.parent;
		bool stop = false;
		while (!stop && !child.is_invalid() && registry.has<Tree_Information>(parent.id())) {
			stop = fn(parent, child);
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
		registry.erase<Draw_Order>(e.id());
	}

	void remove_layout(const Handle& e) {
		remove_node(e);
		const auto& [handles] = registry.read<Renderer_Handles>(e.id());
		for (const auto& handle : handles.value) {
			if (handle.is_invalid())
				continue;
			Vulkan::remove_draw(handle);
		}
		registry.erase<Layout_Renderer_Information>(e.id());
		registry.erase<Renderer_Handles>(e.id());
		registry.erase<Size_Pinning>(e.id());
		registry.erase<Size_Percent>(e.id());
		registry.erase<Touching_Information>(e.id());
		registry.erase<Size>(e.id());
		registry.erase<Clipping_Information>(e.id());
		registry.erase<Rect>(e.id());
	}

	void remove_scroll_layout(const Handle& e) {
		remove_layout(e);
		registry.erase<Scroll_Information>(e.id());
	}

	void remove_image(const Handle& e) {
		remove_node(e);
		const auto& [handles] = registry.read<Renderer_Handles>(e.id());
		for (const auto& handle : handles.value) {
			if (handle.is_invalid())
				continue;
			Vulkan::remove_draw(handle);
		}
		const auto& [nine_scale_handles] = registry.read<Nine_Scale_Renderer_Handles>(e.id());
		for (const auto& handle : nine_scale_handles.value) {
			if (handle.is_invalid())
				continue;
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
		registry.erase<Rect>(e.id());
	}

	void remove_label(const Handle& e) {
		remove_node(e);
		const auto& [handles] = registry.read<Renderer_Handles>(e.id());
		for (const auto& handle : handles.value) {
			if (handle.is_invalid())
				continue;
			Vulkan::remove_draw(handle);
		}
		registry.erase<Size>(e.id());
		registry.erase<Label_Renderer_Information>(e.id());
		registry.erase<Renderer_Handles>(e.id());
		registry.erase<Inner_Renderer_Size>(e.id());
		registry.erase<Touching_Information>(e.id());
		registry.erase<Rect>(e.id());
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

	bool has_node_ancestor_dirty(const Handle& e, uint64_t flag) {
		bool found_dirty = false;
		travel_up(e, [&found_dirty, &flag](const Handle&, const Handle& child) {
			found_dirty = is_dirty(child, flag);
			return found_dirty;
		});
		return found_dirty;
	}

	void remove_entity_from_dirty_list(const Handle& e) {
		if (dirty_entities.has(e.id())) {
			dirty_entities.erase(e.id());
		}
	}

	std::vector<Handle> make_processing_dirty_entities(
		uint64_t dirty_flag, TRAVEL_NODE_MODE travel_mode, PICKED_NODE_MODE picked_mode = PICKED_NODE_MODE::PICKED_ALL
	) {
		std::vector<Handle> processing_dirty_entities;
		std::vector<Handle> processing_traveled_entities;
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
						dirty_entity, [&dirty_flag, &picked_mode, &push_item_by_picked_mode,
									   &processing_traveled_entities](const Handle&, const Handle& child) {
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
						dirty_entity, [&dirty_flag, &picked_mode, &push_item_by_picked_mode,
									   &processing_traveled_entities](const Handle&, const Handle& child) {
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
				std::vector<Handle> processing_dirty_parent_entities;
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
						dirty_entity, [&dirty_flag, &picked_mode, &push_item_by_picked_mode,
									   &processing_traveled_entities](const Handle&, const Handle& child) {
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
						dirty_entity, [&dirty_flag, &picked_mode, &push_item_by_picked_mode,
									   &processing_traveled_entities](const Handle&, const Handle& child) {
							push_item_by_picked_mode(dirty_flag, picked_mode, processing_traveled_entities, child);
							return false;
						}
					);
				}
				break;
			}
		}
		return std::move(processing_traveled_entities);
	}

	void resolve_relative_size(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id()))
			return;
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		if (!tree_info.valid_child)
			return;
		auto traveled_enities = make_processing_dirty_entities(DIRTY_RELATIVE_SIZE, TRAVEL_NODE_MODE::TRAVEL_ALL);
		handle_dirty_relative_size(traveled_enities);
	}

	void resolve_relative_position(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id()))
			return;
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		if (!tree_info.valid_child)
			return;
		resolve_relative_size(e);
		auto traveled_enities = make_processing_dirty_entities(DIRTY_RELATIVE_POSITION, TRAVEL_NODE_MODE::TRAVEL_ALL);
		handle_dirty_relative_position(traveled_enities);
	}

	void resolve_transform(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id()))
			return;
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		if (!tree_info.valid_child)
			return;
		resolve_relative_position(e);
		auto traveled_enities = make_processing_dirty_entities(DIRTY_TRANSFORM, TRAVEL_NODE_MODE::TRAVEL_ALL);
		handle_dirty_transform(traveled_enities);
	}

	void update_tree_changing_z_order() {
		std::vector<Handle> prcessing_entities{};
		travel_down(root, [&prcessing_entities](const Handle&, const Handle& child) {
			prcessing_entities.push_back(child);
			return false;
		});
		handle_update_tree_draw_order(prcessing_entities);
	}

	std::vector<Handle> make_processing_enitites_touched() {
		std::vector<Handle> executing_entities_listener;
		const auto& handled_touches = get_handled_touches();
		auto screen_size = get_window_size();
		for (const auto& touch_info : handled_touches) {
			const auto& entities = touch_info.entities;
			for (const auto& e : entities) {
				if (!registry.has<Touching_Information>(e.id()))
					continue;
				auto [c_touch_info] = registry.get<Touching_Information>(e.id());
				auto touch_state = touch_info.touch_state;
				if (c_touch_info.canceling_touch_enabled) {
					auto last_touch = c_touch_info.touch_world_position;
					auto cur_touch = touch_info.touch_world_position;
					if (glm::distance(cur_touch, last_touch) >=
							RATE_CANCEL_MOVE * std::max(screen_size.x, screen_size.y) ||
						std::min(cur_touch.x, cur_touch.y) < 0.f || cur_touch.x > screen_size.x ||
						cur_touch.y > screen_size.y) {
						touch_state = TOUCH_STATE::TOUCH_CANCEL;
					}
				}
				c_touch_info.touch_state = touch_state;
				c_touch_info.touch_world_position = touch_info.touch_world_position;
				c_touch_info.touch_type = touch_info.touch_type;
				c_touch_info.touch_mods = touch_info.touch_mods;
				executing_entities_listener.push_back(e);
			}
		}
		return std::move(executing_entities_listener);
	}

	void clean_touch_removed_child_listener_entities(const std::vector<Handle>& entities) {
		for (const auto& e : entities) {
			Utils::remove_entities_from(entities_touch_listener, e);
		}
		Utils::sort_entities(entities_touch_listener, [](const Handle& a, const Handle& b) {
			const auto& [touch_info_a] = registry.read<Touching_Information>(a.id());
			const auto& [touch_info_b] = registry.read<Touching_Information>(b.id());
			return touch_info_a.touched_order > touch_info_b.touched_order;
		});
		discard_handling_touches_entities(entities);
	}

	void handle_update_entities_touch_listener(const std::vector<Handle>& dirty_entities) {
		std::vector<Handle> removed_enitites_listener;
		for (const auto& e : dirty_entities) {
			if (!registry.has<Touching_Information>(e.id()))
				continue;
			const auto& [touch_info] = registry.read<Touching_Information>(e.id());
			if (!touch_info.enabled) {
				Utils::remove_entities_from(entities_touch_listener, e);
				removed_enitites_listener.push_back(e);
			} else {
				Utils::insert_entities_to(entities_touch_listener, e);
			}
		}
		Utils::sort_entities(entities_touch_listener, [](const Handle& a, const Handle& b) {
			const auto& [touch_info_a] = registry.read<Touching_Information>(a.id());
			const auto& [touch_info_b] = registry.read<Touching_Information>(b.id());
			return touch_info_a.touched_order > touch_info_b.touched_order;
		});
		discard_handling_touches_entities(removed_enitites_listener);
	}

	void clean_scroll_removed_child_listener_entities(const std::vector<Handle>& entities) {
		for (const auto& e : entities) {
			if (!registry.has<Scroll_Information>(e.id()))
				continue;
			Utils::remove_entities_from(entities_scroll_listener, e);
		}
		Utils::sort_entities(entities_scroll_listener, [](const Handle& a, const Handle& b) {
			const auto& [scroll_info_a] = registry.read<Scroll_Information>(a.id());
			const auto& [scroll_info_b] = registry.read<Scroll_Information>(b.id());
			return scroll_info_a.scroll_order > scroll_info_b.scroll_order;
		});
	}

	void handle_update_entities_scroll_listener(const std::vector<Handle>& dirty_entities) {
		for (const auto& e : dirty_entities) {
			if (!registry.has<Scroll_Information>(e.id()))
				continue;
			const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
			if (!scroll_info.enabled) {
				Utils::remove_entities_from(entities_scroll_listener, e);
			} else {
				Utils::insert_entities_to(entities_scroll_listener, e);
			}
			Utils::sort_entities(entities_scroll_listener, [](const Handle& a, const Handle& b) {
				const auto& [scroll_info_a] = registry.read<Scroll_Information>(a.id());
				const auto& [scroll_info_b] = registry.read<Scroll_Information>(b.id());
				return scroll_info_a.scroll_order > scroll_info_b.scroll_order;
			});
		}
	}

	bool is_position_clipped_by_clipping_ancestors(const Handle& e, const glm::vec2& touch_position) {
		Registry& registry = get_registry();
		if (!registry.has_all<Clipping_Ancestor_Information, Rect>(e.id())) {
			return false;
		}
		const auto& [clipping_ancestor, rect] = registry.read<Clipping_Ancestor_Information, Rect>(e.id());
		for (int i = 0; i < clipping_ancestor.number_clipping; i++) {
			const auto& clipping_parent = clipping_ancestor.clipping_ancestors[i];
			if (!registry.has<Rect>(clipping_parent.id()))
				continue;
			const auto& [parent_rect] = registry.read<Rect>(clipping_parent.id());
			if (!Math::point_in_rect(parent_rect.transformed_rect, touch_position)) {
				return true;
			}
		}
		return false;
	}

	bool is_world_position_in_entity_rect(const Handle& e, const glm::vec2& touch_position) {
		if (!registry.has<Touching_Information>(e.id()))
			return false;
		const auto& [touch_info, rect] = registry.read<Touching_Information, Rect>(e.id());
		return !is_position_clipped_by_clipping_ancestors(e, touch_position) &&
			   Math::point_in_rect(rect.transformed_rect, touch_position);
	}

	void process_clean_up_entities() {
		for (const auto& e : clean_up_entities) {
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
					case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
						remove_scroll_layout(node);
						break;
					}
					case DESCRIPTION_TYPE::SCROLL_LISTENER_CONTAINER:
					case DESCRIPTION_TYPE::INNER_CONTAINER:
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
		clean_up_entities.clear();
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
		registry.add(entity.id(), Draw_Order{});
		return entity;
	}

	void init(GLFWwindow* window) {
		Vulkan::init_vulkan(window, []() { set_content_size(root, get_window_size()); });
		glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
			glm::vec2 screen_size = get_window_size();
			on_cursor_touch_position(window, x, y);
			on_cursor_scroll_position(window, x, y);
			cursor_world_position = glm::vec2(x, screen_size.y - y);
		});
		init_touch_handler(window);
		init_scroll_handler(window);
		init_root();
	}

	void process_frame(double dt) {

		std::vector<Handle> processing_dirty_entities;

		handle_touches_logic();
		processing_dirty_entities = make_processing_enitites_touched();
		handle_enities_touched(processing_dirty_entities);

		if (has_scrolling()) {
			processing_dirty_entities = find_entities_listen_scroll_at(get_world_scrolling_position());
			handle_entites_scrolling(processing_dirty_entities);
		}

		process_clean_up_entities();

		processing_dirty_entities = make_processing_dirty_entities(DIRTY_DRAW_ORDER, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_dirty_z_order(processing_dirty_entities);
		if (processing_dirty_entities.size() > 0) {
			update_tree_changing_z_order();
		}

		processing_dirty_entities = make_processing_dirty_entities(DIRTY_RENDERER, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_dirty_renderer(processing_dirty_entities);

		processing_dirty_entities = make_processing_dirty_entities(
			DIRTY_RELATIVE_SIZE, TRAVEL_NODE_MODE::TRAVEL_DOWN, PICKED_NODE_MODE::PICKED_ALL
		);
		handle_dirty_relative_size(processing_dirty_entities);

		processing_dirty_entities = make_processing_dirty_entities(
			DIRTY_RELATIVE_POSITION, TRAVEL_NODE_MODE::TRAVEL_DOWN, PICKED_NODE_MODE::PICKED_ALL
		);
		handle_dirty_relative_position(processing_dirty_entities);

		processing_dirty_entities = make_processing_dirty_entities(DIRTY_CLIPPING, TRAVEL_NODE_MODE::TRAVEL_DOWN);
		handle_dirty_clipping(processing_dirty_entities);

		processing_dirty_entities = make_processing_dirty_entities(DIRTY_TRANSFORM, TRAVEL_NODE_MODE::TRAVEL_DOWN);
		handle_dirty_transform(processing_dirty_entities);

		processing_dirty_entities = make_processing_dirty_entities(DIRTY_TOUCHING, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_update_entities_touch_listener(processing_dirty_entities);

		processing_dirty_entities = make_processing_dirty_entities(DIRTY_SCROLLING, TRAVEL_NODE_MODE::TRAVEL_NONE);
		handle_update_entities_scroll_listener(processing_dirty_entities);

		dirty_entities.clear();
		Vulkan::draw_frame(dt);
	}

	void destroy() {
		process_clean_up_entities();
		Vulkan::destroy_vulkan();
	}

	glm::vec2 get_window_size() {
		return Vulkan::Utils::get_window_size(Vulkan::device);
	}

	Registry& get_registry() {
		return registry;
	}

	bool is_valid(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id())) {
			return false;
		}
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		return tree_info.valid_child && entity_generator.validate_handle(e);
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
		registry.add(layout.id(), Rect{});
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
		registry.add(image.id(), Rect{});
		return image;
	}

	Handle create_label() {
		Handle label = create_node(DESCRIPTION_TYPE::LABEL_2D);
		registry.add(label.id(), Size{{0.f, 0.f}});
		registry.add(label.id(), Label_Renderer_Information{});
		registry.add(label.id(), Renderer_Handles{});
		registry.add(label.id(), Inner_Renderer_Size{});
		registry.add(label.id(), Touching_Information{});
		registry.add(label.id(), Rect{});
		return label;
	}

	Handle create_scroll_layout() {
		Handle scroll_layout = create_layout();
		Handle inner_container = create_layout();
		Handle touch_container = create_layout();
		Scroll_Information container{};
		container.container_handle = inner_container;
		container.container_touch_handle = touch_container;
		registry.add(scroll_layout.id(), container);
		add_child(scroll_layout, inner_container);
		add_child(scroll_layout, touch_container);
		set_touch_enabled(touch_container, true);
		set_touch_enabled(scroll_layout, true);
		set_swallow_touches(touch_container, false);
		set_layout_clipping_enabled(scroll_layout, true);
		set_touch_caller(
			touch_container,
			[](Handle touch_container, TOUCH_STATE touch_state, glm::vec2 world_position, int button, int) {
				if (button != GLFW_MOUSE_BUTTON_LEFT)
					return;
				const auto& [tree_info] = registry.read<Tree_Information>(touch_container.id());
				auto [scroll_info] = registry.get<Scroll_Information>(tree_info.parent.id());
				switch (touch_state) {
					case TOUCH_STATE::TOUCH_BEGAN: {
						// TODO: handle stop all scroll action here!
						scroll_info.start_touch = world_position;
						scroll_info.touch_state = TOUCH_SCROLL_STATE::IDLE;
						break;
					}
					case TOUCH_STATE::TOUCH_MOVE: {
						glm::vec2 delta_position = world_position - scroll_info.start_touch;
						bool is_moving = false;
						glm::vec2 moving_distance{0.f, 0.f};
						switch (scroll_info.scroll_direction) {
							case SCROLL_DIRECTION::SCROLL_HORIZONTAL: {
								if (abs(delta_position.x) >= scroll_info.dragging_threshold) {
									is_moving = true;
									moving_distance.x = delta_position.x;
								}
								break;
							}
							default: {
								if (abs(delta_position.y) >= scroll_info.dragging_threshold) {
									is_moving = true;
									moving_distance.y = delta_position.y;
								}
								break;
							}
						}
						if (is_moving) {
							scroll_info.touch_state = TOUCH_SCROLL_STATE::SCROLLING;
							auto [transform] = registry.get<Transform_2D>(scroll_info.container_handle.id());
							transform.position += moving_distance;
							set_dirty(tree_info.parent, true, DIRTY_TRANSFORM);
							scroll_info.start_touch = world_position;
							claim_touch(touch_container);
						}
						break;
					}
					case TOUCH_STATE::TOUCH_CANCEL:
					case TOUCH_STATE::TOUCH_ENDED: {
						// TODO: handle bounce here when has action!
						scroll_info.start_touch = {0.f, 0.f};
						scroll_info.touch_state = TOUCH_SCROLL_STATE::IDLE;
						break;
					}
					default: {
						break;
					}
				}
			}
		);
		auto [scroll_info] = registry.get<Scroll_Information>(scroll_layout.id());
		scroll_info.caller = [](Handle e, float scroll_distance) {
			const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
			auto [transform] = registry.get<Transform_2D>(scroll_info.container_handle.id());
			glm::vec2 distance{0.f, 0.f};
			switch (scroll_info.scroll_direction) {
				case SCROLL_DIRECTION::SCROLL_HORIZONTAL: {
					distance.x = scroll_distance;
					break;
				}
				case SCROLL_DIRECTION::SCROLL_VERTICAL: {
					distance.y = scroll_distance;
					break;
				}
				default: {
					break;
				}
			}
			transform.position += distance * scroll_info.scroll_speed;
			set_dirty(e, true, DIRTY_TRANSFORM);
		};
		set_scroll_enabled(scroll_layout, true);
		registry.add(scroll_layout.id(), Description{DESCRIPTION_TYPE::SCROLL_LAYOUT});
		registry.add(inner_container.id(), Description{DESCRIPTION_TYPE::INNER_CONTAINER});
		registry.add(touch_container.id(), Description{DESCRIPTION_TYPE::SCROLL_LISTENER_CONTAINER});
		return scroll_layout;
	}

	Handle get_parent(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id())) {
			return Handle::make_invalid_handle();
		}
		const auto& [tree_handle] = registry.read<Tree_Information>(e.id());
		return tree_handle.parent;
	}

	std::vector<Handle> get_children(const Handle& e) {
		if (!registry.has<Tree_Information>(e.id())) {
			return {};
		}
		const auto& [tree_handle] = registry.read<Tree_Information>(e.id());
		return tree_handle.children;
	}

	std::vector<Handle> get_scroll_layout_children(const Handle& e) {
		if (!registry.has<Scroll_Information>(e.id())) {
			return {};
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		const auto& [tree_info] = registry.read<Tree_Information>(scroll_info.container_handle.id());
		return tree_info.children;
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
			uint64_t all_dirty = DIRTY_TRANSFORM | DIRTY_RENDERER | DIRTY_CLIPPING;
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

	void add_child_to_container(const Handle& e_parent, const Handle& e_child) {
		if (!registry.has_all<Description, Scroll_Information>(e_parent.id())) {
			return;
		}
		const auto& [description, scroll_info] = registry.read<Description, Scroll_Information>(e_parent.id());
		switch (description.value) {
			case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
				break;
			}
			default: {
				return;
			}
		}
		add_child(scroll_info.container_handle, e_child);
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
		/**
		 * @Remove enitites in dirty list to
		 * don't update it in process frame.
		 */
		for (const auto& e : removed_children) {
			remove_entity_from_dirty_list(e);
		}

		std::vector<Handle> removed_enitites_listener;
		for (const auto& e : removed_children) {
			if (registry.has<Touching_Information>(e.id())) {
				removed_enitites_listener.push_back(e);
			}
		}
		clean_touch_removed_child_listener_entities(removed_enitites_listener);

		removed_enitites_listener.clear();
		for (const auto& e : removed_children) {
			if (registry.has<Scroll_Information>(e.id())) {
				removed_enitites_listener.push_back(e);
			}
		}
		clean_scroll_removed_child_listener_entities(removed_enitites_listener);
	}

	void clean_up(const Handle& e) {
		assert_handle(e);
		remove_child(e);
		clean_up_entities.push_back(e);
	}

	std::vector<Handle> find_entites_listen_touch_at(const glm::vec2& world_touch_position) {
		return std::move(
			Utils::find_entites_satify_from(entities_touch_listener, [&world_touch_position](const Handle& e) {
				bool found = false, stop = false;
				const auto& [touch_info, transform] = registry.read<Touching_Information, World_Transform_2D>(e.id());
				if (is_valid(e) && transform.is_visible && is_world_position_in_entity_rect(e, world_touch_position)) {
					found = true;
					if (touch_info.swallow_touches) {
						stop = true;
					}
				}
				return std::make_tuple(found, stop);
			})
		);
	}

	std::vector<Handle> find_entities_listen_scroll_at(const glm::vec2& world_scroll_position) {
		return std::move(
			Utils::find_entites_satify_from(entities_scroll_listener, [&world_scroll_position](const Handle& e) {
				bool found = false, stop = false;
				const auto& [scroll_info, transform] = registry.read<Scroll_Information, World_Transform_2D>(e.id());
				if (is_valid(e) && transform.is_visible && is_world_position_in_entity_rect(e, world_scroll_position)) {
					found = true;
					if (scroll_info.swallow_scroll) {
						stop = true;
					}
				}
				return std::make_tuple(found, stop);
			})
		);
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
			resolve_relative_position(e);
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
		if (!registry.has_all<Transform_2D, Description>(e.id()))
			return;
		const auto& [description] = registry.read<Description>(e.id());
		/**
		 * @Note: the scroll and inner container
		 * belong to logic of scroll we can't
		 * change order of that.
		 */
		switch (description.value) {
			case DESCRIPTION_TYPE::INNER_CONTAINER:
			case DESCRIPTION_TYPE::SCROLL_LISTENER_CONTAINER: {
				return;
			}
			default: {
				break;
			}
		}
		auto [transform] = registry.get<Transform_2D>(e.id());
		set_dirty(e, transform.z_order != z_order, DIRTY_TRANSFORM);
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		if (!tree_info.parent.is_invalid()) {
			set_dirty(tree_info.parent, transform.z_order != z_order, DIRTY_DRAW_ORDER);
		}
		transform.z_order = z_order;
	}

	uint32_t get_z_order(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Transform_2D>(e.id()))
			return 0;
		const auto& [transform] = registry.read<Transform_2D>(e.id());
		return transform.z_order;
	}

	void set_name(const Handle& e, const std::string& name) {
		assert_handle(e);
		if (!registry.has<Tree_Information>(e.id()))
			return;
		auto [tree_info] = registry.get<Tree_Information>(e.id());
		tree_info.name = name;
	}

	std::string get_name(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Tree_Information>(e.id()))
			return "";
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		return tree_info.name;
	}

	void set_tag(const Handle& e, int tag) {
		assert_handle(e);
		if (!registry.has<Tree_Information>(e.id()))
			return;
		auto [tree_info] = registry.get<Tree_Information>(e.id());
		tree_info.tag = tag;
	}

	int get_tag(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Tree_Information>(e.id()))
			return -1;
		const auto& [tree_info] = registry.read<Tree_Information>(e.id());
		return tree_info.tag;
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

	void set_touch_caller(const Handle& e, const std::function<void(Handle, TOUCH_STATE, glm::vec2, int, int)> caller) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id())) {
			return;
		}
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		touch_info.caller = caller;
	}

	void set_click_caller(const Handle& e, const std::function<void(Handle, glm::vec2, int, int)> caller) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id())) {
			return;
		}
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		touch_info.click_caller = caller;
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

	void set_image_nine_scale_size(const Handle& e, const glm::vec2& renderer_size) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return;
		}
		auto [nine_scale] = registry.get<Nine_Scale_Information>(e.id());
		set_dirty(e, nine_scale.use_rect_ratio && nine_scale.renderer_size != renderer_size, DIRTY_RENDERER);
		nine_scale.is_manual_update_inner_size = true;
		nine_scale.renderer_size = renderer_size;
	}

	glm::vec2 get_image_nine_scale_size(const Handle& e, const Handle& renderer_size) {
		assert_handle(e);
		if (!registry.has<Nine_Scale_Information>(e.id())) {
			return {0.f, 0.f};
		}
		const auto& [nine_scale] = registry.read<Nine_Scale_Information>(e.id());
		return nine_scale.renderer_size;
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
		const auto& [description] = registry.read<Description>(e.id());
		auto [c_size] = registry.get<Size>(e.id());
		set_dirty(e, c_size.value != size, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE | DIRTY_RELATIVE_POSITION);
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
		if (has_node_ancestor_dirty(e, DIRTY_RELATIVE_SIZE)) {
			resolve_relative_size(e);
		}
		const auto& [c_size] = registry.read<Size>(e.id());
		return c_size.value;
	}

	glm::vec2 get_inner_renderer_size(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Inner_Renderer_Size>(e.id())) {
			return {0.f, 0.f};
		}
		if (registry.has<Description>(e.id())) {
			const auto& [description] = registry.read<Description>(e.id());
			switch (description.value) {
				case DESCRIPTION_TYPE::LABEL_2D: {
					if (is_dirty(e, DIRTY_RENDERER)) {
						update_label_renderer(e);
						resolve_transform(e);
					}
					break;
				}
				default: {
					break;
				}
			}
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
		set_dirty(e, is_dirty, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE | DIRTY_RELATIVE_POSITION);
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
		set_dirty(e, is_dirty, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE | DIRTY_RELATIVE_POSITION);
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

	glm::vec2 get_world_position(const Handle& e) {
		assert_handle(e);
		if (!registry.has<World_Transform_2D>(e.id()) || !is_valid(e)) {
			return {0.f, 0.f};
		}
		if (has_node_ancestor_dirty(e, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE | DIRTY_RELATIVE_POSITION)) {
			resolve_transform(e);
		}
		const auto& [world_transfrom] = registry.read<World_Transform_2D>(e.id());
		return world_transfrom.position;
	}

	glm::vec2 to_my_position_space(
		const Handle& e, const glm::vec2& world_position, const glm::vec2& fallback_position
	) {
		assert_handle(e);
		if (!registry.has<World_Transform_2D>(e.id()) || !is_valid(e)) {
			return fallback_position;
		}
		if (has_node_ancestor_dirty(e, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE | DIRTY_RELATIVE_POSITION)) {
			resolve_transform(e);
		}
		auto world_transfrom = get_relative_world_transform(e);
		if (std::abs(world_transfrom.scale.x) <= ::Const::EPSILON ||
			std::abs(world_transfrom.scale.y) <= ::Const::EPSILON) {
			return fallback_position;
		}
		return Math::rotate(-world_transfrom.rotation, world_position - world_transfrom.position) *
			   glm::vec2(1.f / world_transfrom.scale.x, 1.f / world_transfrom.scale.y);
	}

	std::array<glm::vec2, 4> get_world_rect(const Handle& e) {
		assert_handle(e);
		if (!registry.has_all<World_Transform_2D, Rect>(e.id()) || !is_valid(e)) {
			return {};
		}
		if (!registry.has_all<Size, Rect>(e.id())) {
			std::array<glm::vec2, 4> rect;
			glm::vec2 world_position = get_world_position(e);
			for (int i = 0; i < rect.size(); i++) {
				rect[i] = world_position;
			}
			return rect;
		}
		if (has_node_ancestor_dirty(e, DIRTY_TRANSFORM | DIRTY_RELATIVE_SIZE | DIRTY_RELATIVE_POSITION)) {
			resolve_transform(e);
		}
		const auto& [c_rect] = registry.read<Rect>(e.id());
		return c_rect.transformed_rect;
	}

	bool is_world_visible(const Handle& e) {
		if (!registry.has_all<World_Transform_2D>(e.id()) || !is_valid(e)) {
			return false;
		}
		if (has_node_ancestor_dirty(e, DIRTY_TRANSFORM)) {
			resolve_transform(e);
		}
		const auto& [transform] = registry.read<World_Transform_2D>(e.id());
		return transform.is_visible;
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
		image_renderer.texture_changing = image_renderer.image_path != image_path;
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

	void set_image_fit_mode(const Handle& e, IMAGE_RENDER_FIT_MODE fit_mode) {
		assert_handle(e);
		if (!registry.has<Image_Renderer_Information>(e.id())) {
			return;
		}
		auto [image_renderer] = registry.get<Image_Renderer_Information>(e.id());
		set_dirty(e, image_renderer.fit_mode != fit_mode, DIRTY_RENDERER);
		image_renderer.fit_mode = fit_mode;
	}

	IMAGE_RENDER_FIT_MODE get_image_fit_mode(const Handle& e) {
		if (!registry.has<Image_Renderer_Information>(e.id())) {
			return IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_AUTO;
		}
		const auto& [image_renderer] = registry.read<Image_Renderer_Information>(e.id());
		return image_renderer.fit_mode;
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
		set_touch_enabled(scroll_info.container_touch_handle, enabled);
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
		set_dirty(e, scroll_info.size != size, DIRTY_RENDERER | DIRTY_TRANSFORM);
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
		if (has_node_ancestor_dirty(e, DIRTY_RENDERER)) {
			switch (description.value) {
				case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
					update_scroll_layout_renderer(e);
					resolve_relative_size(e);
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
			return 0.f;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		return scroll_info.scroll_speed;
	}

	void set_scroll_dragging_threshold(const Handle& e, float threshold) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		scroll_info.dragging_threshold = threshold;
	}

	float get_scroll_dragging_threshold(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return 0.f;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		return scroll_info.dragging_threshold;
	}

	void set_layout_scroll_touch_enabled(const Handle& e, bool enabled) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		set_touch_enabled(scroll_info.container_touch_handle, enabled);
	}

	bool is_layout_scroll_touch_enabled(const Handle& e) {
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
		scroll_info.swallow_scroll = swallows;
	}

	bool is_swallow_scrolling(const Handle& e) {
		assert_handle(e);
		if (!registry.has<Scroll_Information>(e.id())) {
			return false;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		return scroll_info.swallow_scroll;
	}

	void set_scroll_layout_container_position(const Handle& e, const glm::vec2& inner_container_position) {
		if (!registry.has<Scroll_Information>(e.id())) {
			return;
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		auto [container_transform] = registry.get<Transform_2D>(scroll_info.container_handle.id());
		set_dirty(e, container_transform.position != inner_container_position, DIRTY_TRANSFORM);
		container_transform.position = inner_container_position;
	}

	glm::vec2 get_scroll_layout_container_position(const Handle& e) {
		if (!registry.has<Scroll_Information>(e.id())) {
			return {0.f, 0.f};
		}
		const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
		if (registry.has<Description>(e.id())) {
			const auto& [description] = registry.read<Description>(e.id());
			switch (description.value) {
				case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
					if (is_valid(e)) {
						if (is_dirty(e, DIRTY_RELATIVE_SIZE) ||
							is_dirty(scroll_info.container_handle, DIRTY_RELATIVE_SIZE)) {
							resolve_relative_size(scroll_info.container_handle);
						}
						const auto& [c_size] = registry.read<Size>(e.id());
						const auto& [c_container_size] = registry.read<Size>(scroll_info.container_handle.id());
						auto [container_transform] = registry.get<Transform_2D>(scroll_info.container_handle.id());
						container_transform.position = Utils::fit_container_position_with(
							c_size.value, c_container_size.value, container_transform.position
						);
					}
					break;
				}
				default: {
					break;
				}
			}
		}
		auto [container_transform] = registry.get<Transform_2D>(scroll_info.container_handle.id());
		return container_transform.position;
	}

	Handle find_node_by_name(const std::string& name, Handle parent) {
		if (parent.is_invalid()) {
			parent = get_root();
		}
		bool found = false;
		Handle found_entity = Handle::make_invalid_handle();
		travel_down(
			parent,
			[&found, &name, &found_entity](const Handle&, const Handle& child) {
				const auto& [tree_info] = registry.read<Tree_Information>(child.id());
				if (tree_info.name == name) {
					found_entity = child;
					found = true;
				}
				return found;
			},
			TRAVEL_TYPE::TRAVEL_BFS
		);
		return found_entity;
	}

	Handle find_node_by_tag(int tag, Handle parent) {
		if (parent.is_invalid()) {
			parent = get_root();
		}
		bool found = false;
		Handle found_entity = Handle::make_invalid_handle();
		travel_down(
			parent,
			[&found, &tag, &found_entity](const Handle&, const Handle& child) {
				const auto& [tree_info] = registry.read<Tree_Information>(child.id());
				if (tree_info.tag == tag) {
					found_entity = child;
					found = true;
				}
				return found;
			},
			TRAVEL_TYPE::TRAVEL_BFS
		);
		return found_entity;
	}

	void set_label_horizontal_align(const Handle& e, LABEL_HORIZONTAL_ALIGN horizontal_align) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.horizontal_align != horizontal_align, DIRTY_TRANSFORM | DIRTY_RENDERER);
		label_renderer.horizontal_align = horizontal_align;
	}

	LABEL_HORIZONTAL_ALIGN get_label_horizontal_align(const Handle& e) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_CENTER;
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.horizontal_align;
	}

	void set_label_vertical_align(const Handle& e, LABEL_VERTICAL_ALIGN vertical_align) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.vertical_align != vertical_align, DIRTY_TRANSFORM);
		label_renderer.vertical_align = vertical_align;
	}

	LABEL_VERTICAL_ALIGN get_label_vertical_align(const Handle& e) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_CENTER;
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.vertical_align;
	}

	void set_label_padding_horizontal(const Handle& e, float padding_horizontal) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.horizontal_padding != padding_horizontal, DIRTY_TRANSFORM);
		label_renderer.horizontal_padding = padding_horizontal;
	}

	float get_label_padding_horizontal(const Handle& e) {
		if (registry.has<Label_Renderer_Information>(e.id())) {
			return 0.f;
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.horizontal_padding;
	}

	void set_label_padding_vertical(const Handle& e, float padding_vertical) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.vertical_padding != padding_vertical, DIRTY_TRANSFORM);
		label_renderer.vertical_padding = padding_vertical;
	}

	float get_label_padding_vertical(const Handle& e) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return 0.f;
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.vertical_padding;
	}

	void set_label_render_fit_mode(const Handle& e, LABEL_RENDER_FIT_MODE fit_mode) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return;
		}
		auto [label_renderer] = registry.get<Label_Renderer_Information>(e.id());
		set_dirty(e, label_renderer.fit_mode != fit_mode, DIRTY_TRANSFORM);
		label_renderer.fit_mode = fit_mode;
	}

	LABEL_RENDER_FIT_MODE get_label_render_fit_mode(const Handle& e) {
		if (!registry.has<Label_Renderer_Information>(e.id())) {
			return LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_CLIP;
		}
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		return label_renderer.fit_mode;
	}

	int get_min_font_size(const std::string& path) {
		return Vulkan::get_max_font_size(path);
	}

	int get_max_font_size(const std::string& path) {
		return Vulkan::get_min_font_size(path);
	}

	void set_clear_screen_color(const glm::vec3& color, float opacity) {
		Vulkan::set_clear_screen_color(color, opacity);
	}

	glm::vec2 query_image_size(const std::string& path) {
		std::string full_path = ::Utils::get_root_path() + path;
		if (!::Files::is_file_exists(full_path)) {
			return {0.f, 0.f};
		}
		Vulkan::Texture_Helper texture_helper = Vulkan::Draw_Texture_2D::gen_texture_info_from(path);
		return texture_helper.get_size();
	}

	void claim_touch(const Handle& e) {
		if (!registry.has<Touching_Information>(e.id())) {
			return;
		}
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		if (touch_info.enabled && is_touch_entity_handling(e)) {
			touch_info.claim_touch = true;
		}
	}

	glm::vec2 get_cursor_world_position() {
		return cursor_world_position;
	}

	void set_touch_cancel_on_fast_move(const Handle& e, bool enabled) {
		assert_handle(e);
		if (!registry.has<Touching_Information>(e.id())) {
			return;
		}
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		touch_info.canceling_touch_enabled = enabled;
	}

	Handle find_topmost_node_at(const glm::vec2& world_position, Handle under) {
		Handle found = Handle::make_invalid_handle();
		travel_down(
			under,
			[&found, &world_position, &under](const Handle&, const Handle& child) {
				if (child.id() == under.id())
					return false;
				if (is_world_position_in_entity_rect(child, world_position)) {
					found = child;
				}
				return !found.is_invalid();
			},
			TRAVEL_TYPE::TRAVEL_DFS, true
		);
		return found;
	}

	SIZE_TYPE get_size_type(const Handle& e) {
		if (registry.has<Size_Pinning>(e.id())) {
			const auto& [size_pinning] = registry.read<Size_Pinning>(e.id());
			if (size_pinning.enabled) {
				return SIZE_TYPE::SIZE_TYPE_PINNING_RELATIVE;
			}
		}
		if (registry.has<Size_Percent>(e.id())) {
			const auto& [size_percent] = registry.read<Size_Percent>(e.id());
			if (size_percent.enabled) {
				return SIZE_TYPE::SIZE_TYPE_PERCENT_RELATIVE;
			}
		}
		return SIZE_TYPE::SIZE_TYPE_ABSOLUTE;
	}

	POSITION_TYPE get_position_type(const Handle& e) {
		if (registry.has<Position_Percent>(e.id())) {
			const auto& [position_percent] = registry.read<Position_Percent>(e.id());
			if (position_percent.enabled) {
				return POSITION_TYPE::POSITION_TYPE_PERCENT_RELATIVE;
			}
		}
		return POSITION_TYPE::POSITION_TYPE_ABSOLUTE;
	}

	DESCRIPTION_TYPE get_description_type(const Handle& e) {
		if (registry.has<Description>(e.id())) {
			const auto& [description] = registry.read<Description>(e.id());
			return description.value;
		}
		return DESCRIPTION_TYPE::NONE;
	}

} // namespace Engine