#include <editor_internal.h>

namespace Editor {

	namespace {
		Handle tree_drag_node = Handle::make_invalid_handle();
		bool tree_dragging = false;
		glm::vec2 tree_drag_start{0.f};

		Tree_Row* row_at(glm::vec2 world) {
			for (auto& row : state.tree_rows) {
				if (Editor_Api::is_point_in_node(row.row, world))
					return &row;
			}
			return nullptr;
		}

		void paint_rows(const Handle& target) {
			for (const auto& row : state.tree_rows) {
				glm::vec3 color = PANEL_BG;
				if (Engine::is_valid(state.selected) && row.node.id() == state.selected.id())
					color = ROW_SELECTED;
				if (Engine::is_valid(target) && row.node.id() == target.id())
					color = ROW_TARGET;
				Engine::set_layout_background_color(row.row, color);
			}
		}

		const char* kind_tag(Kind kind) {
			switch (kind) {
				case Kind::LAYOUT:
					return "[L] ";
				case Kind::IMAGE:
					return "[I] ";
				case Kind::LABEL:
					return "[T] ";
				default:
					return "[N] ";
			}
		}

		void attach_row_touch(const Tree_Row& tree_row) {
			Handle node = tree_row.node;
			Engine::set_touch_enabled(tree_row.row, true);
			Engine::set_touch_cancel_on_fast_move(tree_row.row, false);
			Engine::set_touch_caller(tree_row.row, [node](Handle row, Engine::TOUCH_STATE touch_state, glm::vec2 world, int, int) {
				switch (touch_state) {
					case Engine::TOUCH_STATE::TOUCH_BEGAN: {
						tree_drag_node = node;
						tree_dragging = false;
						tree_drag_start = world;
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_MOVE: {
						if (!tree_dragging && glm::distance(world, tree_drag_start) >= DRAG_START_DISTANCE) {
							tree_dragging = true;
							// the row keeps the touch: the tree's scroll layout must not start scrolling.
							Engine::claim_touch(row);
						}
						if (tree_dragging) {
							Tree_Row* target = row_at(world);
							paint_rows(target ? target->node : Handle::make_invalid_handle());
						}
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_ENDED: {
						if (!tree_dragging) {
							select_node(node);
							break;
						}
						tree_dragging = false;
						Tree_Row* target = row_at(world);
						bool moved = false;
						if (target != nullptr) {
							moved = reparent(node, target->node);
						} else if (Editor_Api::is_point_in_node(state.tree_scroll, world)) {
							moved = reparent(node, state.canvas); // empty part of the tree = canvas root
						} else if (Editor_Api::is_point_in_node(state.canvas, world)) {
							Handle layout = topmost_layout_at(world);
							moved = reparent(node, layout.is_invalid() ? state.canvas : layout);
						}
						if (!moved) {
							paint_rows(Handle::make_invalid_handle());
							set_status("Drop on another row to make it the parent, on empty space for the canvas root");
						}
						break;
					}
					default: {
						tree_dragging = false;
						paint_rows(Handle::make_invalid_handle());
						break;
					}
				}
			});
		}
	} // namespace

	// ---------------------------------------------------------------- Scene tree
	void build_tree_panel(const Handle& parent, glm::vec2 position, glm::vec2 size) {
		Handle panel = make_panel(parent, position, size, PANEL_BG, 0.f);
		make_text(panel, "Scene", 16, TEXT, {14.f, size.y - 18.f});
		make_text(panel, "drag a row onto a row = reparent", 11, MUTED, {90.f, size.y - 18.f});
		state.tree_view = {size.x - 16.f, size.y - 44.f};
		state.tree_scroll = Engine::create_scroll_layout();
		Engine::set_content_size(state.tree_scroll, state.tree_view);
		Engine::set_position(state.tree_scroll, {8.f, 8.f});
		Engine::set_layout_background_enabled(state.tree_scroll, true);
		Engine::set_layout_background_color(state.tree_scroll, PANEL_BG);
		Engine::set_scroll_direction(state.tree_scroll, Engine::SCROLL_DIRECTION::SCROLL_VERTICAL);
		Engine::set_scroll_speed(state.tree_scroll, 30.f);
		Engine::add_child(panel, state.tree_scroll);
	}

	void rebuild_tree() {
		if (!Engine::is_valid(state.tree_scroll))
			return;
		for (const auto& row : state.tree_rows) {
			if (Engine::is_valid(row.row))
				Engine::clean_up(row.row);
		}
		state.tree_rows.clear();
		std::vector<std::pair<Handle, int>> nodes;
		for_each_canvas_node([&](const Handle& e, int depth) { nodes.push_back({e, depth}); });

		const glm::vec2 view = state.tree_view;
		const float content = std::max(view.y, nodes.size() * ROW_HEIGHT + 4.f);
		Engine::set_inner_container_size(state.tree_scroll, {view.x, content});
		Engine::set_scroll_layout_container_position(state.tree_scroll, {0.f, view.y - content});
		for (size_t i = 0; i < nodes.size(); i++) {
			const auto& [node, depth] = nodes[i];
			Tree_Row tree_row;
			tree_row.node = node;
			tree_row.row = Engine::create_layout();
			Engine::set_content_size(tree_row.row, {view.x - 4.f, ROW_HEIGHT - 2.f});
			Engine::set_position(tree_row.row, {2.f, content - (i + 1) * ROW_HEIGHT});
			Engine::set_layout_background_enabled(tree_row.row, true);
			Engine::set_layout_border_radius(tree_row.row, 4.f);
			Engine::add_child_to_container(state.tree_scroll, tree_row.row);
			float indent = 8.f + depth * 16.f;
			make_fitted_text(tree_row.row, std::string(kind_tag(kind_of(node))) + Engine::get_name(node), 13,
							 Engine::is_visible(node) ? TEXT : MUTED, {indent, 1.f},
							 {view.x - 12.f - indent, ROW_HEIGHT - 4.f}, Engine::LABEL_HORIZONTAL_ALIGN_LEFT);
			attach_row_touch(tree_row);
			state.tree_rows.push_back(tree_row);
		}
		paint_rows(Handle::make_invalid_handle());
	}

} // namespace Editor
