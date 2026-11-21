/**
 * @Note: this file was written by Claude (Claude Code) on 2026-11-19.
 * Engine::find_label_character_index checks: `./ui_test repro-char-detect` (also in tests/run_all.sh).
 * Every check logs "[repro] PASS | char-detect | <group> | ..." or "[repro] FAIL ..."; the suite counts a FAIL
 * line as a failed case. There is no public getter for an individual char's rect, so these checks don't assert
 * exact pixel positions - instead they rebuild the forward world transform (position + rotate(rotation, local *
 * scale), the inverse of Engine::to_my_position_space) from the public getters and sweep local points across
 * the label's own content box, checking properties that must hold regardless of font metrics:
 *   basic     - single line, identity transform: left-to-right sweep is monotonic non-decreasing, first/last
 *               char are both reachable, clearly outside the box is -1
 *   space      - a space character is a selectable index too, not skipped
 *   vietnamese - "Tieng Viet" with 2 precomposed letters (10 NFC code points, 14 UTF-8 bytes):
 *               Engine::find_label_character_index now returns a UTF-8 BYTE offset into
 *               Engine::get_label_string, not a codepoint index - this group computes its own ground
 *               truth with an independent Helper::utf8_to_u32_nfc_mapped call on the same raw bytes and
 *               checks every returned index is exactly one of the real byte-start offsets (never a byte
 *               mid-character, never a codepoint index), monotonic, and that all 10 are reachable
 *   multiline - explicit '\n': both lines are reachable, the '\n' itself is never a hit
 *   transform - same monotonic/coverage properties still hold once the label has its own rotation + non-uniform
 *               scale + a centred anchor (this is the exact shape of the draggable/scaled label in main.cpp that
 *               motivated this feature)
 *   wrap      - fixed-width word-wrap: first and last character of the original text are both still reachable
 *               after wrapping, and every returned index stays in range
 *   truncate  - fixed, very small box with CLIP: robustness only (no out-of-range index, no crash)
 *   invalid   - no Label_Renderer_Information, and a cleaned-up handle
 */
#include <engine/engine.h>
#include <log.h>
#include <math_custom.h>
#include <string>
#include <test_ui.h>
#include <utf8_helper.h>
#include <vector>

namespace Test {

	namespace {
		int char_detect_failed = 0;

		void expect(const std::string& group, bool ok, const std::string& what) {
			if (!ok)
				char_detect_failed++;
			Log::info(ok ? "[repro] PASS" : "[repro] FAIL", "char-detect", group, what);
		}

		std::string join_ints(const std::vector<int>& values) {
			std::string out = "[";
			for (size_t i = 0; i < values.size(); i++) {
				out += std::to_string(values[i]);
				if (i + 1 < values.size())
					out += ", ";
			}
			return out + "]";
		}

		// forward transform: local label-content-space point -> world point. Mirrors the position line of
		// World_Transform_2D::transform() / is the inverse of Engine::to_my_position_space. Only correct when
		// `e`'s own ancestors are all identity transforms, which holds for every label in this file (direct
		// child of root, and root's own transform is left at its defaults).
		glm::vec2 local_to_world(const Handle& e, const glm::vec2& local) {
			glm::vec2 position = Engine::get_world_position(e);
			glm::vec2 scale = Engine::get_scale(e);
			float rotation = Engine::get_rotation(e);
			return position + Math::rotate(rotation, local * scale);
		}

		Handle make_test_label(
			const std::string& text, const glm::vec2& content_size, Engine::LABEL_RENDER_FIT_MODE fit,
			const glm::vec2& anchor = {0.f, 0.f}, const glm::vec2& position = {400.f, 400.f}, float rotation = 0.f,
			const glm::vec2& scale = {1.f, 1.f}
		) {
			Handle label = Engine::create_label();
			Engine::set_label_font_path(label, "");
			Engine::set_label_font_size(label, 16);
			Engine::set_label_horizontal_align(label, Engine::LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_LEFT);
			Engine::set_label_vertical_align(label, Engine::LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_TOP);
			Engine::set_anchor(label, anchor);
			Engine::set_position(label, position);
			Engine::set_rotation(label, rotation);
			Engine::set_scale(label, scale);
			Engine::set_label_render_fit_mode(label, fit);
			Engine::set_content_size(label, content_size);
			Engine::set_label_string(label, text);
			Engine::add_child(Engine::get_root(), label);
			return label;
		}

		// margin padding around the content box so the sweep definitely covers "clearly outside" samples too.
		const float MARGIN = 30.f;
		const float STEP = 1.f;

		// sweep local x (at a fixed local y) from (anchor-relative) left edge - margin to right edge + margin,
		// returning every char index find_label_character_index reports, in sweep order (duplicates kept).
		std::vector<int> sweep_x(
			const Handle& e, const glm::vec2& content_size, const glm::vec2& anchor, float local_y
		) {
			std::vector<int> indices;
			float left = -anchor.x * content_size.x - MARGIN;
			float right = (1.f - anchor.x) * content_size.x + MARGIN;
			for (float x = left; x <= right; x += STEP) {
				glm::vec2 world = local_to_world(e, {x, local_y});
				indices.push_back(Engine::find_label_character_index(e, world));
			}
			return indices;
		}

		// ------------------------------ basic ------------------------------

		void check_basic() {
			const std::string group = "basic";
			std::string text = "ABCDE";
			glm::vec2 anchor{0.f, 0.f};
			Handle label =
				make_test_label(text, {0.f, 0.f}, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO, anchor);
			glm::vec2 content_size = Engine::get_content_size(label);
			expect(group, content_size.x > 0.f && content_size.y > 0.f, "AUTO label resolves a non-zero content size");

			float mid_y = content_size.y * 0.5f;
			std::vector<int> indices = sweep_x(label, content_size, anchor, mid_y);

			bool saw_first = false, saw_last = false, monotonic = true, out_of_range = false;
			int previous = -1;
			for (int index : indices) {
				if (index < -1 || index >= (int)text.size())
					out_of_range = true;
				if (index == 0)
					saw_first = true;
				if (index == (int)text.size() - 1)
					saw_last = true;
				if (index != -1) {
					if (previous != -1 && index < previous)
						monotonic = false;
					previous = index;
				}
			}
			expect(
				group, !out_of_range, "every returned index is -1 or inside [0, text.size()): " + join_ints(indices)
			);
			expect(group, saw_first, "first character (index 0) is reachable somewhere in the left-to-right sweep");
			expect(
				group, saw_last,
				"last character (index " + std::to_string((int)text.size() - 1) +
					") is reachable somewhere in the left-to-right sweep"
			);
			expect(
				group, monotonic,
				"left-to-right sweep never jumps to an earlier character once it has moved past it: " +
					join_ints(indices)
			);

			glm::vec2 far_left = local_to_world(label, {-anchor.x * content_size.x - MARGIN, mid_y});
			glm::vec2 far_right = local_to_world(label, {(1.f - anchor.x) * content_size.x + MARGIN, mid_y});
			expect(
				group, Engine::find_label_character_index(label, far_left) == -1,
				"a point well to the left of the text returns -1"
			);
			expect(
				group, Engine::find_label_character_index(label, far_right) == -1,
				"a point well to the right of the text returns -1"
			);

			Engine::clean_up(label);
		}

		// ------------------------------ space ------------------------------

		void check_space() {
			const std::string group = "space";
			std::string text = "AB CD";
			glm::vec2 anchor{0.f, 0.f};
			Handle label =
				make_test_label(text, {0.f, 0.f}, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO, anchor);
			glm::vec2 content_size = Engine::get_content_size(label);
			float mid_y = content_size.y * 0.5f;
			std::vector<int> indices = sweep_x(label, content_size, anchor, mid_y);
			bool saw_space = false;
			for (int index : indices) {
				if (index == 2) // 'A'=0, 'B'=1, ' '=2
					saw_space = true;
			}
			expect(
				group, saw_space,
				"the space character (index 2 in \"AB CD\") is itself a selectable slot, not skipped: " +
					join_ints(indices)
			);
			Engine::clean_up(label);
		}

		// ------------------------------ vietnamese ------------------------------

		void check_vietnamese() {
			const std::string group = "vietnamese";
			// "Tieng Viet" with the real NFC letters e-with-circumflex-and-acute (U+1EBF) and
			// e-with-circumflex-and-dot-below (U+1EC7) - written as explicit \u escapes so the exact code
			// points survive regardless of source/tool-call encoding, instead of as literal UTF-8 bytes in
			// this file. 10 NFC code points, 14 UTF-8 bytes - picked so a byte-vs-codepoint index mix-up
			// shows up directly (14 != 10), which a plain-ASCII string can never expose.
			std::u32string u32_literal = U"Tiếng Việt";
			std::string raw_utf8 = Helper::u32_to_utf8(u32_literal);

			std::vector<size_t> expected_byte_start, expected_byte_len;
			std::u32string expected_u32 = Helper::utf8_to_u32_nfc_mapped(raw_utf8, expected_byte_start, expected_byte_len);
			expect(
				group, expected_u32 == u32_literal && raw_utf8.size() == 14 && expected_u32.size() == 10,
				"test setup sanity: " + std::to_string(raw_utf8.size()) + " UTF-8 bytes, " +
					std::to_string(expected_u32.size()) + " NFC code points (want 14 / 10)"
			);

			glm::vec2 anchor{0.f, 0.f};
			Handle label = Engine::create_label();
			Engine::set_label_font_path(label, "");
			Engine::set_label_font_size(label, 16);
			Engine::set_label_horizontal_align(label, Engine::LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_LEFT);
			Engine::set_label_vertical_align(label, Engine::LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_TOP);
			Engine::set_anchor(label, anchor);
			Engine::set_position(label, {400.f, 400.f});
			Engine::set_label_render_fit_mode(label, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO);
			Engine::set_content_size(label, {0.f, 0.f});
			Engine::set_label_string(label, raw_utf8);
			Engine::add_child(Engine::get_root(), label);

			expect(
				group, Engine::get_label_string(label) == raw_utf8,
				"get_label_string echoes back the exact raw UTF-8 bytes that were set"
			);
			std::u32string stored = Engine::get_label_u32_string(label);
			expect(
				group, stored == expected_u32,
				"get_label_u32_string returns the NFC-composed text (" + std::to_string(stored.size()) +
					" code points), matching an independent utf8_to_u32_nfc_mapped call on the same raw bytes"
			);

			glm::vec2 content_size = Engine::get_content_size(label);
			float mid_y = content_size.y * 0.5f;
			std::vector<int> indices = sweep_x(label, content_size, anchor, mid_y);

			std::vector<int> expected_starts_as_int(expected_byte_start.begin(), expected_byte_start.end());
			bool bad_index = false;
			for (int index : indices) {
				if (index == -1)
					continue;
				bool is_valid_start = false;
				for (size_t start : expected_byte_start) {
					if ((size_t)index == start) {
						is_valid_start = true;
						break;
					}
				}
				if (!is_valid_start)
					bad_index = true;
			}
			expect(
				group, !bad_index,
				"every returned index is exactly one of the expected UTF-8 byte-start offsets " +
					join_ints(expected_starts_as_int) + ": got " + join_ints(indices)
			);

			bool monotonic = true;
			int previous = -1;
			for (int index : indices) {
				if (index != -1) {
					if (previous != -1 && index < previous)
						monotonic = false;
					previous = index;
				}
			}
			expect(group, monotonic, "left-to-right sweep is monotonic in byte-offset space: " + join_ints(indices));

			std::vector<bool> seen_start(expected_byte_start.size(), false);
			for (int index : indices) {
				if (index == -1)
					continue;
				for (size_t k = 0; k < expected_byte_start.size(); k++) {
					if ((size_t)index == expected_byte_start[k])
						seen_start[k] = true;
				}
			}
			bool full_coverage = true;
			std::string missing;
			for (size_t k = 0; k < seen_start.size(); k++) {
				if (!seen_start[k]) {
					full_coverage = false;
					missing += std::to_string(expected_byte_start[k]) + " ";
				}
			}
			expect(
				group, full_coverage,
				"every one of the " + std::to_string(expected_byte_start.size()) +
					" composed letters' byte-start offsets is reachable: missing [" + missing + "]"
			);

			Engine::clean_up(label);
		}

		// ------------------------------ multiline ------------------------------

		void check_multiline() {
			const std::string group = "multiline";
			std::string text = "AB\nCD"; // index 2 is '\n', unselectable by construction.
			glm::vec2 anchor{0.f, 0.f};
			Handle label =
				make_test_label(text, {0.f, 0.f}, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO, anchor);
			glm::vec2 content_size = Engine::get_content_size(label);
			expect(group, content_size.y > 0.f, "two-line AUTO label resolves a non-zero height");

			std::vector<bool> seen(text.size(), false);
			bool any_newline_hit = false;
			float left = -MARGIN;
			float right = content_size.x + MARGIN;
			for (float y = -MARGIN; y <= content_size.y + MARGIN; y += STEP * 2.f) {
				for (float x = left; x <= right; x += STEP * 2.f) {
					glm::vec2 world = local_to_world(label, {x, y});
					int index = Engine::find_label_character_index(label, world);
					if (index == 2)
						any_newline_hit = true;
					if (index >= 0 && index < (int)seen.size())
						seen[index] = true;
				}
			}
			expect(
				group, seen[0] && seen[1],
				"both characters of the first line ('A','B') are reachable somewhere in the grid"
			);
			expect(
				group, seen[3] && seen[4],
				"both characters of the second line ('C','D') are reachable somewhere in the grid"
			);
			expect(group, !any_newline_hit, "the '\\n' itself (index 2) is never returned as a hit");

			Engine::clean_up(label);
		}

		// ------------------------------ transform ------------------------------

		void check_transform() {
			const std::string group = "transform";
			std::string text = "ABCDE";
			glm::vec2 anchor{0.5f, 0.5f};
			Handle label = make_test_label(
				text, {0.f, 0.f}, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO, anchor, {300.f, 150.f}, 25.f,
				{1.4f, 0.7f}
			);
			glm::vec2 content_size = Engine::get_content_size(label);
			float mid_y = 0.f; // anchor is centred, so local (0,0) is the middle of the text block already.
			std::vector<int> indices = sweep_x(label, content_size, anchor, mid_y);

			bool saw_first = false, saw_last = false, monotonic = true, out_of_range = false;
			int previous = -1;
			for (int index : indices) {
				if (index < -1 || index >= (int)text.size())
					out_of_range = true;
				if (index == 0)
					saw_first = true;
				if (index == (int)text.size() - 1)
					saw_last = true;
				if (index != -1) {
					if (previous != -1 && index < previous)
						monotonic = false;
					previous = index;
				}
			}
			expect(
				group, !out_of_range,
				"rotated (25 deg) + non-uniformly scaled (1.4, 0.7) + centred-anchor label: every index still in "
				"range: " +
					join_ints(indices)
			);
			expect(group, saw_first, "first character still reachable under rotation + scale + centred anchor");
			expect(group, saw_last, "last character still reachable under rotation + scale + centred anchor");
			expect(
				group, monotonic,
				"sweep (in the label's own local space) still comes out monotonic once converted through the "
				"rotated/scaled forward transform: " +
					join_ints(indices)
			);

			Engine::clean_up(label);
		}

		// ------------------------------ wrap ------------------------------

		void check_wrap() {
			const std::string group = "wrap";
			std::string text = "Hello World"; // 11 chars, space at index 5; width below forces wrapping.
			glm::vec2 anchor{0.f, 0.f};
			Handle label = make_test_label(
				text, {40.f, 10.f}, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_WIDTH_SHOW_ALL, anchor
			);
			glm::vec2 content_size = Engine::get_content_size(label);
			expect(
				group, content_size.x <= 40.f + 1.f,
				"WIDTH_SHOW_ALL keeps the fixed width: got " + std::to_string(content_size.x)
			);
			expect(
				group, content_size.y > 0.f,
				"WIDTH_SHOW_ALL grows a non-zero height once \"Hello World\" wraps inside a 40px-wide box"
			);

			std::vector<bool> seen(text.size(), false);
			bool out_of_range = false;
			for (float y = -MARGIN; y <= content_size.y + MARGIN; y += STEP * 2.f) {
				for (float x = -MARGIN; x <= content_size.x + MARGIN; x += STEP * 2.f) {
					int index = Engine::find_label_character_index(label, local_to_world(label, {x, y}));
					if (index < -1 || index >= (int)text.size())
						out_of_range = true;
					if (index >= 0)
						seen[index] = true;
				}
			}
			expect(group, !out_of_range, "every index stays in [0, text.size()) across a wrapped, multi-line box");
			expect(group, seen[0], "first character ('H') is reachable somewhere after wrapping");
			expect(group, seen[text.size() - 1], "last character ('d') is reachable somewhere after wrapping");

			Engine::clean_up(label);
		}

		// ------------------------------ truncate ------------------------------

		void check_truncate() {
			const std::string group = "truncate";
			std::string text = "This text is way too long to fit a tiny box";
			glm::vec2 anchor{0.f, 0.f};
			Handle label =
				make_test_label(text, {50.f, 20.f}, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_CLIP, anchor);
			glm::vec2 content_size = Engine::get_content_size(label);
			bool out_of_range = false;
			bool threw = false;
			try {
				for (float y = 0.f; y <= content_size.y; y += STEP * 3.f) {
					for (float x = 0.f; x <= content_size.x; x += STEP * 3.f) {
						int index = Engine::find_label_character_index(label, local_to_world(label, {x, y}));
						if (index < -1 || index >= (int)text.size())
							out_of_range = true;
					}
				}
			} catch (const std::exception&) {
				threw = true;
			}
			expect(group, !threw, "clicking inside a CLIP-truncated label does not throw");
			expect(group, !out_of_range, "every index inside a CLIP-truncated label stays in [0, text.size())");

			Engine::clean_up(label);
		}

		// ------------------------------ invalid ------------------------------

		void check_invalid() {
			const std::string group = "invalid";
			Handle node = Engine::create_node();
			Engine::add_child(Engine::get_root(), node);
			expect(
				group, Engine::find_label_character_index(node, {0.f, 0.f}) == -1,
				"a node with no Label_Renderer_Information returns -1 instead of touching label state"
			);
			Engine::clean_up(node);
			Engine::process_frame(0.f); // clean_up is queued until the next frame.

			bool threw = false;
			try {
				Engine::find_label_character_index(node, {0.f, 0.f});
			} catch (const std::exception&) {
				threw = true;
			}
			expect(
				group, threw,
				"a cleaned-up handle throws (assert_handle's contract, same as every other Engine:: entity function) "
				"rather than silently reading stale/recycled state"
			);
		}
	} // namespace

	void repro_char_detect_checks() {
		Log::info(
			"[repro] label char-index-at-point checks (basic, space, vietnamese, multiline, transform, wrap, "
			"truncate, invalid)"
		);
		char_detect_failed = 0;
		auto run = [](const std::string& name, void (*check)()) {
			try {
				check();
			} catch (const std::exception& exception) {
				expect(name, false, std::string("threw: ") + exception.what());
			}
		};
		run("basic", check_basic);
		run("space", check_space);
		run("vietnamese", check_vietnamese);
		run("multiline", check_multiline);
		run("transform", check_transform);
		run("wrap", check_wrap);
		run("truncate", check_truncate);
		run("invalid", check_invalid);
		Engine::process_frame(0.f);
		print_pool_diff(get_baseline(), count_pools(), "after char-detect checks");
		if (char_detect_failed == 0) {
			Log::info("[repro] survived char-detect checks, all pass");
		} else {
			Log::info("[repro] char-detect checks:", char_detect_failed, "failed");
		}
	}

} // namespace Test
