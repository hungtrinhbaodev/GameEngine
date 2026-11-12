#include <cstdio>
#include <editor.h>
#include <editor_internal.h>
#include <filesystem>
#include <utils.h>

namespace fs = std::filesystem;

namespace Editor {

	namespace {
		// new nodes go inside the selected layout, else on the canvas, at the parent's centre.
		Handle creation_parent() {
			if (Engine::is_valid(state.selected) && kind_of(state.selected) == Kind::LAYOUT)
				return state.selected;
			return state.canvas;
		}

		void place_new_node(const Handle& node, Kind kind) {
			Handle parent = creation_parent();
			Engine::set_anchor(node, {0.5f, 0.5f});
			Engine::set_position(node, Engine::get_content_size(parent) * 0.5f);
			Engine::set_z_order(node, state.next_z++);
			Engine::add_child(parent, node);
			register_node(node, kind);
			select_node(node);
			rebuild_tree();
			commit("Added " + Engine::get_name(node));
		}

		std::string run_dialog(const std::string& script) {
#ifdef __APPLE__
			std::string command = "osascript -e '" + script + "' 2>/dev/null";
			FILE* pipe = popen(command.c_str(), "r");
			if (pipe == nullptr)
				return "";
			char line[4096];
			std::string result;
			if (std::fgets(line, sizeof(line), pipe))
				result = line;
			pclose(pipe);
			while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
				result.pop_back();
			return result;
#else
			(void)script;
			return "";
#endif
		}

		std::string default_scene_folder() {
#ifdef EDITOR_SOURCE_DIR
			return std::string(EDITOR_SOURCE_DIR) + "/res/scenes";
#else
			return Utils::get_root_path() + "res/scenes";
#endif
		}

		void save_with_dialog(bool save_as) {
			std::string path = state.scene_path;
			if (path.empty() || save_as) {
				std::error_code error;
				fs::create_directories(default_scene_folder(), error);
				path = run_dialog("POSIX path of (choose file name with prompt \"Save scene\" default name \"scene.json\" "
								  "default location (POSIX file \"" + default_scene_folder() + "\"))");
			}
			if (!path.empty())
				save_to(path);
		}

		void open_with_dialog() {
			std::string path = run_dialog("POSIX path of (choose file with prompt \"Open scene\" default location (POSIX file \"" +
										  default_scene_folder() + "\"))");
			if (!path.empty())
				open_from(path);
		}

		void build() {
			const glm::vec2 window = Engine::get_window_size();
			const Handle& root = Engine::get_root();
			// the clear colour is the page background: no full-window rect (GPU, see --perf).
			// TODO(engine finding 40): the engine root has a grey full-window background (init_root), which hides
			// the clear colour and costs a full-window layer. The editor turns it off.
			Engine::set_layout_background_enabled(root, false);
			Engine::set_clear_screen_color(PAGE_BG);
			state.page = Engine::create_layout();
			Engine::set_content_size(state.page, window);
			Engine::add_child(root, state.page);

			// toolbar
			Handle top = make_panel(state.page, {0.f, window.y - TOP_BAR}, {window.x, TOP_BAR}, BAR_BG, 0.f);
			make_text(top, "Scene Editor", 18, TEXT, {16.f, TOP_BAR * 0.5f});
			struct Tool {
				const char* text;
				float width;
				glm::vec3 color;
				std::function<void()> action;
			};
			const std::vector<Tool> tools = {
				{"Open", 70.f, BUTTON, [] { open_with_dialog(); }},
				{"Save", 70.f, BUTTON, [] { save_with_dialog(false); }},
				{"Undo", 70.f, BUTTON, [] { undo(); }},
				{"Redo", 70.f, BUTTON, [] { redo(); }},
				{"+ Layout", 96.f, ACCENT, [] { add_layout(); }},
				{"+ Label", 90.f, ACCENT, [] { add_label(); }},
			};
			float x = 180.f;
			for (const auto& tool : tools) {
				make_button(top, tool.text, {x, 8.f}, {tool.width, 32.f}, tool.color, tool.action);
				x += tool.width + 8.f;
			}
			make_button(top, "Import...", {window.x - 126.f, 8.f}, {110.f, 32.f}, BUTTON,
						[] { import_paths(pick_files_with_dialog(), false); });

			// status bar
			Handle bottom = make_panel(state.page, {0.f, 0.f}, {window.x, STATUS_BAR}, BAR_BG, 0.f);
			state.status = make_fitted_text(bottom, "", 13, MUTED, {12.f, 4.f}, {window.x - 24.f, 20.f},
											Engine::LABEL_HORIZONTAL_ALIGN_LEFT);

			const float middle = window.y - TOP_BAR - STATUS_BAR;
			// left column: Scene over FileSystem (1 px gap shows the clear colour as a separator)
			build_tree_panel(state.page, {0.f, STATUS_BAR + middle - SCENE_HEIGHT}, {LEFT_WIDTH, SCENE_HEIGHT});
			build_resource_panel(state.page, {0.f, STATUS_BAR}, {LEFT_WIDTH, middle - SCENE_HEIGHT - 1.f});
			// right: Inspector
			build_inspector_panel(state.page, {window.x - RIGHT_WIDTH, STATUS_BAR}, {RIGHT_WIDTH, middle});

			// canvas
			glm::vec2 area_min{LEFT_WIDTH, STATUS_BAR};
			glm::vec2 area_size{window.x - LEFT_WIDTH - RIGHT_WIDTH, middle};
			state.canvas = make_panel(state.page, area_min + area_size * 0.5f, area_size - glm::vec2(48.f), CANVAS_BG, 6.f,
									  {0.5f, 0.5f});
			Engine::set_name(state.canvas, "canvas");
			Engine::set_layout_clipping_enabled(state.canvas, true);
			set_canvas_drop_highlight(false);
			Engine::set_touch_enabled(state.canvas, true);
			Engine::set_touch_caller(state.canvas, [](Handle, Engine::TOUCH_STATE touch_state, glm::vec2, int, int) {
				if (touch_state == Engine::TOUCH_STATE::TOUCH_BEGAN)
					select_node(Handle::make_invalid_handle());
			});

			// selection frame: 4 lines (a layout's border is not drawn without its background).
			state.frame = Engine::create_layout();
			Engine::set_z_order(state.frame, Z_FRAME);
			Engine::add_child(state.canvas, state.frame);
			for (auto& line : state.frame_lines)
				line = make_panel(state.frame, {0.f, 0.f}, {1.f, 1.f}, ACCENT, 0.f);
			Engine::set_visible(state.frame, false);

			// drag preview, above everything.
			state.ghost = Engine::create_image();
			Engine::set_anchor(state.ghost, {0.5f, 0.5f});
			Engine::set_opacity(state.ghost, 0.6f);
			Engine::set_z_order(state.ghost, Z_GHOST);
			Engine::set_visible(state.ghost, false);
			Engine::add_child(root, state.ghost);

			refresh_resources();
			rebuild_tree();
			rebuild_inspector();
			reset_history();
			set_status("Resources: " + std::to_string(state.resources.size()) + " images under res/. Cmd+S save, Cmd+Z undo.");
		}

		bool center_out(const Handle& e, float out[2]) {
			if (!Engine::is_valid(e))
				return false;
			glm::vec2 c = center_of(e);
			out[0] = c.x;
			out[1] = c.y;
			return true;
		}
	} // namespace

	void init(GLFWwindow* window) {
		state = State{};
		state.window = window;
		build();
	}

	// once per frame: labels / images get their size in the renderer pass, so the frame follows it.
	void tick() {
		if (Engine::is_valid(state.selected))
			update_frame();
	}

	void add_layout() {
		Handle node = Engine::create_layout();
		Engine::set_content_size(node, {220.f, 140.f});
		Engine::set_layout_background_enabled(node, true);
		Engine::set_layout_background_color(node, {1.f, 1.f, 1.f});
		Engine::set_layout_border_radius(node, 10.f);
		Engine::set_layout_border_width(node, 2.f);
		Engine::set_layout_border_color(node, {0.75f, 0.78f, 0.83f});
		Engine::set_name(node, unique_name("Layout"));
		place_new_node(node, Kind::LAYOUT);
	}

	void add_label() {
		Handle node = Engine::create_label();
		Engine::set_label_font_path(node, FONT_PATH);
		Engine::set_label_font_size(node, 28);
		Engine::set_label_string(node, "Label");
		Engine::set_color(node, INK);
		Engine::set_name(node, unique_name("Label"));
		place_new_node(node, Kind::LABEL);
	}

	bool select_by_name(const std::string& name) {
		Handle node = find_canvas_node(name);
		select_node(node);
		return !node.is_invalid();
	}

	void on_key(int key, int mods) {
		bool command = (mods & (GLFW_MOD_SUPER | GLFW_MOD_CONTROL)) != 0;
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		float step = shift ? 10.f : 1.f;
		auto nudge = [](glm::vec2 delta) {
			if (!Engine::is_valid(state.selected))
				return;
			Engine::set_position(state.selected, Engine::get_position(state.selected) + delta);
			update_frame();
			refresh_inspector_values();
			commit("Nudged " + Engine::get_name(state.selected));
		};
		switch (key) {
			case GLFW_KEY_DELETE:
			case GLFW_KEY_BACKSPACE:
				delete_selected();
				break;
			case GLFW_KEY_ESCAPE:
				select_node(Handle::make_invalid_handle());
				break;
			case GLFW_KEY_D:
				if (command)
					duplicate_selected();
				break;
			case GLFW_KEY_Z:
				if (command)
					shift ? redo() : undo();
				break;
			case GLFW_KEY_Y:
				if (command)
					redo();
				break;
			case GLFW_KEY_S:
				if (command)
					save_with_dialog(shift);
				break;
			case GLFW_KEY_O:
				if (command)
					open_with_dialog();
				break;
			case GLFW_KEY_LEFT:
				nudge({-step, 0.f});
				break;
			case GLFW_KEY_RIGHT:
				nudge({step, 0.f});
				break;
			case GLFW_KEY_UP:
				nudge({0.f, step});
				break;
			case GLFW_KEY_DOWN:
				nudge({0.f, -step});
				break;
			default:
				break;
		}
	}

	void on_files_dropped(const std::vector<std::string>& paths) {
		import_paths(paths, true);
	}

	void destroy() {
		if (Engine::is_valid(state.ghost))
			Engine::clean_up(state.ghost);
		if (Engine::is_valid(state.page))
			Engine::clean_up(state.page);
		state = State{};
	}

	// ---------------------------------------------------------------- self-test views
	unsigned get_resource_count() {
		return (unsigned)state.resources.size();
	}

	std::string get_resource_path(unsigned index) {
		return index < state.resources.size() ? state.resources[index].path : "";
	}

	bool get_cell_center(unsigned index, float out[2]) {
		return index < state.cells.size() && center_out(state.cells[index], out);
	}

	bool get_canvas_center(float out[2]) {
		return center_out(state.canvas, out);
	}

	bool get_selected_center(float out[2]) {
		return center_out(state.selected, out);
	}

	bool get_selected_position(float out[2]) {
		if (!Engine::is_valid(state.selected))
			return false;
		glm::vec2 position = Engine::get_position(state.selected);
		out[0] = position.x;
		out[1] = position.y;
		return true;
	}

	bool get_node_center(const std::string& name, float out[2]) {
		return center_out(find_canvas_node(name), out);
	}

	bool get_tree_row_center(const std::string& name, float out[2]) {
		for (const auto& row : state.tree_rows) {
			if (Engine::is_valid(row.node) && Engine::get_name(row.node) == name)
				return center_out(row.row, out);
		}
		return false;
	}

	bool get_tree_empty_point(float out[2]) {
		if (!Engine::is_valid(state.tree_scroll))
			return false;
		auto rect = Engine::get_world_rect(state.tree_scroll);
		glm::vec2 lo = glm::min(glm::min(rect[0], rect[1]), glm::min(rect[2], rect[3]));
		out[0] = lo.x + 40.f;
		out[1] = lo.y + 10.f; // bottom of the tree view: below the rows while the scene is small
		return true;
	}

	bool get_field_center(const std::string& property, float out[2]) {
		for (const auto& field : state.fields) {
			if (field.property == property)
				return center_out(field.box, out);
		}
		return false;
	}

	std::string get_selected_name() {
		return Engine::is_valid(state.selected) ? Engine::get_name(state.selected) : "";
	}

	std::string get_parent_name(const std::string& name) {
		Handle node = find_canvas_node(name);
		if (node.is_invalid())
			return "<missing>";
		Handle parent = Engine::get_parent(node);
		return parent.id() == state.canvas.id() ? "" : Engine::get_name(parent);
	}

	float get_property(const std::string& name, const std::string& property) {
		Handle node = find_canvas_node(name);
		return node.is_invalid() ? -1e9f : get_property_value(node, property);
	}

	unsigned get_canvas_node_count() {
		unsigned count = 0;
		for_each_canvas_node([&count](const Handle&, int) { count++; });
		return count;
	}

	unsigned get_tree_row_count() {
		return (unsigned)state.tree_rows.size();
	}

	std::string get_scene_json() {
		return scene_json();
	}

	void set_ui_visible(bool visible) {
		if (Engine::is_valid(state.page))
			Engine::set_visible(state.page, visible);
	}

	void set_copy_imports_to_source(bool enabled) {
		state.copy_imports_to_source = enabled;
	}

} // namespace Editor
