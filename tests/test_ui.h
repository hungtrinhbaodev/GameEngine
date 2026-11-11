#pragma once
#include <engine/engine.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * @Note: UI test harness. Everything here goes through the public
 * Engine:: API, only the checks (test_checks.cpp) read components
 * directly to verify the tree and count pools for leaks.
 */
namespace Test {

	const std::string FONT_PATH = "res/fonts/anonymous_pro_bold.ttf";

	/**
	 * @Note: max distance between touch began and ended to count as a click.
	 */
	const float CLICK_SLOP = 12.f;

	using Touch_Action = std::function<void(Handle e, Engine::TOUCH_STATE touch_state, glm::vec2 world_position)>;

	struct Scene_State {
		std::string name = "";
		Handle root = Handle::make_invalid_handle();
		/**
		 * @Note: every node the scene created and still owns (fuzz picks from here).
		 */
		std::vector<Handle> nodes{};
		/**
		 * @Note: nodes removed from the tree with remove_child but not cleaned yet.
		 */
		std::vector<Handle> detached{};
	};

	struct Slider_State {
		Handle track = Handle::make_invalid_handle();
		Handle knob = Handle::make_invalid_handle();
		Handle label = Handle::make_invalid_handle();
		std::string name = "";
		float width = 0.f;
		float value = 0.f;
		float start_value = 0.f;
		float start_x = 0.f;
		std::function<void(float value)> on_change = nullptr;
		std::function<void(float value)> on_release = nullptr;
	};

	struct Pool_Count {
		std::string name = "";
		size_t count = 0;
	};

	// Scene bookkeeping.

	Scene_State& get_scene();

	inline bool same(const Handle& a, const Handle& b) { return a.handle == b.handle; }

	Handle track(const Handle& e);

	/**
	 * @Note: collect subtree first, clean_up, then drop every collected
	 * handle from the scene lists so nothing keeps a stale handle.
	 */
	void destroy(const Handle& e);

	void clear_scene();

	std::vector<Handle> collect_subtree(const Handle& e);

	// UI kit.

	Handle make_page(const std::string& name, const glm::vec3& color);

	Handle make_layout(
		const Handle& parent, const glm::vec2& position, const glm::vec2& size, const glm::vec3& color,
		float border_radius = 0.f, const glm::vec2& anchor = {0.f, 0.f}
	);

	Handle make_label(
		const Handle& parent, const std::string& text, int font_size, const glm::vec3& color,
		const glm::vec2& position, const glm::vec2& anchor = {0.5f, 0.5f}
	);

	Handle make_image(
		const Handle& parent, const std::string& path, const glm::vec2& position,
		const glm::vec2& anchor = {0.5f, 0.5f}
	);

	/**
	 * @Note: wraps the caller in try/catch so a stale handle inside a
	 * callback is logged instead of crashing the whole test.
	 */
	void set_touch_action(const Handle& e, const Touch_Action& action);

	Handle make_button(
		const Handle& parent, const std::string& text, const glm::vec2& position, const glm::vec2& size,
		const glm::vec3& color, const std::function<void()>& on_click, const glm::vec2& anchor = {0.f, 0.f}
	);

	std::shared_ptr<Slider_State> make_slider(
		const Handle& parent, const std::string& name, const glm::vec2& position, float width, float value,
		const std::function<void(float)>& on_change, const std::function<void(float)>& on_release = nullptr
	);

	void set_slider_value(const std::shared_ptr<Slider_State>& slider, float value);

	glm::vec3 hsv_to_rgb(float h, float s, float v);

	std::string to_hex(const glm::vec3& color);

	// Scenes.

	void build_modal_scene();

	void build_picker_scene();

	void build_lab_scene();

	/**
	 * @Note: label align / padding / fit mode / custom size grid (key 4).
	 */
	void build_label_scene();

	// Checks.

	std::vector<Pool_Count> count_pools();

	std::vector<Pool_Count>& get_baseline();

	void print_pools(const std::vector<Pool_Count>& pools, const std::string& title);

	/**
	 * @Note: return number of pools that differ from baseline.
	 */
	size_t print_pool_diff(const std::vector<Pool_Count>& before, const std::vector<Pool_Count>& after, const std::string& title);

	/**
	 * @Note: walk from root and check parent/child links, depth,
	 * valid_child and cycles. Return number of issues.
	 */
	size_t verify_tree(bool log_issues);

	/**
	 * @Note: draw order must follow the tree: index i in a DFS from root (children sorted by z order)
	 * must be the node's Draw_Order and the draw_index of every visible draw it uploads.
	 * Return number of nodes that break it.
	 */
	size_t check_draw_order(bool log_issues);

	void start_fuzz(uint32_t seed, int frames, int ops_per_frame);

	void start_stress(int cycles);

	bool is_running_job();

	/**
	 * @Note: call once per frame before Engine::process_frame.
	 */
	void tick();

	// App glue (test_app.cpp), shared by tests/test_main.cpp and the root main.cpp.

	/**
	 * @Note: call once after Engine::init: renders one frame, takes the
	 * leak baseline, prints the keys and builds the modal scene.
	 */
	void start();

	void print_help();

	/**
	 * @Note: GLFW key on press. Return true when the key is a test key.
	 */
	bool handle_key(int key, int mods);

	// Scripted run (test_drive.cpp): injects clicks / drags / wheel through the engine input handlers.

	void start_drive(GLFWwindow* window);

	/**
	 * @Note: GPU cost cases (empty, scenes, clipping on / off, full-window layers) with screenshots.
	 */
	void start_perf(GLFWwindow* window);

	void start_label_view(GLFWwindow* window);
	void start_label_wrap(GLFWwindow* window);

	/**
	 * @Note: call once per frame; returns false when the script is finished.
	 */
	bool drive_tick();

} // namespace Test
