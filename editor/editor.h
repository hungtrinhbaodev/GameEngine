#pragma once
#include <string>
#include <vector>

struct GLFWwindow;

/**
 * @Note: a Godot-style scene editor on top of the Engine UI API.
 *   toolbar   : Open / Save / Undo / Redo / + Layout / + Label / Import
 *   left      : Scene tree (select, drag a row onto a row = reparent) over FileSystem (images under res/)
 *   centre    : canvas (drop resources, select, drag to move)
 *   right     : Inspector (drag number fields left / right, colour swatches, toggles, actions)
 */
namespace Editor {

	void init(GLFWwindow* window);

	void on_key(int key, int mods);

	// call once per frame before Engine::process_frame.
	void tick();

	// files dropped on the window from Finder (glfwSetDropCallback).
	void on_files_dropped(const std::vector<std::string>& paths);

	void destroy();

	// ---------------------------------------------------------------- actions (toolbar / keys / self-test)
	void add_layout();
	void add_label();
	void undo();
	void redo();
	bool save_to(const std::string& path);
	bool open_from(const std::string& path);
	bool select_by_name(const std::string& name);

	// ---------------------------------------------------------------- read-only views for the self-test
	unsigned get_resource_count();
	std::string get_resource_path(unsigned index);
	// world centres, false when missing.
	bool get_cell_center(unsigned index, float out[2]);
	bool get_canvas_center(float out[2]);
	bool get_selected_center(float out[2]);
	bool get_selected_position(float out[2]);
	bool get_node_center(const std::string& name, float out[2]);
	bool get_tree_row_center(const std::string& name, float out[2]);
	bool get_tree_empty_point(float out[2]);
	// the inspector's number field for `property` ("X", "Y", "Rotation", "Scale", "Width", ...).
	bool get_field_center(const std::string& property, float out[2]);
	std::string get_selected_name();
	std::string get_parent_name(const std::string& name); // "" = canvas root
	float get_property(const std::string& name, const std::string& property);
	// every node under the canvas (any depth), selection frame excluded.
	unsigned get_canvas_node_count();
	unsigned get_tree_row_count();
	std::string get_scene_json();

	// perf mode: hide / show the whole editor UI.
	void set_ui_visible(bool visible);
	// imports from the self-test must not land in the source tree.
	void set_copy_imports_to_source(bool enabled);

} // namespace Editor
