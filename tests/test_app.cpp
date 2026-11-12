#include <chrono>
#include <engine/components.h>
#include <typeinfo>
#include <vulkan/draws/vk_draw_font_2D.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cmath>
#include <engine/engine.h>
#include <engine/touch_handler.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <log.h>
#include <math_custom.h>
#include <string>
#include <test_ui.h>

namespace Engine {
	uint32_t spawn_touch(const glm::vec2& touch_world_position, int button, int mods);
	void release_touch(uint32_t touch_id, const glm::vec2& touch_world_position);
	extern uint32_t window_unique_touch_id;
	extern glm::vec2 cursor_world_position;
} // namespace Engine

namespace Test {

	namespace {
		uint32_t last_fuzz_seed = 1;

		// ------------------------------ #25 touch re-entrancy ------------------------------

		const glm::vec2 reentry_point{600.f, 360.f};
		int reentry_failed = 0;

		/**
		 * @Note: full-window touchable layout that does not swallow, so every node under the point gets the touch.
		 */
		Handle make_touch_layer() {
			Handle e = Engine::create_layout();
			Engine::set_content_size(e, {1200.f, 720.f});
			Engine::set_anchor(e, {0.5f, 0.5f});
			Engine::set_position(e, reentry_point);
			Engine::set_touch_enabled(e, true);
			Engine::set_swallow_touches(e, false);
			return e;
		}

		void reentry_expect(int case_id, bool ok, const std::string& what) {
			if (!ok)
				reentry_failed++;
			Log::info(ok ? "[repro] PASS" : "[repro] FAIL", "reentry case", case_id, what);
		}

		/**
		 * @Note: press + release at the centre, one frame each, exceptions count as a failure of that case.
		 */
		bool reentry_click(int case_id) {
			try {
				uint32_t id = Engine::spawn_touch(reentry_point, GLFW_MOUSE_BUTTON_LEFT, 0);
				Engine::process_frame(0.f);
				Engine::release_touch(id, reentry_point);
				Engine::process_frame(0.f);
				Engine::process_frame(0.f);
				return true;
			} catch (const std::exception& exception) {
				reentry_expect(
					case_id, false, std::string("threw ") + typeid(exception).name() + ": " + exception.what()
				);
				return false;
			}
		}

		void clean_if_valid(const std::vector<Handle>& nodes) {
			for (const auto& e : nodes) {
				if (Engine::is_valid(e))
					Engine::clean_up(e);
			}
			Engine::process_frame(0.f);
		}

		// ------------------------------ ui_api_need.txt items marked Handled ------------------------------

		int api_failed = 0;

		void api_expect(const std::string& item, bool ok, const std::string& what) {
			if (!ok)
				api_failed++;
			Log::info(ok ? "[repro] PASS" : "[repro] FAIL", "api", item, what);
		}

		std::string vec_text(const glm::vec2& v) {
			return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
		}

		bool nearly_equal(const glm::vec2& a, const glm::vec2& b, float eps = 0.01f) {
			return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps;
		}

		void repro_api_checks() {
			Log::info("[repro] API checks for ui_api_need.txt items 1, 3, 4, 7 (item 2: repro-reentry)");
			api_failed = 0;
			Handle root = Engine::get_root();

			// ---- label defaults (#34 g): a new custom-size label is CENTER / CENTER / CLIP and draws centered.
			try {
				Handle label = Engine::create_label();
				Engine::set_label_string(label, "Hi");
				Engine::add_child(root, label);
				// fit mode must be set manually now; set_content_size no longer implies custom sizing.
				Engine::set_label_render_fit_mode(label, Engine::LABEL_RENDER_FIT_CLIP);
				Engine::set_content_size(label, {300.f, 120.f});
				Engine::process_frame(0.f);
				auto h = Engine::get_label_horizontal_align(label);
				auto v = Engine::get_label_vertical_align(label);
				auto fit = Engine::get_label_render_fit_mode(label);
				api_expect(
					"label-default", h == Engine::LABEL_HORIZONTAL_ALIGN_CENTER,
					"horizontal align " + std::to_string((int)h) + " (want CENTER 0)"
				);
				api_expect(
					"label-default", v == Engine::LABEL_VERTICAL_ALIGN_CENTER,
					"vertical align " + std::to_string((int)v) + " (want CENTER 1)"
				);
				api_expect(
					"label-default", fit == Engine::LABEL_RENDER_FIT_CLIP,
					"fit mode " + std::to_string((int)fit) + " (want CLIP 0)"
				);
				const auto& [info] = Engine::get_registry().read<Engine::Label_Renderer_Information>(label.id());
				glm::vec2 inner = info.inner_label_transform.position;
				glm::vec2 text = Vulkan::Draw_Font_2D::get_renderer_text_size(
					"Hi", info.font_path, info.font_size, info.stroke_size
				);
				glm::vec2 want = (glm::vec2{300.f, 120.f} - text) * 0.5f;
				api_expect(
					"label-default", nearly_equal(inner, want, 1.f),
					"inner position " + vec_text(inner) + " want " + vec_text(want)
				);
				Engine::clean_up(label);
				Engine::process_frame(0.f);
			} catch (const std::exception& ex) {
				api_expect("label-default", false, std::string("exception ") + ex.what());
			}

			// ---- 7. find by name / tag
			try {
				Handle panel = Engine::create_layout();
				Engine::set_name(panel, "api_panel");
				Engine::set_tag(panel, 701);
				Engine::add_child(root, panel);
				Handle row = Engine::create_layout();
				Engine::set_name(row, "api_row");
				Engine::set_tag(row, 702);
				Engine::add_child(panel, row);
				Handle target = Engine::create_layout();
				Engine::set_name(target, "api_target");
				Engine::set_tag(target, 703);
				Engine::add_child(row, target);
				// same name as `row` but one level higher: a breadth-first search must return this one.
				Handle shallow_row = Engine::create_layout();
				Engine::set_name(shallow_row, "api_row");
				Engine::add_child(root, shallow_row);
				Engine::process_frame(0.f);
				api_expect(
					"7", Engine::get_name(target) == "api_target" && Engine::get_tag(target) == 703,
					"get_name / get_tag return what was set"
				);
				api_expect(
					"7", same(Engine::find_node_by_name("api_target"), target),
					"find_node_by_name finds a grandchild from root (got id " +
						std::to_string(Engine::find_node_by_name("api_target").id()) + ")"
				);
				api_expect(
					"7", same(Engine::find_node_by_name("api_target", panel), target),
					"find_node_by_name finds it under a given parent"
				);
				api_expect(
					"7", Engine::find_node_by_name("api_panel", row).is_invalid(),
					"find_node_by_name does not find a node outside the parent's subtree"
				);
				api_expect("7", Engine::find_node_by_name("api_missing").is_invalid(), "missing name returns invalid");
				api_expect(
					"7", same(Engine::find_node_by_name("api_row"), shallow_row),
					"duplicate names: the shallower node wins (got id " +
						std::to_string(Engine::find_node_by_name("api_row").id()) + ", shallow " +
						std::to_string(shallow_row.id()) + ", deep " + std::to_string(row.id()) + ")"
				);
				api_expect(
					"7", same(Engine::find_node_by_tag(703), target),
					"find_node_by_tag finds a grandchild (got id " +
						std::to_string(Engine::find_node_by_tag(703).id()) + ")"
				);
				Engine::clean_up(panel);
				Engine::clean_up(shallow_row);
				Engine::process_frame(0.f);
				api_expect(
					"7", Engine::find_node_by_name("api_target").is_invalid(), "a cleaned up node is not found any more"
				);
			} catch (const std::exception& exception) {
				api_expect("7", false, std::string("threw: ") + exception.what());
			}

			// ---- 3. scroll offset get / set (range: [view - content, 0] per axis)
			try {
				Handle scroll = Engine::create_scroll_layout();
				Engine::set_content_size(scroll, {200.f, 200.f});
				Engine::set_position(scroll, {100.f, 100.f});
				Engine::set_inner_container_size(scroll, {200.f, 1000.f});
				Engine::add_child(root, scroll);
				Engine::process_frame(0.f);
				glm::vec2 scroll_position = Engine::get_position(scroll);
				glm::vec2 offset = Engine::get_scroll_layout_container_position(scroll);
				api_expect(
					"3", nearly_equal(Engine::get_position(scroll), scroll_position),
					"get_scroll_layout_container_position does not move the scroll layout itself (" +
						vec_text(scroll_position) + " -> " + vec_text(Engine::get_position(scroll)) + ")"
				);
				Engine::process_frame(0.f);
				api_expect(
					"3", nearly_equal(Engine::get_position(scroll), scroll_position),
					"scroll layout still at its position one frame after the getter (" +
						vec_text(Engine::get_position(scroll)) + ")"
				);
				Engine::set_scroll_layout_container_position(scroll, {0.f, -300.f});
				Engine::process_frame(0.f);
				glm::vec2 got = Engine::get_scroll_layout_container_position(scroll);
				api_expect(
					"3", nearly_equal(got, {0.f, -300.f}),
					"set {0, -300} then get returns it (start " + vec_text(offset) + ", got " + vec_text(got) + ")"
				);
				Engine::set_scroll_layout_container_position(scroll, {0.f, -5000.f});
				Engine::process_frame(0.f);
				got = Engine::get_scroll_layout_container_position(scroll);
				api_expect(
					"3", nearly_equal(got, {0.f, -800.f}),
					"set past the end is clamped to {0, -800} (got " + vec_text(got) + ")"
				);
				Engine::set_scroll_layout_container_position(scroll, {0.f, 50.f});
				Engine::process_frame(0.f);
				got = Engine::get_scroll_layout_container_position(scroll);
				api_expect(
					"3", nearly_equal(got, {0.f, 0.f}),
					"set before the start is clamped to {0, 0} (got " + vec_text(got) + ")"
				);
				Engine::clean_up(scroll);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("3", false, std::string("threw: ") + exception.what());
			}

			// ---- 4. world-space queries
			try {
				Handle parent = Engine::create_layout();
				Engine::set_content_size(parent, {100.f, 100.f});
				Engine::set_position(parent, {300.f, 200.f});
				Engine::set_rotation(parent, 0.5f);
				Engine::set_scale(parent, {2.f, 2.f});
				Engine::add_child(root, parent);
				Handle child = Engine::create_layout();
				Engine::set_position(child, {10.f, 20.f});
				Engine::add_child(parent, child);
				Engine::process_frame(0.f);
				glm::vec2 parent_world = Engine::get_world_position(parent);
				glm::vec2 expected = parent_world + Math::rotate(0.5f, glm::vec2(10.f, 20.f) * 2.f);
				glm::vec2 child_world = Engine::get_world_position(child);
				api_expect(
					"4", nearly_equal(child_world, expected, 0.05f),
					"child world position = parent + rotate(scale * local) (" + vec_text(child_world) + " vs " +
						vec_text(expected) + ")"
				);
				glm::vec2 back = Engine::to_my_position_space(parent, child_world);
				api_expect(
					"4", nearly_equal(back, {10.f, 20.f}, 0.05f),
					"to_my_position_space(parent, child world) = child local (" + vec_text(back) + ")"
				);
				Engine::set_scale(parent, {0.f, 0.f});
				Engine::process_frame(0.f);
				glm::vec2 fallback = Engine::to_my_position_space(parent, {1.f, 1.f}, {-7.f, -7.f});
				api_expect(
					"4", nearly_equal(fallback, {-7.f, -7.f}),
					"scale 0 returns the fallback, no inf / NaN (" + vec_text(fallback) + ")"
				);
				// a mirrored node (negative scale) is still invertible.
				Engine::set_scale(parent, {-1.f, 1.f});
				Engine::set_rotation(parent, 0.f);
				Engine::process_frame(0.f);
				glm::vec2 mirrored = Engine::to_my_position_space(
					parent, Engine::get_world_position(parent) + glm::vec2(-10.f, 20.f), {-7.f, -7.f}
				);
				api_expect(
					"4", nearly_equal(mirrored, {10.f, 20.f}, 0.05f),
					"negative scale (mirror) converts instead of returning the fallback (" + vec_text(mirrored) + ")"
				);
				// queried in the same frame as the change (an app moves a node, then places a tooltip):
				// the getters resolve dirty ancestors on demand.
				Engine::set_scale(parent, {1.f, 1.f});
				Engine::process_frame(0.f);
				Handle grandchild = Engine::create_layout();
				Engine::set_position(grandchild, {1.f, 2.f});
				Engine::add_child(child, grandchild);
				Engine::process_frame(0.f);
				Engine::set_position(parent, {500.f, 300.f});
				glm::vec2 same_frame = Engine::get_world_position(child);
				api_expect(
					"4", nearly_equal(same_frame, {510.f, 320.f}, 0.05f),
					"child world position is up to date right after the parent moved, before process_frame (" +
						vec_text(same_frame) + ", expected (510, 320))"
				);
				Engine::set_position(parent, {600.f, 100.f});
				glm::vec2 deep = Engine::get_world_position(grandchild);
				api_expect(
					"4", nearly_equal(deep, {611.f, 122.f}, 0.05f),
					"grandchild world position is up to date right after the grandparent moved (" + vec_text(deep) +
						", expected (611, 122))"
				);
				glm::vec2 local = Engine::to_my_position_space(child, {620.f, 130.f});
				api_expect(
					"4", nearly_equal(local, {10.f, 10.f}, 0.05f),
					"to_my_position_space uses the new transform in the same frame (" + vec_text(local) +
						", expected (10, 10))"
				);
				Engine::process_frame(0.f);
				glm::vec2 after_frame = Engine::get_world_position(child);
				api_expect(
					"4", nearly_equal(after_frame, {610.f, 120.f}, 0.05f),
					"after the frame the child is still right (" + vec_text(after_frame) + ")"
				);
				// percent size read right after the parent's size changed.
				Handle percent = Engine::create_layout();
				Engine::set_size_percent(percent, {0.5f, 0.25f});
				Engine::add_child(parent, percent);
				Engine::process_frame(0.f);
				Engine::set_content_size(parent, {400.f, 200.f});
				glm::vec2 percent_size = Engine::get_content_size(percent);
				api_expect(
					"4", nearly_equal(percent_size, {200.f, 50.f}),
					"percent child size is up to date right after the parent resized (" + vec_text(percent_size) +
						", expected (200, 50))"
				);
				Engine::clean_up(parent);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("4", false, std::string("threw: ") + exception.what());
			}

			// ---- 1. click caller
			try {
				auto clicks = std::make_shared<int>(0);
				Handle button = Engine::create_layout();
				Engine::set_content_size(button, {100.f, 60.f});
				Engine::set_anchor(button, {0.5f, 0.5f});
				Engine::set_position(button, reentry_point);
				Engine::set_touch_enabled(button, true);
				Engine::set_click_caller(button, [clicks](Handle, glm::vec2, int, int) { (*clicks)++; });
				Engine::add_child(root, button);
				Engine::process_frame(0.f);
				uint32_t id = Engine::spawn_touch(reentry_point, GLFW_MOUSE_BUTTON_LEFT, 0);
				Engine::process_frame(0.f);
				Engine::release_touch(id, reentry_point);
				Engine::process_frame(0.f);
				Engine::process_frame(0.f);
				api_expect(
					"1", *clicks == 1, "press + release inside fires one click (" + std::to_string(*clicks) + ")"
				);
				*clicks = 0;
				id = Engine::spawn_touch(reentry_point, GLFW_MOUSE_BUTTON_LEFT, 0);
				Engine::process_frame(0.f);
				Engine::release_touch(id, {5.f, 5.f});
				Engine::process_frame(0.f);
				Engine::process_frame(0.f);
				api_expect(
					"1", *clicks == 0,
					"press inside, release far outside fires no click (" + std::to_string(*clicks) + " clicks)"
				);
				*clicks = 0;
				id = Engine::spawn_touch({5.f, 5.f}, GLFW_MOUSE_BUTTON_LEFT, 0);
				Engine::process_frame(0.f);
				Engine::release_touch(id, reentry_point);
				Engine::process_frame(0.f);
				Engine::process_frame(0.f);
				api_expect(
					"1", *clicks == 0, "press outside, release inside fires no click (" + std::to_string(*clicks) + ")"
				);
				Engine::clean_up(button);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("1", false, std::string("threw: ") + exception.what());
			}

			// ---- 18. claim_touch: the claiming listener keeps the touch, every other listener still in
			// the list gets exactly one TOUCH_CANCEL and nothing after it (drag out of a scroll layout).
			try {
				Handle cell = Engine::create_layout();
				Engine::set_content_size(cell, {100.f, 100.f});
				Engine::set_position(cell, {60.f, 60.f});
				Engine::set_touch_enabled(cell, true);
				Engine::set_swallow_touches(cell, false);
				Engine::add_child(root, cell);
				Handle scroll_stub = Engine::create_layout();
				Engine::set_content_size(scroll_stub, {100.f, 100.f});
				Engine::set_position(scroll_stub, {60.f, 60.f});
				Engine::set_touch_enabled(scroll_stub, true);
				Engine::set_swallow_touches(scroll_stub, false);
				Engine::add_child(root, scroll_stub);
				Engine::process_frame(0.f);

				auto scroll_states = std::make_shared<std::vector<Engine::TOUCH_STATE>>();
				auto cell_states = std::make_shared<std::vector<Engine::TOUCH_STATE>>();
				auto claimed = std::make_shared<bool>(false);
				Engine::set_touch_caller(
					scroll_stub,
					[scroll_states](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
						scroll_states->push_back(state);
					}
				);
				Engine::set_touch_caller(
					cell, [cell_states, claimed, cell](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
						cell_states->push_back(state);
						if (state == Engine::TOUCH_STATE::TOUCH_MOVE && !*claimed) {
							*claimed = true;
							Engine::claim_touch(cell);
						}
					}
				);

				uint32_t id = Engine::spawn_touch({110.f, 110.f}, GLFW_MOUSE_BUTTON_LEFT, 0);
				Engine::process_frame(0.f); // TOUCH_BEGAN to both
				Engine::process_frame(0.f); // TOUCH_MOVE to both; cell claims here
				Engine::process_frame(0.f); // scroll_stub already discarded; cell keeps moving
				Engine::release_touch(id, {110.f, 110.f});
				Engine::process_frame(0.f); // TOUCH_ENDED, only to cell
				Engine::process_frame(0.f);

				int cancels = 0;
				for (const auto& state : *scroll_states) {
					if (state == Engine::TOUCH_STATE::TOUCH_CANCEL)
						cancels++;
				}
				api_expect(
					"18", cancels == 1,
					"the non-claiming listener gets exactly one TOUCH_CANCEL (" + std::to_string(cancels) + ")"
				);
				api_expect(
					"18", !scroll_states->empty() && scroll_states->back() == Engine::TOUCH_STATE::TOUCH_CANCEL,
					"TOUCH_CANCEL is the last thing the non-claiming listener ever sees"
				);
				api_expect(
					"18", !cell_states->empty() && cell_states->back() == Engine::TOUCH_STATE::TOUCH_ENDED,
					"the claiming listener keeps the touch through to TOUCH_ENDED"
				);
				Engine::clean_up(cell);
				Engine::clean_up(scroll_stub);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("18", false, std::string("threw: ") + exception.what());
			}

			// ---- 25. to_my_position_space with a non-default anchor (item 4's checks above only used
			// anchor {0, 0}, so they missed the anchor-offset bug this fix addresses).
			try {
				Handle parent = Engine::create_layout();
				Engine::set_content_size(parent, {100.f, 100.f});
				Engine::set_anchor(parent, {0.5f, 0.5f});
				Engine::set_position(parent, {300.f, 200.f});
				Engine::add_child(root, parent);
				Handle child = Engine::create_layout();
				Engine::set_position(child, {10.f, 20.f});
				Engine::add_child(parent, child);
				Engine::process_frame(0.f);
				glm::vec2 child_world = Engine::get_world_position(child);
				glm::vec2 back = Engine::to_my_position_space(parent, child_world);
				api_expect(
					"25", nearly_equal(back, {10.f, 20.f}, 0.05f),
					"to_my_position_space with anchor (0.5, 0.5) still returns the child's position measured "
					"from the parent's bottom-left (" + vec_text(back) + ")"
				);
				Engine::clean_up(parent);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("25", false, std::string("threw: ") + exception.what());
			}

			// ---- 17. query_image_size: reads a texture's pixel size directly, without creating a node.
			try {
				std::string missing_path = "res/this_file_does_not_exist_____.png";
				glm::vec2 missing_size = Engine::query_image_size(missing_path);
				api_expect(
					"17", missing_size == glm::vec2(0.f, 0.f),
					"a missing file returns {0, 0} (" + vec_text(missing_size) + ")"
				);

				std::string image_path = "res/AddonIcon5.png";
				glm::vec2 queried = Engine::query_image_size(image_path);
				api_expect(
					"17", queried.x > 0.f && queried.y > 0.f, "an existing file returns a real size (" + vec_text(queried) + ")"
				);

				Handle image = Engine::create_image();
				Engine::set_image_path(image, image_path);
				Engine::add_child(root, image);
				Engine::process_frame(0.f);
				glm::vec2 rendered = Engine::get_content_size(image);
				api_expect(
					"17", nearly_equal(queried, rendered),
					"matches the size the renderer measures for the same file (" + vec_text(queried) + " vs " +
						vec_text(rendered) + ")"
				);
				Engine::clean_up(image);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("17", false, std::string("threw: ") + exception.what());
			}

			// ---- 27. set_clear_screen_color: smoke test only, there is no readback API to check the
			// value against (nothing in the pipeline exposes the current clear colour).
			try {
				Engine::set_clear_screen_color({0.1f, 0.2f, 0.3f}, 1.f);
				Engine::process_frame(0.f);
				Engine::set_clear_screen_color({0.5f, 0.5f, 0.5f}, 1.f); // restore the default
				Engine::process_frame(0.f);
				api_expect("27", true, "set_clear_screen_color runs without throwing");
			} catch (const std::exception& exception) {
				api_expect("27", false, std::string("threw: ") + exception.what());
			}

			// ---- 12. set_image_fit_mode / get_image_fit_mode: STRETCH scales each axis independently,
			// FIT clamps to the smaller ratio, FILL clamps to the larger ratio.
			try {
				std::string path = "res/AddonIcon5.png";
				glm::vec2 native = Engine::query_image_size(path);
				Handle image = Engine::create_image();
				Engine::set_image_path(image, path);
				Engine::add_child(root, image);
				Engine::process_frame(0.f);
				api_expect(
					"12", Engine::get_image_fit_mode(image) == Engine::IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_AUTO,
					"a new image defaults to fit mode AUTO"
				);

				glm::vec2 target{native.x * 2.f, native.y}; // twice as wide as the image, same height
				auto read_inner_scale = [&]() {
					const auto& [info] = Engine::get_registry().read<Engine::Image_Renderer_Information>(image.id());
					return info.inner_image_transform.scale;
				};

				Engine::set_image_fit_mode(image, Engine::IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_FIT);
				api_expect(
					"12", Engine::get_image_fit_mode(image) == Engine::IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_FIT,
					"fit mode round-trips (FIT)"
				);
				Engine::set_content_size(image, target);
				Engine::process_frame(0.f);
				glm::vec2 fit_scale = read_inner_scale();
				api_expect(
					"12", nearly_equal(fit_scale, glm::vec2(1.f, 1.f), 0.05f),
					"FIT clamps to the smaller axis ratio (" + vec_text(fit_scale) + ", want (1, 1))"
				);

				Engine::set_image_fit_mode(image, Engine::IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_FILL);
				Engine::process_frame(0.f);
				glm::vec2 fill_scale = read_inner_scale();
				api_expect(
					"12", nearly_equal(fill_scale, glm::vec2(2.f, 2.f), 0.05f),
					"FILL clamps to the larger axis ratio (" + vec_text(fill_scale) + ", want (2, 2))"
				);

				Engine::set_image_fit_mode(image, Engine::IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_STRETCH);
				Engine::process_frame(0.f);
				glm::vec2 stretch_scale = read_inner_scale();
				api_expect(
					"12", nearly_equal(stretch_scale, glm::vec2(2.f, 1.f), 0.05f),
					"STRETCH scales each axis independently (" + vec_text(stretch_scale) + ", want (2, 1))"
				);

				Engine::clean_up(image);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("12", false, std::string("threw: ") + exception.what());
			}

			// ---- 19. find_topmost_node_at: works with no touch listener needed, honours z order.
			try {
				Handle a = Engine::create_layout();
				Engine::set_content_size(a, {100.f, 100.f});
				Engine::set_position(a, {800.f, 40.f});
				Engine::set_z_order(a, 0);
				Engine::add_child(root, a);
				Handle b = Engine::create_layout();
				Engine::set_content_size(b, {100.f, 100.f});
				Engine::set_position(b, {800.f, 40.f});
				Engine::set_z_order(b, 1);
				Engine::add_child(root, b);
				// neither node has touch enabled and neither has a caller set.
				Engine::process_frame(0.f);
				Handle top = Engine::find_topmost_node_at({850.f, 90.f});
				api_expect(
					"19", top.id() == b.id(), "the higher z-order node wins with no touch listener on either node"
				);
				Engine::set_z_order(a, 2);
				Engine::process_frame(0.f);
				top = Engine::find_topmost_node_at({850.f, 90.f});
				api_expect("19", top.id() == a.id(), "re-ordering z_order flips which node is topmost");
				Handle miss = Engine::find_topmost_node_at({5000.f, 5000.f});
				api_expect("19", miss.is_invalid(), "a point outside every node returns an invalid handle");
				Engine::clean_up(a);
				Engine::clean_up(b);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("19", false, std::string("threw: ") + exception.what());
			}

			// ---- 20. get_cursor_world_position: returns whatever the real cursor callback last stored.
			try {
				Engine::cursor_world_position = {321.f, 654.f};
				api_expect(
					"20", nearly_equal(Engine::get_cursor_world_position(), {321.f, 654.f}),
					"returns the stored cursor world position (" + vec_text(Engine::get_cursor_world_position()) + ")"
				);
			} catch (const std::exception& exception) {
				api_expect("20", false, std::string("threw: ") + exception.what());
			}

			// ---- 21. set_touch_cancel_on_fast_move(e, false): a fast drag across the window must not
			// get cancelled for a node that opted out.
			try {
				Handle e = Engine::create_layout();
				Engine::set_content_size(e, {1200.f, 720.f});
				Engine::set_touch_enabled(e, true);
				Engine::set_swallow_touches(e, false);
				Engine::add_child(root, e);
				Engine::set_touch_cancel_on_fast_move(e, false);
				Engine::process_frame(0.f);

				auto states = std::make_shared<std::vector<Engine::TOUCH_STATE>>();
				Engine::set_touch_caller(e, [states](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
					states->push_back(state);
				});

				uint32_t id = Engine::spawn_touch({10.f, 10.f}, GLFW_MOUSE_BUTTON_LEFT, 0);
				Engine::window_unique_touch_id = id;
				Engine::process_frame(0.f); // TOUCH_BEGAN
				glm::vec2 screen = Engine::get_window_size();
				glm::vec2 far_world{1100.f, 700.f}; // far corner: well past the fast-move cancel threshold
				Engine::on_cursor_touch_position(nullptr, far_world.x, screen.y - far_world.y);
				Engine::process_frame(0.f);
				Engine::process_frame(0.f);

				bool ever_cancelled = false;
				for (const auto& state : *states) {
					if (state == Engine::TOUCH_STATE::TOUCH_CANCEL)
						ever_cancelled = true;
				}
				api_expect(
					"21", !ever_cancelled,
					"a fast drag is not cancelled once this node opts out of the fast-move cancel rule"
				);
				if (Engine::is_valid(e))
					Engine::release_touch(id, far_world);
				Engine::process_frame(0.f);
				Engine::process_frame(0.f);
				Engine::clean_up(e);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("21", false, std::string("threw: ") + exception.what());
			}

			// ---- 22. the touch caller receives the real mouse button and modifier flags from spawn_touch.
			try {
				Handle e = Engine::create_layout();
				Engine::set_content_size(e, {50.f, 50.f});
				Engine::set_position(e, {700.f, 700.f});
				Engine::set_touch_enabled(e, true);
				Engine::add_child(root, e);
				Engine::process_frame(0.f);

				auto seen_button = std::make_shared<int>(-1);
				auto seen_mods = std::make_shared<int>(-1);
				Engine::set_touch_caller(
					e, [seen_button, seen_mods](Handle, Engine::TOUCH_STATE state, glm::vec2, int button, int mods) {
						if (state == Engine::TOUCH_STATE::TOUCH_BEGAN) {
							*seen_button = button;
							*seen_mods = mods;
						}
					}
				);
				uint32_t id = Engine::spawn_touch({710.f, 710.f}, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOD_SHIFT);
				Engine::process_frame(0.f);
				api_expect(
					"22", *seen_button == GLFW_MOUSE_BUTTON_RIGHT,
					"the touch caller receives the real button (" + std::to_string(*seen_button) + ")"
				);
				api_expect(
					"22", *seen_mods == GLFW_MOD_SHIFT,
					"the touch caller receives the modifier flags (" + std::to_string(*seen_mods) + ")"
				);
				Engine::release_touch(id, {710.f, 710.f});
				Engine::process_frame(0.f);
				Engine::process_frame(0.f);
				Engine::clean_up(e);
				Engine::process_frame(0.f);
			} catch (const std::exception& exception) {
				api_expect("22", false, std::string("threw: ") + exception.what());
			}

			Engine::process_frame(0.f);
			verify_tree(true);
			print_pool_diff(get_baseline(), count_pools(), "after api checks");
			if (api_failed == 0) {
				Log::info("[repro] survived api checks, all pass");
			} else {
				Log::info("[repro] api checks:", api_failed, "failed");
			}
		}

		void repro_touch_reentrancy() {
			Log::info("[repro] #25 touch dispatch re-entrancy: engine calls made inside touch callbacks");
			reentry_failed = 0;
			Handle root = Engine::get_root();

			// 1. a callback resolves a percent size (get_content_size) while the dispatch is running.
			{
				auto b_calls = std::make_shared<std::vector<int>>();
				auto a_calls = std::make_shared<std::vector<int>>();
				Handle percent_holder = Engine::create_layout();
				Engine::set_content_size(percent_holder, {200.f, 200.f});
				Engine::add_child(root, percent_holder);
				Handle percent = Engine::create_layout();
				Engine::set_size_percent(percent, {0.5f, 0.5f});
				Engine::add_child(percent_holder, percent);
				Handle b = make_touch_layer();
				Engine::add_child(root, b);
				Handle a = make_touch_layer();
				Engine::add_child(root, a);
				Engine::set_touch_caller(b, [b_calls](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
					b_calls->push_back(state);
				});
				Engine::set_touch_caller(
					a, [a_calls, percent, percent_holder](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
						a_calls->push_back(state);
						// make the size dirty, then query it: the resolver refills the engine scratch list.
						Engine::set_content_size(percent_holder, {200.f + a_calls->size(), 200.f});
						Engine::get_content_size(percent);
					}
				);
				Engine::process_frame(0.f);
				if (reentry_click(1)) {
					std::vector<int> expected{Engine::TOUCH_STATE::TOUCH_BEGAN, Engine::TOUCH_STATE::TOUCH_ENDED};
					reentry_expect(
						1, *a_calls == expected,
						"top node got BEGAN + ENDED (" + std::to_string(a_calls->size()) + " calls)"
					);
					reentry_expect(
						1, *b_calls == expected,
						"node below still got BEGAN + ENDED after the size query (" + std::to_string(b_calls->size()) +
							" calls)"
					);
				}
				clean_if_valid({a, b, percent_holder});
			}

			// 2. a node destroys itself in its touch caller; it also has a click caller. Another touchable
			//    node created later (last in the component pool, not under the finger) must not get a click.
			{
				auto other_clicks = std::make_shared<int>(0);
				Handle self = make_touch_layer();
				Engine::add_child(root, self);
				Handle other = Engine::create_layout();
				Engine::set_content_size(other, {10.f, 10.f});
				Engine::set_position(other, {5.f, 5.f});
				Engine::set_touch_enabled(other, true);
				Engine::add_child(root, other);
				Engine::set_click_caller(other, [other_clicks](Handle, glm::vec2, int, int) { (*other_clicks)++; });
				Engine::set_click_caller(self, [](Handle, glm::vec2, int, int) {});
				Engine::set_touch_caller(self, [](Handle e, Engine::TOUCH_STATE state, glm::vec2, int, int) {
					if (state == Engine::TOUCH_STATE::TOUCH_ENDED)
						Engine::clean_up(e); // nothing captured is used after this line
				});
				Engine::process_frame(0.f);
				if (reentry_click(2)) {
					reentry_expect(2, !Engine::is_valid(self), "self destroyed inside its own caller");
					reentry_expect(
						2, *other_clicks == 0,
						"untouched node got no click (" + std::to_string(*other_clicks) + " clicks)"
					);
				}
				clean_if_valid({self, other});
			}

			// 3. the top node destroys the node below it during BEGAN; the node below must not be called.
			{
				auto below_calls = std::make_shared<int>(0);
				Handle below = make_touch_layer();
				Engine::add_child(root, below);
				Handle top = make_touch_layer();
				Engine::add_child(root, top);
				Engine::set_touch_caller(below, [below_calls](Handle, Engine::TOUCH_STATE, glm::vec2, int, int) {
					(*below_calls)++;
				});
				Engine::set_touch_caller(top, [below](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
					if (state == Engine::TOUCH_STATE::TOUCH_BEGAN && Engine::is_valid(below))
						Engine::clean_up(below);
				});
				Engine::process_frame(0.f);
				if (reentry_click(3)) {
					reentry_expect(
						3, *below_calls == 0,
						"destroyed node below got no call (" + std::to_string(*below_calls) + " calls)"
					);
				}
				clean_if_valid({top, below});
			}

			// 4. destroy the node below and create a new one in the same callback (the id is reused):
			//    the new node was never pressed, it must not get the old touch state.
			{
				auto fresh_calls = std::make_shared<std::vector<int>>();
				auto fresh = std::make_shared<Handle>(Handle::make_invalid_handle());
				Handle below = make_touch_layer();
				Engine::add_child(root, below);
				Engine::set_touch_caller(below, [](Handle, Engine::TOUCH_STATE, glm::vec2, int, int) {});
				Handle top = make_touch_layer();
				Engine::add_child(root, top);
				Engine::set_touch_caller(
					top, [below, fresh, fresh_calls](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
						if (state != Engine::TOUCH_STATE::TOUCH_ENDED || !Engine::is_valid(below))
							return;
						Engine::clean_up(below);
						*fresh = make_touch_layer();
						Engine::set_touch_caller(
							*fresh, [fresh_calls](Handle, Engine::TOUCH_STATE s, glm::vec2, int, int) {
								fresh_calls->push_back(s);
							}
						);
						Engine::add_child(Engine::get_root(), *fresh);
					}
				);
				Engine::process_frame(0.f);
				if (reentry_click(4)) {
					bool reused = fresh->id() == below.id();
					reentry_expect(
						4, fresh_calls->empty(),
						std::string("node created in the callback got no touch call (") +
							std::to_string(fresh_calls->size()) + " calls, id reused: " + (reused ? "yes" : "no") + ")"
					);
				}
				clean_if_valid({top, below, *fresh});
			}

			// 5. a node with only a click caller is disabled by the node above during BEGAN,
			//    then reaches the "disabled" branch of the same dispatch.
			{
				auto disabled_clicks = std::make_shared<int>(0);
				auto both_clicks = std::make_shared<int>(0);
				Handle click_only = make_touch_layer();
				Engine::add_child(root, click_only);
				Engine::set_click_caller(click_only, [disabled_clicks](Handle, glm::vec2, int, int) { (*disabled_clicks)++; });
				// same, but with a touch caller too (the other path through the disabled branch).
				Handle both = make_touch_layer();
				Engine::add_child(root, both);
				Engine::set_touch_caller(both, [](Handle, Engine::TOUCH_STATE, glm::vec2, int, int) {});
				Engine::set_click_caller(both, [both_clicks](Handle, glm::vec2, int, int) { (*both_clicks)++; });
				Handle top = make_touch_layer();
				Engine::add_child(root, top);
				Engine::set_touch_caller(top, [click_only, both](Handle, Engine::TOUCH_STATE state, glm::vec2, int, int) {
					if (state == Engine::TOUCH_STATE::TOUCH_BEGAN) {
						Engine::set_touch_enabled(click_only, false);
						Engine::set_touch_enabled(both, false);
					}
				});
				Engine::process_frame(0.f);
				if (reentry_click(5)) {
					reentry_expect(5, true, "disabling a click-only node during the dispatch did not throw");
					reentry_expect(
						5, *disabled_clicks == 0,
						"click-only node disabled mid-touch got no click (" + std::to_string(*disabled_clicks) +
							" clicks)"
					);
					reentry_expect(
						5, *both_clicks == 0,
						"touch + click node disabled mid-touch got no click (" + std::to_string(*both_clicks) +
							" clicks)"
					);
				}
				clean_if_valid({top, click_only, both});
			}

			// 6. clean_up the same node twice before the queue runs (e.g. a button click and a scene clear in one
			//    frame). The id must be released once: two nodes created afterwards must get different ids.
			{
				Handle twice = Engine::create_layout();
				Engine::add_child(root, twice);
				Engine::process_frame(0.f);
				try {
					Engine::clean_up(twice);
					Engine::clean_up(twice);
					Engine::process_frame(0.f);
					Handle n1 = Engine::create_layout();
					Handle n2 = Engine::create_layout();
					reentry_expect(
						6, n1.id() != n2.id(),
						"double clean_up released the id once (new ids " + std::to_string(n1.id()) + ", " +
							std::to_string(n2.id()) + ")"
					);
					Engine::add_child(root, n1);
					Engine::add_child(root, n2);
					Engine::process_frame(0.f);
					clean_if_valid({n1, n2});
				} catch (const std::exception& exception) {
					reentry_expect(6, false, std::string("double clean_up threw: ") + exception.what());
				}
			}

			Engine::process_frame(0.f);
			verify_tree(true);
			print_pool_diff(get_baseline(), count_pools(), "after touch re-entrancy repro");
			if (reentry_failed == 0) {
				Log::info("[repro] survived touch re-entrancy, all 6 cases pass");
			} else {
				Log::info("[repro] touch re-entrancy:", reentry_failed, "checks failed");
			}
		}

		/**
		 * @Note: B starts inside clipping A, is detached, A is destroyed, then B is
		 * added back. If add_child does not refresh clipping, B still points at A's clip handle.
		 */
		void repro_reparent_out_of_clip() {
			Log::info("[repro] move child out of a clipping parent, then destroy the old parent");
			Handle a = Engine::create_layout();
			Engine::set_content_size(a, {200.f, 200.f});
			Engine::set_layout_clipping_enabled(a, true);
			Engine::add_child(Engine::get_root(), a);
			Handle b = Engine::create_layout();
			Engine::set_content_size(b, {50.f, 50.f});
			Engine::set_layout_background_enabled(b, true);
			Engine::add_child(a, b);
			// labels and images have no Clipping_Information but still carry clipping ancestors.
			Handle label = Engine::create_label();
			Engine::set_label_string(label, "moved label");
			Engine::add_child(a, label);
			Handle image = Engine::create_image();
			Engine::set_image_path(image, "res/AddonIcon5.png");
			Engine::add_child(a, image);
			Handle c = Engine::create_layout();
			Engine::set_content_size(c, {200.f, 200.f});
			Engine::add_child(Engine::get_root(), c);
			Engine::process_frame(0.f);
			// B: detach then reattach. label + image: direct reparent with add_child.
			Engine::remove_child(b);
			Engine::add_child(c, label);
			Engine::add_child(c, image);
			Engine::process_frame(0.f);
			Engine::clean_up(a);
			Engine::process_frame(0.f);
			Engine::add_child(c, b);
			// force every moved draw to rebuild so it reads its clipping ancestors again.
			Engine::set_position(b, {10.f, 10.f});
			Engine::set_position(label, {20.f, 20.f});
			Engine::set_position(image, {30.f, 30.f});
			Engine::process_frame(0.f);
			Log::info("[repro] survived, layout + label + image no longer use A's clip");
			Engine::clean_up(c);
		}

		/**
		 * @Note: a detached node that gets touch-dirty is added to the touch listener
		 * list; clean_up while detached skips the listener cleanup in remove_child.
		 */
		void repro_detached_touch_listener() {
			Log::info("[repro] child becomes touch-dirty, parent is detached in the same frame, then clean_up");
			Handle parent = Engine::create_layout();
			Engine::set_content_size(parent, {100.f, 100.f});
			Engine::add_child(Engine::get_root(), parent);
			Handle child = Engine::create_layout();
			Engine::set_content_size(child, {50.f, 50.f});
			Engine::add_child(parent, child);
			Engine::process_frame(0.f);
			// same frame: touch-dirty first, then detach. The dirty flag survives the detach.
			Engine::set_touch_enabled(child, true);
			Engine::remove_child(parent);
			Engine::process_frame(0.f);
			Engine::clean_up(parent);
			// any later touch-dirty node makes the listener list sort again.
			Handle other = Engine::create_layout();
			Engine::set_touch_enabled(other, true);
			Engine::add_child(Engine::get_root(), other);
			Engine::process_frame(0.f);
			Log::info("[repro] survived, no deleted node left in the touch listener list");
			Engine::clean_up(other);
		}

		void repro_scroll_clean_up() {
			Log::info("[repro] clean_up a scroll layout, then check tree + pools");
			auto before = count_pools();
			Handle scroll = Engine::create_scroll_layout();
			Engine::set_content_size(scroll, {100.f, 100.f});
			Engine::add_child(Engine::get_root(), scroll);
			Engine::process_frame(0.f);
			Engine::clean_up(scroll);
			Engine::process_frame(0.f);
			verify_tree(true);
			print_pool_diff(before, count_pools(), "scroll layout clean_up");
		}

		void repro_empty_image_path() {
			Log::info("[repro] set_image_path(\"\") then render");
			Handle image = Engine::create_image();
			Engine::add_child(Engine::get_root(), image);
			Engine::process_frame(0.f);
			Engine::set_image_path(image, "");
			Engine::process_frame(0.f);
			Log::info("[repro] survived empty image path");
			Engine::clean_up(image);
		}

		/**
		 * @Note: renderer handles are created lazily in process_frame, so a node
		 * cleaned before its first frame has empty Renderer_Handles.
		 */
		void repro_same_frame_clean_up() {
			Log::info("[repro] add layout + label + image to root and clean_up before any frame");
			Handle layout = Engine::create_layout();
			Engine::set_layout_background_enabled(layout, true);
			Engine::add_child(Engine::get_root(), layout);
			Handle label = Engine::create_label();
			Engine::add_child(layout, label);
			Handle image = Engine::create_image();
			Engine::add_child(layout, image);
			Engine::clean_up(layout);
			Log::info("[repro] survived same-frame create + clean_up");
		}
	} // namespace

	void print_help() {
		Log::info("================ UI TEST KEYS ================");
		Log::info("1 = modal scene | 2 = colour picker | 3 = component lab | 4 = label lab");
		Log::info("F = fuzz current scene (new seed) | Shift+F = fuzz again with last seed");
		Log::info("S = stress: 90 build/clear cycles, then leak check");
		Log::info("R = clear scene + leak check | P = print pools + verify tree | H = help");
		Log::info("C = create + clean_up in the same frame (repro: crashes before the fix)");
		Log::info("X = reparent out of a clipping parent, destroy old parent (repro)");
		Log::info("E = image with empty path (repro)");
		Log::info("L = clean_up a scroll layout, check orphans + pools (repro)");
		Log::info("U = clean_up a detached touchable node (repro)");
		Log::info("K = checks for the ui_api_need.txt items marked Handled (find, scroll offset, world, click)");
		Log::info("J = engine calls inside touch callbacks: size query, self destroy, destroy + create (repro #25)");
		Log::info("==============================================");
	}

	void start() {
		// one frame so the root renders, then the baseline only holds engine-owned entities.
		Engine::process_frame(0.f);
		get_baseline() = count_pools();
		print_pools(get_baseline(), "baseline (root only)");
		print_help();
		build_modal_scene();
	}

	bool handle_key(int key, int mods) {
		if (is_running_job() && key != GLFW_KEY_H) {
			Log::info("[test] a fuzz/stress job is running, wait for it to finish");
			return true;
		}
		switch (key) {
			case GLFW_KEY_1: {
				build_modal_scene();
				return true;
			}
			case GLFW_KEY_2: {
				build_picker_scene();
				return true;
			}
			case GLFW_KEY_3: {
				build_lab_scene();
				return true;
			}
			case GLFW_KEY_F: {
				if (!(mods & GLFW_MOD_SHIFT)) {
					last_fuzz_seed = (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count();
				}
				start_fuzz(last_fuzz_seed, 300, 12);
				return true;
			}
			case GLFW_KEY_S: {
				start_stress(90);
				return true;
			}
			case GLFW_KEY_R: {
				clear_scene();
				Engine::process_frame(0.f); // clean_up is queued until the next frame
				verify_tree(true);
				print_pool_diff(get_baseline(), count_pools(), "after clear");
				return true;
			}
			case GLFW_KEY_P: {
				print_pools(count_pools(), "current");
				verify_tree(true);
				return true;
			}
			case GLFW_KEY_H: {
				print_help();
				return true;
			}
			case GLFW_KEY_C: {
				repro_same_frame_clean_up();
				return true;
			}
			case GLFW_KEY_X: {
				repro_reparent_out_of_clip();
				return true;
			}
			case GLFW_KEY_E: {
				repro_empty_image_path();
				return true;
			}
			case GLFW_KEY_L: {
				repro_scroll_clean_up();
				return true;
			}
			case GLFW_KEY_U: {
				repro_detached_touch_listener();
				return true;
			}
			case GLFW_KEY_4: {
				build_label_scene();
				return true;
			}
			case GLFW_KEY_K: {
				repro_api_checks();
				return true;
			}
			case GLFW_KEY_J: {
				repro_touch_reentrancy();
				return true;
			}
			default: {
				return false;
			}
		}
	}

} // namespace Test
