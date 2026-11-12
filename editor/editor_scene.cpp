#include <algorithm>
#include <cctype>
#include <editor_internal.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace Editor {

	State state;

	// ---------------------------------------------------------------- kinds / lookup
	Kind kind_of(const Handle& e) {
		auto it = state.kinds.find(e.id());
		return it == state.kinds.end() ? Kind::NODE : it->second;
	}

	bool is_canvas_node(const Handle& e) {
		return Engine::is_valid(e) && state.kinds.count(e.id()) > 0;
	}

	// the selection frame lives on the canvas but is not part of the scene.
	bool skip_in_scene(const Handle& e) {
		return !is_canvas_node(e);
	}

	void for_each_canvas_node(const std::function<void(const Handle&, int depth)>& fn) {
		std::function<void(const Handle&, int)> walk = [&](const Handle& parent, int depth) {
			for (const auto& child : Engine::get_children(parent)) {
				if (!is_canvas_node(child))
					continue;
				fn(child, depth);
				walk(child, depth + 1);
			}
		};
		walk(state.canvas, 0);
	}

	Handle find_canvas_node(const std::string& name) {
		Handle found = Handle::make_invalid_handle();
		for_each_canvas_node([&](const Handle& e, int) {
			if (found.is_invalid() && Engine::get_name(e) == name)
				found = e;
		});
		return found;
	}

	std::string unique_name(const std::string& base) {
		std::string stem = base;
		// "Layout_12" -> "Layout": keep names short when duplicating.
		size_t underscore = stem.find_last_of('_');
		if (underscore != std::string::npos && underscore + 1 < stem.size() &&
			std::all_of(stem.begin() + underscore + 1, stem.end(), [](unsigned char c) { return std::isdigit(c); }))
			stem = stem.substr(0, underscore);
		if (stem.empty())
			stem = "Node";
		return stem + "_" + std::to_string(++state.node_counter);
	}

	// ---------------------------------------------------------------- canvas node touch
	void attach_node_touch(const Handle& node) {
		Engine::set_touch_enabled(node, true);
		Engine::set_touch_cancel_on_fast_move(node, false);
		Engine::set_touch_caller(node, [](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2 world, int, int) {
			Handle parent = Engine::get_parent(e);
			glm::vec2 local = Editor_Api::to_child_space(parent, world);
			switch (touch_state) {
				case Engine::TOUCH_STATE::TOUCH_BEGAN: {
					select_node(e);
					state.grab_offset = Engine::get_position(e) - local;
					state.drag_start = world;
					state.dragging = false;
					break;
				}
				case Engine::TOUCH_STATE::TOUCH_MOVE: {
					if (!Engine::is_valid(state.selected) || state.selected.id() != e.id())
						break;
					if (!state.dragging && glm::distance(world, state.drag_start) < 2.f)
						break;
					state.dragging = true;
					Engine::set_position(e, local + state.grab_offset);
					update_frame();
					refresh_inspector_values();
					break;
				}
				case Engine::TOUCH_STATE::TOUCH_ENDED: {
					if (state.dragging && Engine::is_valid(e))
						commit("Moved " + Engine::get_name(e) + " to " + vec_text(Engine::get_position(e)));
					state.dragging = false;
					break;
				}
				default: {
					state.dragging = false;
					break;
				}
			}
		});
	}

	void register_node(const Handle& e, Kind kind) {
		state.kinds[e.id()] = kind;
		state.next_z = std::max(state.next_z, Engine::get_z_order(e) + 1);
		attach_node_touch(e);
	}

	// ---------------------------------------------------------------- selection frame
	void update_frame() {
		if (!Engine::is_valid(state.selected)) {
			Engine::set_visible(state.frame, false);
			return;
		}
		// the frame is a canvas child: the node's world rect (any depth / scale) as a canvas-space box.
		auto rect = Engine::get_world_rect(state.selected);
		glm::vec2 lo{1e9f}, hi{-1e9f};
		for (const auto& corner : rect) {
			glm::vec2 local = Editor_Api::to_child_space(state.canvas, corner);
			lo = glm::min(lo, local);
			hi = glm::max(hi, local);
		}
		glm::vec2 size = glm::max(hi - lo, glm::vec2(1.f));
		Engine::set_visible(state.frame, true);
		Engine::set_anchor(state.frame, {0.f, 0.f});
		Engine::set_position(state.frame, lo);
		Engine::set_content_size(state.frame, size);
		const float t = 2.f;
		const glm::vec2 positions[4] = {{0.f, 0.f}, {0.f, size.y - t}, {0.f, 0.f}, {size.x - t, 0.f}};
		const glm::vec2 sizes[4] = {{size.x, t}, {size.x, t}, {t, size.y}, {t, size.y}};
		for (int i = 0; i < 4; i++) {
			Engine::set_position(state.frame_lines[i], positions[i]);
			Engine::set_content_size(state.frame_lines[i], sizes[i]);
		}
	}

	void select_node(const Handle& node) {
		// compare the handles themselves: a just-deleted selection is already invalid but still shown.
		bool changed = node.is_invalid() != state.selected.is_invalid() || node.id() != state.selected.id() ||
					   !Engine::is_valid(state.selected);
		state.selected = node;
		update_frame();
		if (changed) {
			rebuild_inspector();
			rebuild_tree();
		}
	}

	/**
	 * @Note: TODO(engine finding 39) Engine::find_topmost_node_at returns a parent over its own child and
	 * hits invisible nodes. Stand-in: walk the canvas in draw order, keep the last visible layout hit.
	 */
	Handle topmost_layout_at(glm::vec2 world) {
		Handle found = Handle::make_invalid_handle();
		for_each_canvas_node([&](const Handle& e, int) {
			if (kind_of(e) == Kind::LAYOUT && Engine::is_world_visible(e) && Editor_Api::is_point_in_node(e, world))
				found = e;
		});
		return found;
	}

	// ---------------------------------------------------------------- create / delete / duplicate
	glm::vec2 fit_scale(const std::string& path) {
		glm::vec2 texture = Engine::query_image_size(path);
		glm::vec2 canvas = Engine::get_content_size(state.canvas) * 0.8f;
		if (texture.x <= 0.f || texture.y <= 0.f)
			return {1.f, 1.f};
		float scale = std::min({1.f, canvas.x / texture.x, canvas.y / texture.y});
		return {scale, scale};
	}

	Handle create_canvas_image(const std::string& path, const Handle& parent, glm::vec2 local, glm::vec2 scale) {
		Handle node = Engine::create_image();
		Engine::set_image_path(node, path);
		Engine::set_anchor(node, {0.5f, 0.5f});
		Engine::set_position(node, local);
		Engine::set_scale(node, scale);
		Engine::set_z_order(node, state.next_z++);
		Engine::set_name(node, unique_name(fs::path(path).stem().string()));
		Engine::add_child(parent, node);
		register_node(node, Kind::IMAGE);
		return node;
	}

	bool drop_resource(const std::string& path, glm::vec2 world) {
		if (!Editor_Api::is_point_in_node(state.canvas, world))
			return false;
		Handle parent = topmost_layout_at(world);
		if (parent.is_invalid())
			parent = state.canvas;
		glm::vec2 local = Editor_Api::to_child_space(parent, world);
		Handle node = create_canvas_image(path, parent, local, fit_scale(path));
		select_node(node);
		rebuild_tree();
		commit("Added " + Engine::get_name(node) + (parent.id() == state.canvas.id() ? "" : " in " + Engine::get_name(parent)));
		return true;
	}

	void delete_selected() {
		if (!Engine::is_valid(state.selected))
			return;
		std::string name = Engine::get_name(state.selected);
		std::vector<uint32_t> ids;
		std::function<void(const Handle&)> collect = [&](const Handle& e) {
			ids.push_back(e.id());
			for (const auto& child : Engine::get_children(e))
				collect(child);
		};
		collect(state.selected);
		Handle node = state.selected;
		select_node(Handle::make_invalid_handle());
		Engine::clean_up(node);
		for (uint32_t id : ids)
			state.kinds.erase(id);
		rebuild_tree();
		commit("Deleted " + name);
	}

	void duplicate_selected() {
		if (!Engine::is_valid(state.selected))
			return;
		Handle source = state.selected;
		std::string text = Scene_Io::node_to_string(source, kind_of, skip_in_scene);
		Handle copy = Scene_Io::node_from_string(text, Engine::get_parent(source), [](const Handle& e, Kind kind) {
			Engine::set_name(e, unique_name(Engine::get_name(e)));
			register_node(e, kind);
		});
		if (copy.is_invalid())
			return;
		Engine::set_position(copy, Engine::get_position(source) + glm::vec2(20.f, -20.f));
		Engine::set_z_order(copy, state.next_z++);
		select_node(copy);
		rebuild_tree();
		commit("Duplicated as " + Engine::get_name(copy));
	}

	bool is_descendant(const Handle& node, const Handle& maybe_ancestor) {
		for (Handle e = node; Engine::is_valid(e); e = Engine::get_parent(e)) {
			if (e.id() == maybe_ancestor.id())
				return true;
			if (e.id() == state.canvas.id())
				break;
		}
		return false;
	}

	// keeps the node where it is on screen (position only: parents in the editor are not rotated).
	bool reparent(const Handle& node, const Handle& new_parent) {
		if (!Engine::is_valid(node) || !Engine::is_valid(new_parent))
			return false;
		if (is_descendant(new_parent, node) || Engine::get_parent(node).id() == new_parent.id())
			return false;
		glm::vec2 world = Engine::get_world_position(node);
		Engine::remove_child(node);
		Engine::add_child(new_parent, node);
		Engine::set_position(node, Editor_Api::to_child_space(new_parent, world));
		Engine::set_z_order(node, state.next_z++);
		update_frame();
		rebuild_tree();
		commit("Moved " + Engine::get_name(node) + " under " +
			   (new_parent.id() == state.canvas.id() ? std::string("canvas") : Engine::get_name(new_parent)));
		return true;
	}

	// ---------------------------------------------------------------- undo / redo / file
	std::string scene_json() {
		return Scene_Io::to_json(state.canvas, kind_of, skip_in_scene);
	}

	bool load_scene_json(const std::string& text, std::string& error) {
		// parse first: a broken file must not clear the current scene.
		if (!Scene_Io::is_valid_document(text, error))
			return false;
		std::string selected_name = Engine::is_valid(state.selected) ? Engine::get_name(state.selected) : "";
		for (const auto& child : Engine::get_children(state.canvas)) {
			if (is_canvas_node(child))
				Engine::clean_up(child);
		}
		state.kinds.clear();
		state.selected = Handle::make_invalid_handle();
		Scene_Io::from_json(text, state.canvas, register_node, error);
		Handle again = selected_name.empty() ? Handle::make_invalid_handle() : find_canvas_node(selected_name);
		state.selected = Handle::make_invalid_handle();
		select_node(again);
		rebuild_tree();
		rebuild_inspector();
		return true;
	}

	void reset_history() {
		state.history.clear();
		state.history.push_back(scene_json());
		state.history_index = 0;
	}

	// every finished edit: snapshot the scene (undo / redo walk these snapshots).
	void commit(const std::string& what) {
		std::string snapshot = scene_json();
		if (state.history_index >= 0 && state.history[state.history_index] == snapshot) {
			set_status(what);
			return;
		}
		state.history.resize(state.history_index + 1);
		state.history.push_back(snapshot);
		if (state.history.size() > 100)
			state.history.erase(state.history.begin());
		state.history_index = (int)state.history.size() - 1;
		set_status(what);
	}

	void undo() {
		if (state.history_index <= 0) {
			set_status("Nothing to undo");
			return;
		}
		std::string error;
		load_scene_json(state.history[--state.history_index], error);
		set_status("Undo (" + std::to_string(state.history_index) + " left)");
	}

	void redo() {
		if (state.history_index + 1 >= (int)state.history.size()) {
			set_status("Nothing to redo");
			return;
		}
		std::string error;
		load_scene_json(state.history[++state.history_index], error);
		set_status("Redo");
	}

	bool save_to(const std::string& path) {
		std::error_code error;
		fs::create_directories(fs::path(path).parent_path(), error);
		if (!Scene_Io::save_file(path, scene_json())) {
			set_status("Save failed: " + path);
			return false;
		}
		state.scene_path = path;
		set_status("Saved " + path);
		return true;
	}

	bool open_from(const std::string& path) {
		std::string text, error;
		if (!Scene_Io::load_file(path, text)) {
			set_status("Open failed: cannot read " + path);
			return false;
		}
		if (!load_scene_json(text, error)) {
			set_status("Open failed: " + error);
			return false;
		}
		state.scene_path = path;
		reset_history();
		set_status("Opened " + path);
		return true;
	}

} // namespace Editor
