#include <algorithm>
#include <cstdio>
#include <editor_internal.h>
#include <filesystem>
#include <utils.h>

namespace fs = std::filesystem;

namespace Editor {

	namespace {
		bool cell_dragging = false;
		glm::vec2 cell_drag_start{0.f};

		std::string lower(std::string text) {
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
			return text;
		}

		bool is_image_file(const fs::path& path) {
			std::string ext = lower(path.extension().string());
			return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
		}

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
			for (int i = 1; fs::exists(target); i++)
				target = folder / (file_name.stem().string() + "_" + std::to_string(i) + file_name.extension().string());
			return target;
		}

		/**
		 * @Note: copies an image into <build>/res/imported (what the engine loads now) and, when known,
		 * into <source>/res/imported so the import survives the next build's resource sync.
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
			if (state.copy_imports_to_source) {
				fs::path source_folder = fs::path(EDITOR_SOURCE_DIR) / "res" / "imported";
				std::error_code source_error;
				fs::create_directories(source_folder, source_error);
				if (!source_error)
					fs::copy_file(source, source_folder / target.filename(), fs::copy_options::skip_existing, source_error);
			}
#endif
			return "res/imported/" + target.filename().string();
		}

		void paint_cells() {
			for (size_t i = 0; i < state.cells.size() && i < state.resources.size(); i++) {
				if (Engine::is_valid(state.cells[i]))
					Engine::set_layout_background_color(
						state.cells[i], state.resources[i].path == state.selected_resource ? ROW_SELECTED : CELL_BG
					);
			}
		}

		void attach_cell_touch(const Handle& cell, const Resource& resource) {
			Engine::set_touch_enabled(cell, true);
			Engine::set_touch_cancel_on_fast_move(cell, false);
			Engine::set_touch_caller(cell, [resource](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2 world, int, int) {
				switch (touch_state) {
					case Engine::TOUCH_STATE::TOUCH_BEGAN: {
						cell_drag_start = world;
						cell_dragging = false;
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_MOVE: {
						glm::vec2 delta = world - cell_drag_start;
						// sideways = drag the resource out; up / down = let the list scroll.
						if (!cell_dragging && glm::length(delta) >= DRAG_START_DISTANCE &&
							std::abs(delta.x) > std::abs(delta.y)) {
							cell_dragging = true;
							Engine::claim_touch(e);
							Engine::set_image_path(state.ghost, resource.path);
							glm::vec2 texture = Engine::query_image_size(resource.path);
							float scale = texture.x > 0.f ? std::min(1.f, 120.f / std::max(texture.x, texture.y)) : 1.f;
							Engine::set_scale(state.ghost, {scale, scale});
							Engine::set_visible(state.ghost, true);
						}
						if (cell_dragging) {
							Engine::set_position(state.ghost, world);
							set_canvas_drop_highlight(Editor_Api::is_point_in_node(state.canvas, world));
						}
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_ENDED:
					case Engine::TOUCH_STATE::TOUCH_CANCEL: {
						bool was_dragging = cell_dragging;
						cell_dragging = false;
						Engine::set_visible(state.ghost, false);
						set_canvas_drop_highlight(false);
						if (touch_state == Engine::TOUCH_STATE::TOUCH_CANCEL)
							break; // the list took the touch to scroll
						if (was_dragging) {
							if (!drop_resource(resource.path, world))
								set_status("Drop on the canvas to add " + resource.name);
						} else {
							state.selected_resource = resource.path;
							paint_cells();
							glm::vec2 size = Engine::query_image_size(resource.path);
							set_status(resource.path + "  (" + vec_text(size) + " px) - drag it sideways onto the canvas");
						}
						break;
					}
					default: {
						break;
					}
				}
			});
		}
	} // namespace

	void set_canvas_drop_highlight(bool on) {
		Engine::set_layout_border_color(state.canvas, on ? CANVAS_DROP : CANVAS_BORDER);
		Engine::set_layout_border_width(state.canvas, on ? 4.f : 2.f);
	}

	void build_resource_panel(const Handle& parent, glm::vec2 position, glm::vec2 size) {
		Handle panel = make_panel(parent, position, size, PANEL_BG, 0.f);
		make_text(panel, "FileSystem", 16, TEXT, {14.f, size.y - 18.f});
		make_text(panel, "drag sideways onto the canvas", 11, MUTED, {120.f, size.y - 18.f});
		state.resource_view = {size.x - 16.f, size.y - 44.f};
		state.resource_scroll = Engine::create_scroll_layout();
		Engine::set_content_size(state.resource_scroll, state.resource_view);
		Engine::set_position(state.resource_scroll, {8.f, 8.f});
		Engine::set_scroll_direction(state.resource_scroll, Engine::SCROLL_DIRECTION::SCROLL_VERTICAL);
		Engine::set_scroll_speed(state.resource_scroll, 30.f);
		Engine::add_child(panel, state.resource_scroll);
		// a horizontal drag on a cell claims the touch; a vertical drag scrolls the list.
		Engine::set_layout_scroll_touch_enabled(state.resource_scroll, true);
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
		Engine::set_scroll_layout_container_position(state.resource_scroll, {0.f, view.y - content_height});

		for (size_t i = 0; i < state.resources.size(); i++) {
			Resource& resource = state.resources[i];
			int row = (int)i / 2, column = (int)i % 2;
			glm::vec2 position{CELL_GAP + column * (cell_width + CELL_GAP), content_height - (row + 1) * (CELL_HEIGHT + CELL_GAP)};
			Handle cell = Engine::create_layout();
			Engine::set_content_size(cell, {cell_width, CELL_HEIGHT});
			Engine::set_position(cell, position);
			Engine::set_layout_background_enabled(cell, true);
			Engine::set_layout_border_radius(cell, 8.f);
			Engine::set_layout_clipping_enabled(cell, true);
			Engine::add_child_to_container(state.resource_scroll, cell);
			state.cells.push_back(cell);

			resource.size = Engine::query_image_size(resource.path);
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
			make_fitted_text(cell, resource.name, 12, TEXT, {6.f, 4.f}, {cell_width - 12.f, 20.f},
							 Engine::LABEL_HORIZONTAL_ALIGN_CENTER);
			attach_cell_touch(cell, resource);
		}
		paint_cells();
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
		glm::vec2 cursor = Engine::get_cursor_world_position();
		if (place_on_canvas && Editor_Api::is_point_in_node(state.canvas, cursor)) {
			for (size_t i = 0; i < imported.size(); i++)
				drop_resource(imported[i], cursor + glm::vec2(24.f, -24.f) * (float)i);
		}
		set_status("Imported " + std::to_string(imported.size()) + " image(s) into res/imported");
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

} // namespace Editor
