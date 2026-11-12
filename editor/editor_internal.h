#pragma once
#include <array>
#include <editor_api.h>
#include <engine/engine.h>
#include <functional>
#include <scene_io.h>
#include <string>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

/**
 * @Note: state and helpers shared by the editor's files:
 *   editor_kit.cpp     small UI kit (panel, text, button) + status line
 *   editor_scene.cpp   canvas nodes: kinds, selection frame, create / delete / duplicate / reparent, undo, save
 *   editor_panels.cpp  Scene tree, FileSystem (resources), Inspector
 *   editor.cpp         build, init, keys, actions + self-test views
 */
namespace Editor {

	using Kind = Scene_Io::Kind;

	// ---------------------------------------------------------------- style
	inline const std::string FONT_PATH = "res/fonts/anonymous_pro_bold.ttf";
	inline const glm::vec3 PAGE_BG{0.11f, 0.12f, 0.14f};
	inline const glm::vec3 PANEL_BG{0.16f, 0.17f, 0.2f};
	inline const glm::vec3 BAR_BG{0.2f, 0.21f, 0.25f};
	inline const glm::vec3 CELL_BG{0.23f, 0.24f, 0.28f};
	inline const glm::vec3 ROW_SELECTED{0.24f, 0.36f, 0.6f};
	inline const glm::vec3 ROW_TARGET{0.3f, 0.55f, 0.35f};
	inline const glm::vec3 FIELD_BG{0.1f, 0.11f, 0.13f};
	inline const glm::vec3 CANVAS_BG{0.92f, 0.93f, 0.95f};
	inline const glm::vec3 CANVAS_BORDER{0.35f, 0.37f, 0.42f};
	inline const glm::vec3 CANVAS_DROP{0.3f, 0.6f, 1.f};
	inline const glm::vec3 TEXT{0.9f, 0.91f, 0.93f};
	inline const glm::vec3 MUTED{0.6f, 0.63f, 0.68f};
	inline const glm::vec3 INK{0.13f, 0.15f, 0.19f};
	inline const glm::vec3 ACCENT{0.25f, 0.5f, 0.95f};
	inline const glm::vec3 DANGER{0.8f, 0.3f, 0.3f};
	inline const glm::vec3 BUTTON{0.32f, 0.34f, 0.4f};

	inline const float TOP_BAR = 48.f;
	inline const float STATUS_BAR = 28.f;
	inline const float LEFT_WIDTH = 300.f;
	inline const float RIGHT_WIDTH = 300.f;
	inline const float SCENE_HEIGHT = 340.f;
	inline const float CELL_GAP = 8.f;
	inline const float CELL_HEIGHT = 130.f;
	inline const float ROW_HEIGHT = 24.f;
	inline const float DRAG_START_DISTANCE = 6.f;
	inline const uint32_t Z_FRAME = 1u << 24;
	inline const uint32_t Z_GHOST = 1u << 25;

	struct Resource {
		std::string path; // relative to the root, e.g. "res/imported/cat.png"
		std::string name;
		glm::vec2 size{0.f};
	};

	struct Tree_Row {
		Handle row = Handle::make_invalid_handle();
		Handle node = Handle::make_invalid_handle();
	};

	struct Field {
		std::string property;
		Handle box = Handle::make_invalid_handle();
		Handle value = Handle::make_invalid_handle();
	};

	struct State {
		GLFWwindow* window = nullptr;
		Handle page = Handle::make_invalid_handle();
		Handle canvas = Handle::make_invalid_handle();
		Handle status = Handle::make_invalid_handle();
		Handle ghost = Handle::make_invalid_handle();
		Handle frame = Handle::make_invalid_handle();
		std::array<Handle, 4> frame_lines{};
		Handle selected = Handle::make_invalid_handle();

		// canvas node kinds (stand-in for Engine::get_node_type, ui_api_need.txt 31b).
		std::unordered_map<uint32_t, Kind> kinds;
		int node_counter = 0;
		uint32_t next_z = 1000;
		uint32_t back_z = 999;

		// FileSystem
		Handle resource_scroll = Handle::make_invalid_handle();
		glm::vec2 resource_view{0.f};
		std::vector<Handle> cells;
		std::vector<Resource> resources;
		std::string selected_resource = "";
		bool copy_imports_to_source = true;

		// Scene tree
		Handle tree_scroll = Handle::make_invalid_handle();
		glm::vec2 tree_view{0.f};
		std::vector<Tree_Row> tree_rows;

		// Inspector
		Handle inspector = Handle::make_invalid_handle();
		Handle inspector_content = Handle::make_invalid_handle();
		std::vector<Field> fields;

		// drags
		bool dragging = false;
		glm::vec2 drag_start{0.f};
		glm::vec2 grab_offset{0.f};
		float scrub_start_value = 0.f;

		// undo / file
		std::vector<std::string> history;
		int history_index = -1;
		std::string scene_path = "";
	};
	extern State state;

	// ---------------------------------------------------------------- kit (editor_kit.cpp)
	Handle make_panel(const Handle& parent, glm::vec2 position, glm::vec2 size, glm::vec3 color, float radius,
					  glm::vec2 anchor = {0.f, 0.f});
	Handle make_text(const Handle& parent, const std::string& text, int font_size, glm::vec3 color, glm::vec2 position,
					 glm::vec2 anchor = {0.f, 0.5f});
	Handle make_fitted_text(const Handle& parent, const std::string& text, int font_size, glm::vec3 color,
							glm::vec2 position, glm::vec2 size, Engine::LABEL_HORIZONTAL_ALIGN align);
	Handle make_button(const Handle& parent, const std::string& text, glm::vec2 position, glm::vec2 size,
					   glm::vec3 color, std::function<void()> on_click);
	void set_status(const std::string& text);
	std::string vec_text(glm::vec2 v);
	std::string number_text(float value, int decimals);
	glm::vec2 center_of(const Handle& e);

	// ---------------------------------------------------------------- scene (editor_scene.cpp)
	Kind kind_of(const Handle& e);
	bool is_canvas_node(const Handle& e);
	bool skip_in_scene(const Handle& e);
	void register_node(const Handle& e, Kind kind);
	void for_each_canvas_node(const std::function<void(const Handle&, int depth)>& fn);
	Handle find_canvas_node(const std::string& name);
	std::string unique_name(const std::string& base);
	void select_node(const Handle& node);
	void update_frame();
	Handle topmost_layout_at(glm::vec2 world);
	Handle create_canvas_image(const std::string& path, const Handle& parent, glm::vec2 local, glm::vec2 scale);
	bool drop_resource(const std::string& path, glm::vec2 world);
	void delete_selected();
	void duplicate_selected();
	bool reparent(const Handle& node, const Handle& new_parent);
	void commit(const std::string& what);
	void reset_history();
	std::string scene_json();
	bool load_scene_json(const std::string& text, std::string& error);
	glm::vec2 fit_scale(const std::string& path);

	// ---------------------------------------------------------------- panels (editor_panels.cpp)
	void build_tree_panel(const Handle& parent, glm::vec2 position, glm::vec2 size);
	void rebuild_tree();
	void build_resource_panel(const Handle& parent, glm::vec2 position, glm::vec2 size);
	void refresh_resources();
	void import_paths(const std::vector<std::string>& paths, bool place_on_canvas);
	std::vector<std::string> pick_files_with_dialog();
	void build_inspector_panel(const Handle& parent, glm::vec2 position, glm::vec2 size);
	void rebuild_inspector();
	void refresh_inspector_values();
	float get_property_value(const Handle& node, const std::string& property);
	void set_canvas_drop_highlight(bool on);

} // namespace Editor
