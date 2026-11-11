#pragma once
#include <string>
#include <vector>

struct GLFWwindow;

/**
 * @Note: a small scene editor on top of the Engine UI API:
 * resource panel (images under res/, import from Finder or the Import button),
 * drag a resource onto the canvas to create an image node, select / move / delete nodes.
 */
namespace Editor {

	void init(GLFWwindow* window);

	void on_key(int key, int mods);

	// files dropped on the window from Finder (glfwSetDropCallback).
	void on_files_dropped(const std::vector<std::string>& paths);

	void destroy();

	/**
	 * @Note: read-only views for the self-test (editor_selftest.cpp).
	 */
	unsigned get_resource_count();
	std::string get_resource_path(unsigned index);
	// world centre of resource cell `index` / of the canvas / of a canvas node, {-1,-1} when missing.
	bool get_cell_center(unsigned index, float out[2]);
	bool get_canvas_center(float out[2]);
	bool get_selected_center(float out[2]);
	bool get_selected_position(float out[2]);
	std::string get_selected_name();
	unsigned get_canvas_node_count();
	// imports from the self-test must not land in the source tree.
	void set_copy_imports_to_source(bool enabled);

} // namespace Editor
