#include <algorithm>
#include <cstdlib>
#include <engine/components.h>
#include <engine/systems.h>
#include <id_generator.h>
#include <log.h>
#include <random>
#include <test_ui.h>
#include <unordered_set>
#include <vulkan/vk_draw.h>

/**
 * @Note: engine.cpp globals (non-static), read only to verify invariants.
 */
namespace Engine {
	extern Handle_Generator entity_generator;
	extern std::vector<Handle> entities_touch_listener;
	extern std::vector<Handle> entities_scroll_listener;
	extern std::vector<Handle> clean_up_entities;
	Handle get_clipping_renderer_handle(const Handle& e);
	CLIPPING_STATE get_clipping_state_by_ancestors(const Handle& e);
} // namespace Engine

namespace Test {

	// ============================== POOLS ==============================

	template <typename T> size_t count_pool() {
		size_t count = 0;
		Engine::get_registry().each<T>([&count](uint32_t, T&) { count++; });
		return count;
	}

	std::vector<Pool_Count> count_pools() {
		using namespace Engine;
		return {
			{"Description", count_pool<Description>()},
			{"Tree_Information", count_pool<Tree_Information>()},
			{"Transform_2D", count_pool<Transform_2D>()},
			{"World_Transform_2D", count_pool<World_Transform_2D>()},
			{"Dirty_Flag", count_pool<Dirty_Flag>()},
			{"Position_Percent", count_pool<Position_Percent>()},
			{"Clipping_Ancestor_Information", count_pool<Clipping_Ancestor_Information>()},
			{"Size", count_pool<Size>()},
			{"Size_Percent", count_pool<Size_Percent>()},
			{"Size_Pinning", count_pool<Size_Pinning>()},
			{"Renderer_Handles", count_pool<Renderer_Handles>()},
			{"Touching_Information", count_pool<Touching_Information>()},
			{"Layout_Renderer_Information", count_pool<Layout_Renderer_Information>()},
			{"Clipping_Information", count_pool<Clipping_Information>()},
			{"Label_Renderer_Information", count_pool<Label_Renderer_Information>()},
			{"Image_Renderer_Information", count_pool<Image_Renderer_Information>()},
			{"Inner_Renderer_Size", count_pool<Inner_Renderer_Size>()},
			{"Nine_Scale_Information", count_pool<Nine_Scale_Information>()},
			{"Nine_Scale_Renderer_Handles", count_pool<Nine_Scale_Renderer_Handles>()},
			{"Scroll_Information", count_pool<Scroll_Information>()},
			{"Draw_Order", count_pool<Draw_Order>()},
		};
	}

	std::vector<Pool_Count>& get_baseline() {
		static std::vector<Pool_Count> baseline{};
		return baseline;
	}

	void print_pools(const std::vector<Pool_Count>& pools, const std::string& title) {
		Log::info("[pools]", title);
		for (const auto& pool : pools) {
			Log::info("   ", pool.name, pool.count);
		}
	}

	size_t print_pool_diff(
		const std::vector<Pool_Count>& before, const std::vector<Pool_Count>& after, const std::string& title
	) {
		size_t differences = 0;
		for (size_t i = 0; i < before.size() && i < after.size(); i++) {
			if (before[i].count == after[i].count)
				continue;
			if (differences == 0) {
				Log::info("[leak]", title, "- pools that differ from baseline:");
			}
			differences++;
			long long delta = (long long)after[i].count - (long long)before[i].count;
			Log::info("   ", after[i].name, before[i].count, "->", after[i].count, "( delta", delta, ")");
		}
		if (differences == 0) {
			Log::info("[leak]", title, "- OK, every pool matches baseline");
		}
		return differences;
	}

	// ============================== TREE ==============================

	size_t verify_tree(bool log_issues) {
		using namespace Engine;
		Registry& registry = get_registry();
		const Handle& root = get_root();
		size_t issues = 0;
		auto report = [&issues, log_issues](const std::string& message, uint32_t a, uint32_t b) {
			issues++;
			if (log_issues && issues <= 12) {
				Log::info("[tree]", message, "| node", a, "| other", b);
			}
		};

		std::unordered_set<uint32_t> visited{root.id()};
		std::vector<Handle> stack{root};
		while (!stack.empty()) {
			Handle node = stack.back();
			stack.pop_back();
			if (!registry.has<Tree_Information>(node.id()))
				continue;
			const auto& [node_info] = registry.read<Tree_Information>(node.id());
			for (const auto& child : node_info.children) {
				if (!registry.has<Tree_Information>(child.id())) {
					report("child linked in parent but its components are gone", child.id(), node.id());
					continue;
				}
				if (visited.count(child.id())) {
					report("child reached twice (duplicate link or cycle)", child.id(), node.id());
					continue;
				}
				visited.insert(child.id());
				const auto& [child_info] = registry.read<Tree_Information>(child.id());
				if (!same(child_info.parent, node)) {
					report("child.parent does not point back to the parent that lists it", child.id(), node.id());
				}
				if (!child_info.valid_child) {
					report("child reachable from root but valid_child = false", child.id(), node.id());
				}
				if (child_info.depth != node_info.depth + 1) {
					report("depth is not parent depth + 1", child.id(), node.id());
				}
				stack.push_back(child);
			}
		}

		// everything reachable from detached subtrees is expected to be off the tree.
		std::unordered_set<uint32_t> detached_ids{};
		for (const auto& e : get_scene().detached) {
			for (const auto& node : collect_subtree(e)) {
				detached_ids.insert(node.id());
			}
		}
		// clean_up is deferred to the next process_frame: queued subtrees are detached but still alive.
		for (const auto& e : Engine::clean_up_entities) {
			if (!Engine::entity_generator.validate_handle(e))
				continue;
			for (const auto& node : collect_subtree(e)) {
				detached_ids.insert(node.id());
			}
		}
		registry.each<Tree_Information>([&](uint32_t e, Tree_Information& info) {
			if (visited.count(e) || detached_ids.count(e))
				return;
			report("entity alive but not reachable from root (orphan / leak)", e, info.parent.id());
		});
		registry.each<Description>([&](uint32_t e, Description&) {
			if (!registry.has<Tree_Information>(e)) {
				report("Description left without Tree_Information (half removed)", e, 0);
			}
		});
		for (const auto& e : get_scene().nodes) {
			if (!registry.has<Tree_Information>(e.id())) {
				report("tracked handle lost its components without destroy()", e.id(), 0);
			}
		}
		if (log_issues) {
			if (issues == 0) {
				Log::info("[tree] OK, nodes reachable from root:", visited.size());
			} else {
				Log::info("[tree] issues found:", issues, issues > 12 ? "(first 12 shown)" : "");
			}
		}
		return issues;
	}

	// ============================== JOBS ==============================

	namespace {
		enum class JOB {
			NONE,
			FUZZ,
			STRESS
		};

		struct Job_State {
			JOB type = JOB::NONE;
			int frames_left = 0;
			int frame = 0;
			int ops_per_frame = 0;
			uint32_t seed = 0;
			size_t exceptions = 0;
			size_t tree_issue_frames = 0;
			bool logged_tree = false;
		};

		Job_State job{};
		std::mt19937 rng{};

		/**
		 * @Note: nodes created this frame have no renderer handles yet, and
		 * remove_child on them crashes (see key C repro), so detach / reparent /
		 * destroy skip them until the next frame.
		 */
		std::vector<Handle> fresh_nodes{};

		bool is_fresh(const Handle& e) {
			for (const auto& fresh : fresh_nodes) {
				if (same(fresh, e))
					return true;
			}
			return false;
		}

		/**
		 * @Note: UI_TEST_AVOID_KNOWN=1 skips the triggers of bugs already
		 * reported (empty image path, reattach after detach) to search deeper.
		 */
		bool avoid_known_bugs() {
			static bool avoid = std::getenv("UI_TEST_AVOID_KNOWN") != nullptr;
			return avoid;
		}

		bool is_detached(const Handle& e) {
			for (const auto& d : get_scene().detached) {
				for (const auto& node : collect_subtree(d)) {
					if (same(node, e))
						return true;
				}
			}
			return false;
		}

		int random_int(int min, int max) {
			return std::uniform_int_distribution<int>(min, max)(rng);
		}

		float random_float(float min, float max) {
			return std::uniform_real_distribution<float>(min, max)(rng);
		}

		bool chance(float probability) {
			return random_float(0.f, 1.f) < probability;
		}

		Handle pick_node(bool allow_scene_root) {
			auto& nodes = get_scene().nodes;
			if (nodes.empty())
				return Handle::make_invalid_handle();
			for (int attempt = 0; attempt < 8; attempt++) {
				const Handle& e = nodes[random_int(0, (int)nodes.size() - 1)];
				if (allow_scene_root || !same(e, get_scene().root))
					return e;
			}
			return Handle::make_invalid_handle();
		}

		glm::vec3 random_color() {
			return {random_float(0.f, 1.f), random_float(0.f, 1.f), random_float(0.f, 1.f)};
		}

		const char* random_text() {
			static const char* texts[] = {"", "x", "Hello", "Two\nlines", "A long label string for measuring", "\n\n"};
			return texts[random_int(0, 5)];
		}

		const char* random_image() {
			static const char* paths[] = {
				"res/AddonIcon5.png", "res/BtnGreen.png", "res/GemsBarBg.png", "res/missing_file.png", ""
			};
			return paths[random_int(0, avoid_known_bugs() ? 3 : 4)];
		}

		Handle create_random_node(const Handle& parent) {
			Handle e = Handle::make_invalid_handle();
			switch (random_int(0, 4)) {
				case 0: {
					e = Engine::create_node();
					break;
				}
				case 1: {
					e = Engine::create_layout();
					Engine::set_content_size(e, {random_float(10.f, 200.f), random_float(10.f, 200.f)});
					Engine::set_layout_background_enabled(e, true);
					Engine::set_layout_background_color(e, random_color());
					break;
				}
				case 2: {
					e = Engine::create_label();
					Engine::set_label_font_path(e, FONT_PATH);
					Engine::set_label_string(e, random_text());
					break;
				}
				case 3: {
					e = Engine::create_image();
					Engine::set_image_path(e, random_image());
					break;
				}
				default: {
					e = Engine::create_scroll_layout();
					Engine::set_content_size(e, {120.f, 120.f});
					Engine::set_inner_container_size(e, {120.f, 400.f});
					break;
				}
			}
			Engine::set_position(e, {random_float(0.f, 300.f), random_float(0.f, 300.f)});
			Engine::add_child(parent, e);
			fresh_nodes.push_back(e);
			return track(e);
		}

		/**
		 * @Note: one random API call. Return the op name for logging.
		 */
		Handle last_target = Handle::make_invalid_handle();

		std::string run_random_op() {
			auto& scene = get_scene();
			Handle e = pick_node(false);
			last_target = e;
			if (e.is_invalid())
				return "skip";
			int op = random_int(0, 23);
			if (avoid_known_bugs() && (op == 20 || (op == 18 && is_detached(e)))) {
				return "skip (known bug: reattach)";
			}
			if (op == 18 || op == 19 || op == 21) {
				// remove_child walks the whole subtree, so a fresh descendant crashes too.
				for (const auto& node : collect_subtree(e)) {
					if (is_fresh(node))
						return "skip (fresh node in subtree)";
				}
			}
			switch (op) {
				case 0:
					Engine::set_position(
						e, Engine::get_position(e) + glm::vec2(random_float(-40.f, 40.f), random_float(-40.f, 40.f))
					);
					return "set_position";
				case 1:
					Engine::set_scale(e, glm::vec2(random_float(0.3f, 1.8f)));
					return "set_scale";
				case 2:
					Engine::set_rotation(e, random_float(-45.f, 45.f));
					return "set_rotation";
				case 3:
					Engine::set_opacity(e, random_float(-0.5f, 1.5f));
					return "set_opacity";
				case 4:
					Engine::set_color(e, random_color());
					return "set_color";
				case 5:
					Engine::set_anchor(e, {random_float(0.f, 1.f), random_float(0.f, 1.f)});
					return "set_anchor";
				case 6:
					Engine::set_visible(e, !Engine::is_visible(e));
					return "set_visible";
				case 7:
					Engine::set_z_order(e, (uint32_t)random_int(0, 6));
					return "set_z_order";
				case 8:
					Engine::set_touch_enabled(e, chance(0.5f));
					return "set_touch_enabled";
				case 9:
					Engine::set_content_size(e, {random_float(0.f, 300.f), random_float(0.f, 300.f)});
					return "set_content_size";
				case 10:
					Engine::set_layout_background_enabled(e, chance(0.8f));
					Engine::set_layout_background_color(e, random_color());
					Engine::set_layout_border_radius(e, random_float(0.f, 40.f));
					Engine::set_layout_border_width(e, random_float(0.f, 8.f));
					Engine::set_layout_border_color(e, random_color());
					return "layout_style";
				case 11:
					Engine::set_layout_clipping_enabled(e, chance(0.5f));
					return "set_layout_clipping_enabled";
				case 12:
					Engine::set_label_string(e, random_text());
					Engine::set_label_font_size(e, random_int(6, 40));
					Engine::set_label_stroke_size(e, random_float(0.f, 3.f));
					return "label_text";
				case 13:
					Engine::set_image_path(e, random_image());
					return "set_image_path";
				case 14:
					Engine::set_nine_scale_enabled(e, chance(0.5f));
					Engine::set_nine_scale_rect_ratio(e, {0.2f, 0.2f, 0.6f, 0.6f});
					return "nine_scale";
				case 15:
					Engine::set_size_percent(e, {random_float(0.f, 1.2f), random_float(0.f, 1.2f)});
					return "set_size_percent";
				case 16:
					Engine::set_size_pinning(
						e, {random_float(0.f, 30.f), random_float(0.f, 30.f), random_float(0.f, 30.f),
							random_float(0.f, 30.f)}
					);
					return "set_size_pinning";
				case 17:
					Engine::set_position_percent(e, {random_float(-0.2f, 1.2f), random_float(-0.2f, 1.2f)});
					return "set_position_percent";
				case 18: {
					Handle parent = pick_node(true);
					if (!parent.is_invalid())
						Engine::add_child(parent, e);
					return "add_child (reparent)";
				}
				case 19: {
					Engine::remove_child(e);
					bool known = false;
					for (const auto& d : scene.detached)
						known |= same(d, e);
					if (!known)
						scene.detached.push_back(e);
					return "remove_child (detach)";
				}
				case 20: {
					if (scene.detached.empty())
						return "reattach (none)";
					int index = random_int(0, (int)scene.detached.size() - 1);
					Handle d = scene.detached[index];
					Handle parent = pick_node(true);
					if (parent.is_invalid())
						return "reattach (no parent)";
					Engine::add_child(parent, d);
					const auto& [info] = Engine::get_registry().read<Engine::Tree_Information>(d.id());
					if (!info.parent.is_invalid()) {
						scene.detached.erase(scene.detached.begin() + index);
					}
					return "reattach";
				}
				case 21: {
					destroy(e);
					return "destroy (clean_up)";
				}
				case 22: {
					Handle parent = pick_node(true);
					// a node created under a detached parent never renders (known bug 1 on removal).
					if (avoid_known_bugs() && is_detached(parent))
						return "skip (known bug: create under detached)";
					create_random_node(parent);
					return "create random node";
				}
				default: {
					Engine::get_content_size(e);
					Engine::get_inner_renderer_size(e);
					Engine::get_inner_container_size(e);
					Engine::get_label_string(e);
					Engine::get_size_pinning(e);
					return "queries";
				}
			}
		}

		std::vector<std::string> last_frame_ops{};
		bool logged_dead_clip = false;

		bool is_draw_handle_alive(const Handle& handle) {
			try {
				Vulkan::assert_draw_handle(handle);
				return true;
			} catch (const std::exception&) {
				return false;
			}
		}

		size_t clip_cull_misses = 0;

		// independent separating-axis classification (not the engine's): 0 out, 1 in, 2 intersect.
		int classify_rect(const std::array<glm::vec2, 4>& node, const std::array<glm::vec2, 4>& clip) {
			const glm::vec2 axes[4] = {node[1] - node[0], node[3] - node[0], clip[1] - clip[0], clip[3] - clip[0]};
			bool inside = true;
			for (int k = 0; k < 4; k++) {
				float n_min = 1e30f, n_max = -1e30f, c_min = 1e30f, c_max = -1e30f;
				for (int i = 0; i < 4; i++) {
					float n = glm::dot(node[i], axes[k]);
					float c = glm::dot(clip[i], axes[k]);
					n_min = std::min(n_min, n), n_max = std::max(n_max, n);
					c_min = std::min(c_min, c), c_max = std::max(c_max, c);
				}
				if (n_max < c_min || c_max < n_min)
					return 0;
				// contained on the clip's own axes (k >= 2) decides "fully in"; small epsilon for float noise.
				float eps = 1e-3f * std::max(1.f, std::abs(c_max - c_min));
				if (k >= 2 && (n_min < c_min - eps || n_max > c_max + eps))
					inside = false;
			}
			return inside ? 1 : 2;
		}

		/**
		 * @Note: ancestors of e with clipping enabled, outermost first, keeping
		 * only the nearest MAX_DEPTH_CLIPPING (same rule as add_clipping_ancestor).
		 */
		std::vector<Handle> expected_clipping_ancestors(uint32_t e) {
			using namespace Engine;
			Registry& registry = get_registry();
			std::vector<Handle> ancestors;
			Handle parent = std::get<0>(registry.read<Tree_Information>(e)).parent;
			while (!parent.is_invalid() && registry.has<Tree_Information>(parent.id())) {
				if (registry.has<Clipping_Information>(parent.id()) &&
					std::get<0>(registry.read<Clipping_Information>(parent.id())).enabled) {
					ancestors.push_back(parent);
				}
				parent = std::get<0>(registry.read<Tree_Information>(parent.id())).parent;
			}
			std::reverse(ancestors.begin(), ancestors.end());
			if (ancestors.size() > ::Const::MAX_DEPTH_CLIPPING) {
				ancestors.erase(ancestors.begin(), ancestors.end() - ::Const::MAX_DEPTH_CLIPPING);
			}
			return ancestors;
		}

		/**
		 * @Note: after a full frame, for every node in the tree:
		 * 1. clipping_ancesors (entity handles) must be exactly its live ancestors
		 *    with clipping enabled, outermost first.
		 * 2. every draw of the node must upload the clip handles of those ancestors
		 *    (skipping ancestors without a clip handle), and each one must still exist.
		 * Returns the number of nodes that break either rule.
		 */
		size_t check_dead_clip_holders(bool log_issues) {
			using namespace Engine;
			Registry& registry = get_registry();
			size_t holders = 0;
			auto report = [&](uint32_t e, const Tree_Information& tree_info, const std::string& issue) {
				holders++;
				if (log_issues && holders <= 5) {
					int type =
						registry.has<Description>(e) ? (int)std::get<0>(registry.read<Description>(e)).value : -1;
					Log::info("[clip] node", e, "type", type, "parent", tree_info.parent.id(), issue);
				}
			};
			registry.each<Clipping_Ancestor_Information, Tree_Information>([&](uint32_t e,
																			   Clipping_Ancestor_Information& clipping,
																			   Tree_Information& tree_info) {
				if (!tree_info.valid_child || tree_info.parent.is_invalid())
					return;
				auto expected = expected_clipping_ancestors(e);
				std::string issue = "";
				for (int i = 0; i < clipping.number_clipping && issue.empty(); i++) {
					const Handle& ancestor = clipping.clipping_ancestors[i];
					if (!entity_generator.validate_handle(ancestor)) {
						issue = "holds dead clip ancestor " + std::to_string(ancestor.id()) + " at slot " +
								std::to_string(i);
					} else if (std::find_if(expected.begin(), expected.end(), [&ancestor](const Handle& a) {
								   return same(a, ancestor);
							   }) == expected.end()) {
						issue = "holds clip ancestor " + std::to_string(ancestor.id()) + " at slot " +
								std::to_string(i) + " that is not a clipping ancestor any more";
					}
				}
				if (issue.empty()) {
					bool same_list = clipping.number_clipping == expected.size();
					for (int i = 0; same_list && i < clipping.number_clipping; i++) {
						same_list = same(clipping.clipping_ancestors[i], expected[i]);
					}
					if (!same_list) {
						issue = "has " + std::to_string(clipping.number_clipping) + " clip ancestors, expected " +
								std::to_string(expected.size()) + " (missing or out of order)";
					}
				}
				if (!issue.empty()) {
					report(e, tree_info, issue);
					return;
				}

				// CPU clip culling: fully out of any ancestor -> draw hidden (clip list not checked);
				// fully inside every ancestor -> no ancestor clips; otherwise every ancestor clip.
				int state = 1;
				if (registry.has<Rect>(e)) {
					const auto& node_rect = std::get<0>(registry.read<Rect>(e)).transformed_rect;
					for (const auto& ancestor : expected) {
						if (!registry.has<Rect>(ancestor.id()))
							continue;
						int s =
							classify_rect(node_rect, std::get<0>(registry.read<Rect>(ancestor.id())).transformed_rect);
						if (s == 0) {
							state = 0;
							break;
						}
						if (s == 2)
							state = 2;
					}
				}
				// correct lists: `full` = every ancestor clip (+ own); `culled` = own clip only (ancestors fully in).
				// fully in   -> culled, or full (a missed cull: right picture, wasted GPU)
				// intersect  -> full
				// fully out  -> hidden, or visible with full (missed cull)
				std::vector<Handle> full_clips, culled_clips;
				for (const auto& ancestor : expected) {
					Handle clip = get_clipping_renderer_handle(ancestor);
					if (!clip.is_invalid())
						full_clips.push_back(clip);
				}
				// a custom-size label whose text overflows clips itself with its own clip (renderer handle 1),
				// appended after its ancestors; at the depth limit the oldest ancestor is dropped.
				std::string label_text = "";
				if (registry.has<Label_Renderer_Information>(e)) {
					const auto& [label] = registry.read<Label_Renderer_Information>(e);
					const auto& handles = std::get<0>(registry.read<Renderer_Handles>(e)).value;
					label_text =
						" | label custom " +
						std::to_string((label.fit_mode == Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO)) +
						" clip " + std::to_string(label.clipping_enabled);
					if (label.fit_mode == Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO &&
						label.clipping_enabled && handles.size() > 1) {
						if ((int)full_clips.size() >= ::Const::MAX_DEPTH_CLIPPING)
							full_clips.erase(full_clips.begin());
						full_clips.push_back(handles[1]);
						culled_clips.push_back(handles[1]);
						label_text += " own clip " + std::to_string(handles[1].id());
					}
				}
				auto list_text = [](const std::vector<Handle>& list) {
					std::string text = "[";
					for (const auto& c : list)
						text += std::to_string(c.id()) + " ";
					return text + "]";
				};
				for (const auto& renderer_handle : get_renderer_handles(Handle(e, 0))) {
					if (!is_draw_handle_alive(renderer_handle))
						continue;
					// modify_ is the only accessor; reading here only re-marks the draw for upload.
					const auto& draw_attributes = Vulkan::modify_draw_attributes(renderer_handle);
					if (state == 0 && !draw_attributes.is_visible())
						continue;
					uint32_t number = draw_attributes.get_number_clipping();
					const auto& uploaded_array = draw_attributes.get_clipping_ancestors();
					std::vector<Handle> uploaded;
					for (uint32_t i = 0; i < number && i < uploaded_array.size(); i++) {
						if (!is_draw_handle_alive(uploaded_array[i])) {
							issue = "draw " + std::to_string(renderer_handle.id()) + " uploads dead clip handle " +
									std::to_string(uploaded_array[i].id()) + " at slot " + std::to_string(i);
							report(e, tree_info, issue);
							return;
						}
						uploaded.push_back(uploaded_array[i]);
					}
					auto equal = [&](const std::vector<Handle>& want) {
						if (want.size() != uploaded.size())
							return false;
						for (size_t i = 0; i < want.size(); i++)
							if (!same(want[i], uploaded[i]))
								return false;
						return true;
					};
					bool is_full = equal(full_clips);
					bool ok = state == 2 ? is_full : state == 1 ? (equal(culled_clips) || is_full) : is_full;
					if (!ok) {
						const char* names[] = {"OUT(visible)", "IN", "INTERSECT"};
						issue = "draw " + std::to_string(renderer_handle.id()) + " state " + names[state] +
								" (engine " + std::to_string((int)get_clipping_state_by_ancestors(Handle(e, 0))) +
								") uploads " + list_text(uploaded) + " want " +
								(state == 1 ? list_text(culled_clips) + " or " : "") + list_text(full_clips) +
								label_text;
						report(e, tree_info, issue);
						return;
					}
					if (state != 2 && is_full && !full_clips.empty() && full_clips.size() != culled_clips.size()) {
						clip_cull_misses++;
						if (log_issues && clip_cull_misses <= 3)
							Log::info(
								"[clip-cull] miss: node", e, state == 1 ? "fully in" : "fully out but visible",
								"still uploads", list_text(uploaded)
							);
					}
				}
			});
			return holders;
		}

		bool logged_draw_order = false;

	} // namespace

	size_t check_draw_order(bool log_issues) {
		using namespace Engine;
		Registry& registry = get_registry();
		size_t issues = 0;
		std::vector<Handle> order;
		std::vector<Handle> stack{get_root()};
		while (!stack.empty()) {
			Handle current = stack.back();
			stack.pop_back();
			if (!registry.has<Tree_Information>(current.id()))
				continue;
			order.push_back(current);
			const auto& children = std::get<0>(registry.read<Tree_Information>(current.id())).children;
			for (int i = (int)children.size() - 1; i >= 0; i--) {
				stack.push_back(children[i]);
			}
		}
		auto report = [&](uint32_t e, uint32_t expected, const std::string& issue) {
			issues++;
			if (log_issues && issues <= 8) {
				int type = registry.has<Description>(e) ? (int)std::get<0>(registry.read<Description>(e)).value : -1;
				Log::info("[draw-order] node", e, "type", type, "expected draw_index", expected, issue);
			}
		};
		for (uint32_t i = 0; i < order.size(); i++) {
			uint32_t e = order[i].id();
			const auto& children = std::get<0>(registry.read<Tree_Information>(e)).children;
			for (size_t c = 1; c < children.size(); c++) {
				uint32_t z_prev = std::get<0>(registry.read<Transform_2D>(children[c - 1].id())).z_order;
				uint32_t z_next = std::get<0>(registry.read<Transform_2D>(children[c].id())).z_order;
				if (z_prev > z_next) {
					report(
						e, i,
						"children not sorted by z order (" + std::to_string(children[c - 1].id()) + " z " +
							std::to_string(z_prev) + " before " + std::to_string(children[c].id()) + " z " +
							std::to_string(z_next) + ")"
					);
					break;
				}
			}
			if (registry.has<Draw_Order>(e)) {
				uint32_t order_value = std::get<0>(registry.read<Draw_Order>(e)).value;
				if (order_value != i) {
					report(e, i, "Draw_Order " + std::to_string(order_value));
					continue;
				}
			}
			for (const auto& renderer_handle : get_renderer_handles(Handle(e, 0))) {
				if (!is_draw_handle_alive(renderer_handle))
					continue;
				const auto& draw_attributes = Vulkan::modify_draw_attributes(renderer_handle);
				// hidden draws (unused nine-scale pieces, disabled backgrounds) are never drawn, any index is fine.
				if (draw_attributes.is_visible() && draw_attributes.get_draw_index() != i) {
					report(
						e, i,
						"draw " + std::to_string(renderer_handle.id()) + " uploads draw_index " +
							std::to_string(draw_attributes.get_draw_index())
					);
					break;
				}
			}
		}
		return issues;
	}

	namespace {

		bool logged_stale_listener = false;

		/**
		 * @Note: after a full frame every listener entry must be a live handle
		 * that still has its component, otherwise the next sort / hit test reads a missing one.
		 */
		size_t check_stale_listeners(bool log_issues) {
			using namespace Engine;
			Registry& registry = get_registry();
			size_t stale = 0;
			auto check = [&](const std::vector<Handle>& list, const char* name, bool touch) {
				for (const auto& e : list) {
					bool alive = entity_generator.validate_handle(e);
					bool has_component =
						touch ? registry.has<Touching_Information>(e.id()) : registry.has<Scroll_Information>(e.id());
					if (alive && has_component)
						continue;
					stale++;
					if (log_issues) {
						Log::info(
							"[listener]", name, "holds entity", e.id(), "generation", e.generation(), "| handle alive",
							alive, "| has component", has_component
						);
					}
				}
			};
			check(entities_touch_listener, "touch list", true);
			check(entities_scroll_listener, "scroll list", false);
			return stale;
		}

		void fuzz_frame() {
			fresh_nodes.clear();
			if (!logged_stale_listener && check_stale_listeners(false) > 0) {
				logged_stale_listener = true;
				Log::info("[listener] stale entry after frame", job.frame - 1, "- ops of that frame:");
				for (const auto& op : last_frame_ops) {
					Log::info("   ", op);
				}
				check_stale_listeners(true);
			}
			if (!logged_draw_order && check_draw_order(false) > 0) {
				logged_draw_order = true;
				Log::info("[draw-order] broken after frame", job.frame - 1, "- ops of that frame:");
				for (const auto& op : last_frame_ops) {
					Log::info("   ", op);
				}
				check_draw_order(true);
			}
			if (!logged_dead_clip && check_dead_clip_holders(false) > 0) {
				logged_dead_clip = true;
				Log::info("[clip] dead clip handle after frame", job.frame - 1, "- ops of that frame:");
				for (const auto& op : last_frame_ops) {
					Log::info("   ", op);
				}
				check_dead_clip_holders(true);
			}
			last_frame_ops.clear();
			for (int i = 0; i < job.ops_per_frame; i++) {
				std::string op = "?";
				try {
					op = run_random_op();
					last_frame_ops.push_back(op + " on " + std::to_string(last_target.id()));
					if (std::getenv("UI_TEST_VERBOSE")) {
						Log::info("[fuzz] frame", job.frame, "op", op, "on", last_target.id());
					}
				} catch (const std::exception& exception) {
					job.exceptions++;
					if (job.exceptions <= 10) {
						Log::info("[fuzz] frame", job.frame, "exception:", exception.what());
					}
				}
			}
			if (std::getenv("UI_TEST_VERBOSE")) {
				Registry& registry = Engine::get_registry();
				for (const auto& e : Engine::entities_touch_listener) {
					int type = registry.has<Engine::Description>(e.id())
								   ? (int)std::get<0>(registry.read<Engine::Description>(e.id())).value
								   : -1;
					Log::info(
						"[listener] frame", job.frame, "touch list has", e.id(), "gen", e.generation(), "type", type,
						"alive", Engine::entity_generator.validate_handle(e), "touch component",
						registry.has<Engine::Touching_Information>(e.id())
					);
				}
			}
			// same check right after the ops: process_frame sorts the lists before the next frame starts.
			if (!logged_stale_listener && check_stale_listeners(false) > 0) {
				logged_stale_listener = true;
				Log::info("[listener] stale entry right after the ops of frame", job.frame, ":");
				for (const auto& op : last_frame_ops) {
					Log::info("   ", op);
				}
				check_stale_listeners(true);
			}
			size_t issues = verify_tree(false);
			if (issues > 0) {
				job.tree_issue_frames++;
				if (!job.logged_tree) {
					job.logged_tree = true;
					Log::info("[fuzz] first tree issue at frame", job.frame, ":");
					verify_tree(true);
				}
			}
		}

		void stress_frame() {
			switch (job.frame % 3) {
				case 0:
					build_modal_scene();
					break;
				case 1:
					build_picker_scene();
					break;
				default:
					build_lab_scene();
					break;
			}
		}

		void finish_job() {
			switch (job.type) {
				case JOB::FUZZ: {
					Log::info(
						"[fuzz] done. seed", job.seed, "| frames", job.frame, "| exceptions", job.exceptions,
						"| frames with tree issues", job.tree_issue_frames
					);
					Log::info("[fuzz] press R to clear the scene and run the leak check");
					break;
				}
				case JOB::STRESS: {
					clear_scene();
					Engine::process_frame(0.f); // clean_up is queued until the next frame
					Log::info("[stress] done,", job.frame, "build/clear cycles");
					verify_tree(true);
					print_pool_diff(get_baseline(), count_pools(), "after stress");
					break;
				}
				default: {
					break;
				}
			}
			job = Job_State{};
		}
	} // namespace

	void start_fuzz(uint32_t seed, int frames, int ops_per_frame) {
		if (get_scene().root.is_invalid()) {
			build_lab_scene();
		}
		job = Job_State{};
		job.type = JOB::FUZZ;
		job.frames_left = frames;
		job.ops_per_frame = ops_per_frame;
		job.seed = seed;
		rng.seed(seed);
		Log::info(
			"[fuzz] start on scene", get_scene().name, "| seed", seed, "| frames", frames, "| ops/frame", ops_per_frame
		);
	}

	void start_stress(int cycles) {
		job = Job_State{};
		job.type = JOB::STRESS;
		job.frames_left = cycles;
		Log::info("[stress] start,", cycles, "build/clear cycles (one per frame)");
	}

	bool is_running_job() {
		return job.type != JOB::NONE;
	}

	void tick() {
		if (job.type == JOB::NONE)
			return;
		if (job.frames_left <= 0) {
			finish_job();
			return;
		}
		switch (job.type) {
			case JOB::FUZZ:
				fuzz_frame();
				break;
			case JOB::STRESS:
				stress_frame();
				break;
			default:
				break;
		}
		job.frame++;
		job.frames_left--;
	}

} // namespace Test
