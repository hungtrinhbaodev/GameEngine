#include <algorithm>
#include <cmath>
#include <cstdio>
#include <editor.h>
#include <editor_api.h>
#include <engine/engine.h>
#include <filesystem>
#include <functional>
#include <log.h>
#include <memory>
#include <utils.h>

namespace fs = std::filesystem;

namespace Editor {

	namespace {
		// ---------------------------------------------------------------- style
		const std::string FONT_PATH = "res/fonts/anonymous_pro_bold.ttf";
		const glm::vec3 PAGE_BG{0.11f, 0.12f, 0.14f};
		const glm::vec3 PANEL_BG{0.16f, 0.17f, 0.2f};
		const glm::vec3 BAR_BG{0.2f, 0.21f, 0.25f};
		const glm::vec3 CELL_BG{0.23f, 0.24f, 0.28f};
		const glm::vec3 CELL_SELECTED{0.24f, 0.36f, 0.6f};
		const glm::vec3 CANVAS_BG{0.92f, 0.93f, 0.95f};
		const glm::vec3 CANVAS_BORDER{0.35f, 0.37f, 0.42f};
		const glm::vec3 CANVAS_DROP{0.3f, 0.6f, 1.f};
		const glm::vec3 TEXT{0.9f, 0.91f, 0.93f};
		const glm::vec3 MUTED{0.6f, 0.63f, 0.68f};
		const glm::vec3 ACCENT{0.25f, 0.5f, 0.95f};
		const glm::vec3 DANGER{0.8f, 0.3f, 0.3f};
		const glm::vec3 BUTTON{0.32f, 0.34f, 0.4f};

		const float TOP_BAR = 48.f;
		const float STATUS_BAR = 28.f;
		const float LEFT_WIDTH = 300.f;
		const float RIGHT_WIDTH = 260.f;
		const float CELL_GAP = 8.f;
		const float CELL_HEIGHT = 140.f;
		const float DRAG_START_DISTANCE = 6.f;
		const uint32_t Z_FRAME = 1u << 24;
		const uint32_t Z_GHOST = 1u << 25;

		// ---------------------------------------------------------------- state
		struct Resource {
			std::string path; // relative to the root, e.g. "res/imported/cat.png"
			std::string name; // file name shown in the cell
			glm::vec2 size{0.f};
		};

		struct State {
			GLFWwindow* window = nullptr;
			Handle page = Handle::make_invalid_handle();
			Handle canvas = Handle::make_invalid_handle();
			Handle resource_scroll = Handle::make_invalid_handle();
			glm::vec2 resource_view{0.f};
			std::vector<Handle> cells;
			std::vector<Resource> resources;
			std::string selected_resource = "";

			Handle frame = Handle::make_invalid_handle();
			std::array<Handle, 4> frame_lines{};
			Handle selected = Handle::make_invalid_handle();

			Handle ghost = Handle::make_invalid_handle();
			bool dragging_resource = false;
			glm::vec2 drag_start{0.f};
			glm::vec2 grab_offset{0.f};

			Handle status = Handle::make_invalid_handle();
			std::array<Handle, 6> inspector_lines{};

			bool copy_imports_to_source = true;
			uint32_t next_z = 1000;
			uint32_t back_z = 999;
			int node_counter = 0;
		};
		State state;

		// ---------------------------------------------------------------- small UI kit
		Handle make_panel(const Handle& parent, glm::vec2 position, glm::vec2 size, glm::vec3 color, float radius,
						  glm::vec2 anchor = {0.f, 0.f}) {
			Handle layout = Engine::create_layout();
			Engine::set_content_size(layout, size);
			Engine::set_position(layout, position);
			Engine::set_anchor(layout, anchor);
			Engine::set_layout_background_enabled(layout, true);
			Engine::set_layout_background_color(layout, color);
			Engine::set_layout_border_radius(layout, radius);
			Engine::add_child(parent, layout);
			return layout;
		}

		Handle make_text(const Handle& parent, const std::string& text, int font_size, glm::vec3 color,
						 glm::vec2 position, glm::vec2 anchor = {0.f, 0.5f}) {
			Handle label = Engine::create_label();
			Engine::set_label_font_path(label, FONT_PATH);
			Engine::set_label_font_size(label, font_size);
			Engine::set_label_string(label, text);
			Engine::set_color(label, color);
			Engine::set_position(label, position);
			Engine::set_anchor(label, anchor);
			Engine::add_child(parent, label);
			return label;
		}

		// a label with a fixed box: long text ends with "..." instead of overflowing.
		Handle make_fitted_text(const Handle& parent, const std::string& text, int font_size, glm::vec3 color,
								glm::vec2 position, glm::vec2 size, Engine::LABEL_HORIZONTAL_ALIGN align) {
			Handle label = make_text(parent, text, font_size, color, position, {0.f, 0.f});
			Engine::set_content_size(label, size);
			Engine::set_label_render_fit_mode(label, Engine::ELLIPSIS);
			Engine::set_label_horizontal_align(label, align);
			Engine::set_label_vertical_align(label, Engine::LABEL_VERTICAL_ALIGN_CENTER);
			return label;
		}

		Handle make_button(const Handle& parent, const std::string& text, glm::vec2 position, glm::vec2 size,
						   glm::vec3 color, std::function<void()> on_click) {
			Handle button = make_panel(parent, position, size, color, 6.f);
			make_text(button, text, 15, TEXT, size * 0.5f, {0.5f, 0.5f});
			Engine::set_touch_enabled(button, true);
			Engine::set_touch_caller(button, [color](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2) {
				bool down = touch_state == Engine::TOUCH_STATE::TOUCH_BEGAN || touch_state == Engine::TOUCH_STATE::TOUCH_MOVE;
				Engine::set_layout_background_color(e, down ? color * 0.75f : color);
			});
			Engine::set_click_caller(button, [on_click](Handle, glm::vec2) { on_click(); });
			return button;
		}

		void set_status(const std::string& text) {
			Log::info("[editor]", text);
			if (Engine::is_valid(state.status))
				Engine::set_label_string(state.status, text);
		}

		std::string vec_text(glm::vec2 v) {
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%.0f, %.0f", v.x, v.y);
			return buffer;
		}

		std::string lower(std::string text) {
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
			return text;
		}

		bool is_image_file(const fs::path& path) {
			std::string ext = lower(path.extension().string());
			return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
		}

		// ---------------------------------------------------------------- resources
		std::vector<Resource> scan_resources() {
			std::vector<Resource> found;
			fs::path res = fs::path(Utils::get_root_path()) / "res";
			std::error_code error;
			for (fs::recursive_directory_iterator it(res, error), end; !error && it != end; it.increment(error)) {
				if (!it->is_regular_file() || !is_image_file(it->path()))
					continue;
				Resource resource;
				resource.path = "res/" + fs::relative(it->path(), res).generic_string();
				resource.name = it->path().filename().string();
				found.push_back(resource);
			}
			std::sort(found.begin(), found.end(), [](const Resource& a, const Resource& b) {
				return lower(a.path) < lower(b.path);
			});
			return found;
		}

		fs::path unique_target(const fs::path& folder, const fs::path& file_name) {
			fs::path target = folder / file_name;
			for (int i = 1; fs::exists(target); i++) {
				target = folder / (file_name.stem().string() + "_" + std::to_string(i) + file_name.extension().string());
			}
			return target;
		}

		/**
		 * @Note: copies an image into <build>/res/imported (what the engine loads now) and, when known,
		 * into <source>/res/imported too so the import survives the next build's resource sync.
		 * Returns the engine path ("res/imported/...") or "" when skipped.
		 */
		std::string import_file(const std::string& source_path) {
			fs::path source(source_path);
			if (!fs::is_regular_file(source) || !is_image_file(source))
				return "";
			std::error_code error;
			fs::path folder = fs::path(Utils::get_root_path()) / "res" / "imported";
			fs::create_directories(folder, error);
			fs::path target = unique_target(folder, source.filename());
			fs::copy_file(source, target, error);
			if (error) {
				set_status("Import failed: " + source.filename().string() + " (" + error.message() + ")");
				return "";
			}
#ifdef EDITOR_SOURCE_DIR
			if (!state.copy_imports_to_source)
				return "res/imported/" + target.filename().string();
			fs::path source_folder = fs::path(EDITOR_SOURCE_DIR) / "res" / "imported";
			std::error_code source_error;
			fs::create_directories(source_folder, source_error);
			if (!source_error)
				fs::copy_file(source, source_folder / target.filename(), fs::copy_options::skip_existing, source_error);
#endif
			return "res/imported/" + target.filename().string();
		}

		std::vector<std::string> pick_files_with_dialog() {
			std::vector<std::string> paths;
#ifdef __APPLE__
			const char* command =
				"osascript -e 'set picked to choose file of type {\"public.image\"} with prompt \"Import images\" "
				"with multiple selections allowed' -e 'set out to \"\"' -e 'repeat with f in picked' "
				"-e 'set out to out & POSIX path of f & linefeed' -e 'end repeat' -e 'return out' 2>/dev/null";
			FILE* pipe = popen(command, "r");
			if (pipe == nullptr)
				return paths;
			char line[4096];
			while (std::fgets(line, sizeof(line), pipe)) {
				std::string path = line;
				while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
					path.pop_back();
				if (!path.empty())
					paths.push_back(path);
			}
			pclose(pipe);
#else
			set_status("Import dialog is macOS only for now: drop files on the window instead.");
#endif
			return paths;
		}

		// ---------------------------------------------------------------- selection frame + inspector
		glm::vec2 node_size(const Handle& node) {
			glm::vec2 size = Engine::get_content_size(node);
			if (size.x <= 0.f || size.y <= 0.f)
				size = Editor_Api::get_texture_size(Engine::get_image_path(node));
			return size * Engine::get_scale(node);
		}

		void update_inspector() {
			auto set_line = [](int i, const std::string& text) {
				if (Engine::is_valid(state.inspector_lines[i]))
					Engine::set_label_string(state.inspector_lines[i], text);
			};
			if (!Engine::is_valid(state.selected)) {
				set_line(0, "Nothing selected");
				for (int i = 1; i < (int)state.inspector_lines.size(); i++)
					set_line(i, "");
				return;
			}
			const Handle& node = state.selected;
			glm::vec2 scale = Engine::get_scale(node);
			char scale_text[32];
			std::snprintf(scale_text, sizeof(scale_text), "%.2f", scale.x);
			set_line(0, "Name   " + Engine::get_name(node));
			set_line(1, "Image  " + fs::path(Engine::get_image_path(node)).filename().string());
			set_line(2, "Pos    " + vec_text(Engine::get_position(node)));
			set_line(3, "Size   " + vec_text(node_size(node)));
			set_line(4, std::string("Scale  ") + scale_text);
			set_line(5, "Z      " + std::to_string(Engine::get_z_order(node)));
		}

		void update_frame() {
			if (!Engine::is_valid(state.selected)) {
				Engine::set_visible(state.frame, false);
				update_inspector();
				return;
			}
			glm::vec2 size = node_size(state.selected);
			Engine::set_visible(state.frame, true);
			Engine::set_anchor(state.frame, Engine::get_anchor(state.selected));
			Engine::set_position(state.frame, Engine::get_position(state.selected));
			Engine::set_content_size(state.frame, size);
			const float t = 2.f;
			const glm::vec2 positions[4] = {{0.f, 0.f}, {0.f, size.y - t}, {0.f, 0.f}, {size.x - t, 0.f}};
			const glm::vec2 sizes[4] = {{size.x, t}, {size.x, t}, {t, size.y}, {t, size.y}};
			for (int i = 0; i < 4; i++) {
				Engine::set_position(state.frame_lines[i], positions[i]);
				Engine::set_content_size(state.frame_lines[i], sizes[i]);
			}
			update_inspector();
		}

		void select_node(const Handle& node) {
			state.selected = node;
			update_frame();
		}

		void delete_selected() {
			if (!Engine::is_valid(state.selected))
				return;
			std::string name = Engine::get_name(state.selected);
			Engine::clean_up(state.selected);
			select_node(Handle::make_invalid_handle());
			set_status("Deleted " + name);
		}

		// ---------------------------------------------------------------- canvas nodes
		Handle create_canvas_image(const std::string& path, glm::vec2 local_position, glm::vec2 scale);

		void attach_node_touch(const Handle& node) {
			Engine::set_touch_enabled(node, true);
			Engine::set_touch_caller(node, [](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2 world) {
				glm::vec2 local = Editor_Api::to_child_space(state.canvas, world);
				switch (touch_state) {
					case Engine::TOUCH_STATE::TOUCH_BEGAN: {
						select_node(e);
						state.grab_offset = Engine::get_position(e) - local;
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_MOVE: {
						if (!Engine::is_valid(state.selected) || state.selected.id() != e.id())
							break;
						Engine::set_position(e, local + state.grab_offset);
						update_frame();
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_ENDED: {
						if (Engine::is_valid(e))
							set_status("Moved " + Engine::get_name(e) + " to " + vec_text(Engine::get_position(e)));
						break;
					}
					default: {
						break;
					}
				}
			});
		}

		Handle create_canvas_image(const std::string& path, glm::vec2 local_position, glm::vec2 scale) {
			Handle node = Engine::create_image();
			Engine::set_image_path(node, path);
			Engine::set_anchor(node, {0.5f, 0.5f});
			Engine::set_position(node, local_position);
			Engine::set_scale(node, scale);
			Engine::set_z_order(node, state.next_z++);
			Engine::set_name(node, fs::path(path).stem().string() + "_" + std::to_string(++state.node_counter));
			Engine::add_child(state.canvas, node);
			attach_node_touch(node);
			return node;
		}

		// new nodes keep their pixel size, scaled down only when larger than the canvas.
		glm::vec2 fit_scale(const std::string& path) {
			glm::vec2 texture = Editor_Api::get_texture_size(path);
			glm::vec2 canvas = Engine::get_content_size(state.canvas) * 0.8f;
			if (texture.x <= 0.f || texture.y <= 0.f)
				return {1.f, 1.f};
			float scale = std::min({1.f, canvas.x / texture.x, canvas.y / texture.y});
			return {scale, scale};
		}

		bool drop_on_canvas(const std::string& path, glm::vec2 world) {
			if (!Editor_Api::is_point_in_node(state.canvas, world))
				return false;
			glm::vec2 local = Editor_Api::to_child_space(state.canvas, world);
			Handle node = create_canvas_image(path, local, fit_scale(path));
			select_node(node);
			set_status("Added " + Engine::get_name(node) + " at " + vec_text(local));
			return true;
		}

		void duplicate_selected() {
			if (!Engine::is_valid(state.selected))
				return;
			const Handle& source = state.selected;
			Handle copy = create_canvas_image(Engine::get_image_path(source), Engine::get_position(source) + glm::vec2(20.f, -20.f),
											  Engine::get_scale(source));
			select_node(copy);
			set_status("Duplicated as " + Engine::get_name(copy));
		}

		void scale_selected(float factor) {
			if (!Engine::is_valid(state.selected))
				return;
			glm::vec2 scale = Engine::get_scale(state.selected) * factor;
			scale = glm::clamp(scale, glm::vec2(0.05f), glm::vec2(20.f));
			Engine::set_scale(state.selected, scale);
			update_frame();
		}

		void nudge_selected(glm::vec2 delta) {
			if (!Engine::is_valid(state.selected))
				return;
			Engine::set_position(state.selected, Engine::get_position(state.selected) + delta);
			update_frame();
		}

		// ---------------------------------------------------------------- resource panel
		void refresh_resources();

		void set_canvas_drop_highlight(bool on) {
			Engine::set_layout_border_color(state.canvas, on ? CANVAS_DROP : CANVAS_BORDER);
			Engine::set_layout_border_width(state.canvas, on ? 4.f : 2.f);
		}

		void attach_cell_touch(const Handle& cell, const Resource& resource) {
			Engine::set_touch_enabled(cell, true);
			Engine::set_touch_caller(cell, [resource](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2 world) {
				switch (touch_state) {
					case Engine::TOUCH_STATE::TOUCH_BEGAN: {
						state.drag_start = world;
						state.dragging_resource = false;
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_MOVE: {
						if (!state.dragging_resource && glm::distance(world, state.drag_start) >= DRAG_START_DISTANCE) {
							state.dragging_resource = true;
							Engine::set_image_path(state.ghost, resource.path);
							glm::vec2 texture = Editor_Api::get_texture_size(resource.path);
							float scale = texture.x > 0.f ? std::min(1.f, 120.f / std::max(texture.x, texture.y)) : 1.f;
							Engine::set_scale(state.ghost, {scale, scale});
							Engine::set_visible(state.ghost, true);
						}
						if (state.dragging_resource) {
							Engine::set_position(state.ghost, world);
							set_canvas_drop_highlight(Editor_Api::is_point_in_node(state.canvas, world));
						}
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_ENDED:
					case Engine::TOUCH_STATE::TOUCH_CANCEL: {
						bool was_dragging = state.dragging_resource;
						state.dragging_resource = false;
						Engine::set_visible(state.ghost, false);
						set_canvas_drop_highlight(false);
						if (touch_state == Engine::TOUCH_STATE::TOUCH_CANCEL) {
							set_status("Drag cancelled");
							break;
						}
						if (was_dragging) {
							if (!drop_on_canvas(resource.path, world))
								set_status("Drop on the canvas to add " + resource.name);
						} else {
							state.selected_resource = resource.path;
							for (size_t i = 0; i < state.cells.size() && i < state.resources.size(); i++) {
								if (Engine::is_valid(state.cells[i]))
									Engine::set_layout_background_color(
										state.cells[i], state.resources[i].path == resource.path ? CELL_SELECTED : CELL_BG
									);
							}
							glm::vec2 size = Editor_Api::get_texture_size(resource.path);
							set_status(resource.path + "  (" + vec_text(size) + " px) - drag it onto the canvas");
						}
						break;
					}
					default: {
						break;
					}
				}
			});
		}

		void refresh_resources() {
			for (const auto& cell : state.cells) {
				if (Engine::is_valid(cell))
					Engine::clean_up(cell);
			}
			state.cells.clear();
			state.resources = scan_resources();

			const glm::vec2 view = state.resource_view;
			const float cell_width = (view.x - 3.f * CELL_GAP) / 2.f;
			const int rows = ((int)state.resources.size() + 1) / 2;
			const float content_height = std::max(view.y, CELL_GAP + rows * (CELL_HEIGHT + CELL_GAP));
			Engine::set_inner_container_size(state.resource_scroll, {view.x, content_height});
			// start at the top: the container's y offset is clamped to [view - content, 0].
			Engine::set_scroll_layout_container_position(state.resource_scroll, {0.f, view.y - content_height});

			for (size_t i = 0; i < state.resources.size(); i++) {
				Resource& resource = state.resources[i];
				int row = (int)i / 2, column = (int)i % 2;
				glm::vec2 position{CELL_GAP + column * (cell_width + CELL_GAP),
								   content_height - (row + 1) * (CELL_HEIGHT + CELL_GAP)};
				Handle cell = Engine::create_layout();
				Engine::set_content_size(cell, {cell_width, CELL_HEIGHT});
				Engine::set_position(cell, position);
				Engine::set_layout_background_enabled(cell, true);
				Engine::set_layout_background_color(cell, resource.path == state.selected_resource ? CELL_SELECTED : CELL_BG);
				Engine::set_layout_border_radius(cell, 8.f);
				Engine::set_layout_clipping_enabled(cell, true);
				Engine::add_child_to_container(state.resource_scroll, cell);
				state.cells.push_back(cell);

				resource.size = Editor_Api::get_texture_size(resource.path);
				const glm::vec2 thumb_box{cell_width - 16.f, CELL_HEIGHT - 36.f};
				Handle thumb = Engine::create_image();
				Engine::set_image_path(thumb, resource.path);
				Engine::set_anchor(thumb, {0.5f, 0.5f});
				Engine::set_position(thumb, {cell_width * 0.5f, 28.f + thumb_box.y * 0.5f});
				if (resource.size.x > 0.f && resource.size.y > 0.f) {
					float scale = std::min({1.f, thumb_box.x / resource.size.x, thumb_box.y / resource.size.y});
					Engine::set_scale(thumb, {scale, scale});
				}
				Engine::add_child(cell, thumb);
				make_fitted_text(cell, resource.name, 13, TEXT, {6.f, 4.f}, {cell_width - 12.f, 20.f},
								 Engine::LABEL_HORIZONTAL_ALIGN_CENTER);
				attach_cell_touch(cell, resource);
			}
			set_status("Resources: " + std::to_string(state.resources.size()) + " images under res/");
		}

		void import_paths(const std::vector<std::string>& paths, bool place_on_canvas) {
			std::vector<std::string> imported;
			for (const auto& path : paths) {
				std::string engine_path = import_file(path);
				if (!engine_path.empty())
					imported.push_back(engine_path);
			}
			if (imported.empty()) {
				set_status("Nothing imported (only .png / .jpg / .jpeg)");
				return;
			}
			refresh_resources();
			glm::vec2 cursor = Editor_Api::get_cursor_world_position();
			if (place_on_canvas && Editor_Api::is_point_in_node(state.canvas, cursor)) {
				for (size_t i = 0; i < imported.size(); i++)
					drop_on_canvas(imported[i], cursor + glm::vec2(24.f, -24.f) * (float)i);
			}
			set_status("Imported " + std::to_string(imported.size()) + " image(s) into res/imported");
		}

		// ---------------------------------------------------------------- build
		void build() {
			const glm::vec2 window = Engine::get_window_size();
			const Handle& root = Engine::get_root();

			/**
			 * @Note: GPU: no full-window background. Every opaque layer here costs ~0.45 ms per full window
			 * at 2880x1640 at full clock, and the panels + canvas already cover almost everything; only the
			 * margin around the canvas gets its own strips (see below). The render pass clear colour (grey)
			 * is not settable yet (ui_api_need.txt 27), so nothing may leave a gap.
			 */
			state.page = Engine::create_layout();
			Engine::set_content_size(state.page, window);
			Engine::add_child(root, state.page);

			// top bar
			Handle top = make_panel(state.page, {0.f, window.y - TOP_BAR}, {window.x, TOP_BAR}, BAR_BG, 0.f);
			make_text(top, "Scene Editor", 20, TEXT, {16.f, TOP_BAR * 0.5f});
			make_button(top, "Import...", {window.x - 240.f, 8.f}, {110.f, 32.f}, ACCENT, []() {
				import_paths(pick_files_with_dialog(), false);
			});
			make_button(top, "Refresh", {window.x - 120.f, 8.f}, {104.f, 32.f}, BUTTON, []() { refresh_resources(); });

			// status bar
			Handle bottom = make_panel(state.page, {0.f, 0.f}, {window.x, STATUS_BAR}, BAR_BG, 0.f);
			state.status = make_fitted_text(bottom, "", 13, MUTED, {12.f, 4.f}, {window.x - 24.f, 20.f},
											Engine::LABEL_HORIZONTAL_ALIGN_LEFT);

			const float middle_height = window.y - TOP_BAR - STATUS_BAR;

			// resource panel
			Handle left = make_panel(state.page, {0.f, STATUS_BAR}, {LEFT_WIDTH, middle_height}, PANEL_BG, 0.f);
			make_text(left, "Resources", 16, TEXT, {14.f, middle_height - 20.f});
			make_text(left, "drag onto canvas / drop files here", 11, MUTED, {14.f, middle_height - 38.f});
			state.resource_view = {LEFT_WIDTH - 16.f, middle_height - 58.f};
			state.resource_scroll = Engine::create_scroll_layout();
			Engine::set_content_size(state.resource_scroll, state.resource_view);
			Engine::set_position(state.resource_scroll, {8.f, 8.f});
			Engine::set_scroll_direction(state.resource_scroll, Engine::SCROLL_DIRECTION::SCROLL_VERTICAL);
			Engine::set_scroll_speed(state.resource_scroll, 30.f);
			// TODO(engine claim_touch): wheel-only scrolling so a cell drag never fights the scroll layout.
			Editor_Api::set_scroll_drag_enabled(state.resource_scroll, false);
			Engine::add_child(left, state.resource_scroll);

			// inspector
			Handle right =
				make_panel(state.page, {window.x - RIGHT_WIDTH, STATUS_BAR}, {RIGHT_WIDTH, middle_height}, PANEL_BG, 0.f);
			make_text(right, "Inspector", 16, TEXT, {14.f, middle_height - 20.f});
			for (int i = 0; i < (int)state.inspector_lines.size(); i++) {
				state.inspector_lines[i] = make_fitted_text(right, "", 13, i == 0 ? TEXT : MUTED,
															{14.f, middle_height - 64.f - i * 24.f}, {RIGHT_WIDTH - 28.f, 20.f},
															Engine::LABEL_HORIZONTAL_ALIGN_LEFT);
			}
			const float buttons_top = middle_height - 64.f - 6 * 24.f - 20.f;
			const glm::vec2 half{(RIGHT_WIDTH - 36.f) / 2.f, 32.f};
			auto column = [&](int c) { return 14.f + c * (half.x + 8.f); };
			auto row = [&](int r) { return buttons_top - r * (half.y + 8.f) - half.y; };
			make_button(right, "Delete", {column(0), row(0)}, half, DANGER, []() { delete_selected(); });
			make_button(right, "Duplicate", {column(1), row(0)}, half, BUTTON, []() { duplicate_selected(); });
			make_button(right, "To front", {column(0), row(1)}, half, BUTTON, []() {
				if (Engine::is_valid(state.selected)) {
					Engine::set_z_order(state.selected, state.next_z++);
					update_inspector();
				}
			});
			make_button(right, "To back", {column(1), row(1)}, half, BUTTON, []() {
				if (Engine::is_valid(state.selected) && state.back_z > 0) {
					Engine::set_z_order(state.selected, state.back_z--);
					update_inspector();
				}
			});
			make_button(right, "Scale -", {column(0), row(2)}, half, BUTTON, []() { scale_selected(1.f / 1.1f); });
			make_button(right, "Scale +", {column(1), row(2)}, half, BUTTON, []() { scale_selected(1.1f); });
			make_text(right, "Keys: Delete / Backspace remove", 11, MUTED, {14.f, 66.f});
			make_text(right, "Arrows move 1px (Shift 10px)", 11, MUTED, {14.f, 48.f});
			make_text(right, "Cmd/Ctrl+D duplicate, Esc deselect", 11, MUTED, {14.f, 30.f});

			// canvas
			glm::vec2 area_min{LEFT_WIDTH, STATUS_BAR};
			glm::vec2 area_size{window.x - LEFT_WIDTH - RIGHT_WIDTH, middle_height};
			const float margin = 24.f;
			// the margin around the canvas: 4 strips instead of a background under the canvas (no overdraw).
			make_panel(state.page, area_min, {area_size.x, margin}, PAGE_BG, 0.f);
			make_panel(state.page, {area_min.x, area_min.y + area_size.y - margin}, {area_size.x, margin}, PAGE_BG, 0.f);
			make_panel(state.page, {area_min.x, area_min.y + margin}, {margin, area_size.y - 2.f * margin}, PAGE_BG, 0.f);
			make_panel(state.page, {area_min.x + area_size.x - margin, area_min.y + margin},
					   {margin, area_size.y - 2.f * margin}, PAGE_BG, 0.f);
			// square corners: a rounded canvas would show the grey clear colour in its corners.
			state.canvas = make_panel(state.page, area_min + area_size * 0.5f, area_size - glm::vec2(2.f * margin),
									  CANVAS_BG, 0.f, {0.5f, 0.5f});
			Engine::set_layout_clipping_enabled(state.canvas, true);
			set_canvas_drop_highlight(false);
			Engine::set_touch_enabled(state.canvas, true);
			Engine::set_touch_caller(state.canvas, [](Handle, Engine::TOUCH_STATE touch_state, glm::vec2) {
				if (touch_state == Engine::TOUCH_STATE::TOUCH_BEGAN)
					select_node(Handle::make_invalid_handle());
			});

			// selection frame: 4 thin lines (a layout's border is not drawn without its background).
			state.frame = Engine::create_layout();
			Engine::set_z_order(state.frame, Z_FRAME);
			Engine::add_child(state.canvas, state.frame);
			for (auto& line : state.frame_lines) {
				line = make_panel(state.frame, {0.f, 0.f}, {1.f, 1.f}, ACCENT, 0.f);
			}
			Engine::set_visible(state.frame, false);

			// drag preview, above everything.
			state.ghost = Engine::create_image();
			Engine::set_anchor(state.ghost, {0.5f, 0.5f});
			Engine::set_opacity(state.ghost, 0.6f);
			Engine::set_z_order(state.ghost, Z_GHOST);
			Engine::set_visible(state.ghost, false);
			Engine::add_child(root, state.ghost);

			refresh_resources();
			update_inspector();
		}
	} // namespace

	void init(GLFWwindow* window) {
		state = State{};
		state.window = window;
		Editor_Api::init(window);
		build();
	}

	void on_key(int key, int mods) {
		bool command = (mods & (GLFW_MOD_SUPER | GLFW_MOD_CONTROL)) != 0;
		float step = (mods & GLFW_MOD_SHIFT) ? 10.f : 1.f;
		switch (key) {
			case GLFW_KEY_DELETE:
			case GLFW_KEY_BACKSPACE: {
				delete_selected();
				break;
			}
			case GLFW_KEY_ESCAPE: {
				select_node(Handle::make_invalid_handle());
				break;
			}
			case GLFW_KEY_D: {
				if (command)
					duplicate_selected();
				break;
			}
			case GLFW_KEY_LEFT: {
				nudge_selected({-step, 0.f});
				break;
			}
			case GLFW_KEY_RIGHT: {
				nudge_selected({step, 0.f});
				break;
			}
			case GLFW_KEY_UP: {
				nudge_selected({0.f, step});
				break;
			}
			case GLFW_KEY_DOWN: {
				nudge_selected({0.f, -step});
				break;
			}
			default: {
				break;
			}
		}
	}

	void on_files_dropped(const std::vector<std::string>& paths) {
		import_paths(paths, true);
	}

	namespace {
		bool center_of(const Handle& e, float out[2]) {
			if (!Engine::is_valid(e))
				return false;
			auto rect = Engine::get_world_rect(e);
			glm::vec2 center = (rect[0] + rect[1] + rect[2] + rect[3]) * 0.25f;
			out[0] = center.x;
			out[1] = center.y;
			return true;
		}
	} // namespace

	unsigned get_resource_count() {
		return (unsigned)state.resources.size();
	}

	std::string get_resource_path(unsigned index) {
		return index < state.resources.size() ? state.resources[index].path : "";
	}

	bool get_cell_center(unsigned index, float out[2]) {
		return index < state.cells.size() && center_of(state.cells[index], out);
	}

	bool get_canvas_center(float out[2]) {
		return center_of(state.canvas, out);
	}

	bool get_selected_center(float out[2]) {
		return center_of(state.selected, out);
	}

	bool get_selected_position(float out[2]) {
		if (!Engine::is_valid(state.selected))
			return false;
		glm::vec2 position = Engine::get_position(state.selected);
		out[0] = position.x;
		out[1] = position.y;
		return true;
	}

	std::string get_selected_name() {
		return Engine::is_valid(state.selected) ? Engine::get_name(state.selected) : "";
	}

	unsigned get_canvas_node_count() {
		unsigned count = 0;
		for (const auto& child : Engine::get_children(state.canvas)) {
			if (child.id() != state.frame.id())
				count++;
		}
		return count;
	}

	void set_ui_visible(bool visible) {
		if (Engine::is_valid(state.page))
			Engine::set_visible(state.page, visible);
	}

	void set_copy_imports_to_source(bool enabled) {
		state.copy_imports_to_source = enabled;
	}

	void destroy() {
		if (Engine::is_valid(state.ghost))
			Engine::clean_up(state.ghost);
		if (Engine::is_valid(state.page))
			Engine::clean_up(state.page);
		state = State{};
	}

} // namespace Editor
