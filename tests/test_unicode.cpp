/**
 * @Note: this file was written by Claude (Claude Code) on 2026-11-17.
 * Vietnamese / Unicode checks for labels: `./ui_test repro-unicode` (also in tests/run_all.sh).
 * Every check logs "[repro] PASS | unicode | <group> | ..." or "[repro] FAIL ..."; the suite counts a FAIL line
 * as a failed case. Groups:
 *   helper  - utf8_helper.h decode / encode / NFC (no GPU)
 *   atlas   - the default font (Be Vietnam Pro) has a rect for every English + Vietnamese code point,
 *             no duplicate rects, missing glyphs fall back without throwing
 *   label   - auto-fit label with Vietnamese text: one code point per glyph, NFD input == NFC input
 *   wrap    - CLIP / ELLIPSIS labels with Vietnamese text: every line fits, no character lost or broken
 *   cache   - a fresh bake and a load from the disk cache give the same font
 * The expected strings are written with \u escapes on purpose, so an editor that normalizes the file cannot
 * turn the decomposed (NFD) inputs into NFC and make the NFC checks pass by accident.
 */
#include <algorithm>
#include <cmath>
#include <engine/components.h>
#include <engine/engine.h>
#include <filesystem>
#include <log.h>
#include <parser/font_parser.h>
#include <string>
#include <test_ui.h>
#include <utf8_helper.h>
#include <utils.h>
#include <vulkan/draws/vk_draw_font_2D.h>
#include <vulkan/vk_draw.h>

namespace Test {

	namespace {
		const std::string VN_FONT = "res/fonts/BeVietnamPro-Regular.ttf";

		// "Tiếng Việt": 10 code points, 14 bytes in UTF-8.
		const std::u32string TIENG_VIET = U"Tiếng Việt";
		// the same text decomposed: e + circumflex + acute, e + dot below + circumflex.
		const std::u32string TIENG_VIET_NFD = U"Tiếng Việt";

		const std::u32string ENGLISH = U" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
									   U"abcdefghijklmnopqrstuvwxyz{|}~";
		// all 146 Vietnamese letters (lower + upper), escaped so the file encoding cannot change them.
		const std::u32string VIETNAMESE =
			U"aàáảãạăằắẳẵặâầấẩẫậ"
			U"eèéẻẽẹêềếểễệ"
			U"iìíỉĩị"
			U"oòóỏõọôồốổỗộơờớởỡợ"
			U"uùúủũụưừứửữự"
			U"yỳýỷỹỵđ"
			U"AÀÁẢÃẠĂẰẮẲẴẶÂẦẤẨẪẬ"
			U"EÈÉẺẼẸÊỀẾỂỄỆ"
			U"IÌÍỈĨỊ"
			U"OÒÓỎÕỌÔỒỐỔỖỘƠỜỚỞỠỢ"
			U"UÙÚỦŨỤƯỪỨỬỮỰ"
			U"YỲÝỶỸỴĐ";

		int unicode_failed = 0;

		void expect(const std::string& group, bool ok, const std::string& what) {
			if (!ok)
				unicode_failed++;
			Log::info(ok ? "[repro] PASS" : "[repro] FAIL", "unicode", group, what);
		}

		/**
		 * @Note: printable form for logs: ASCII as is, everything else as U+XXXX.
		 */
		std::string show(std::u32string_view text) {
			std::string out;
			for (char32_t c : text) {
				if (c >= 0x20 && c < 0x7F) {
					out += (char)c;
				} else {
					char buffer[16];
					snprintf(buffer, sizeof(buffer), "<U+%04X>", (unsigned)c);
					out += buffer;
				}
			}
			return out;
		}

		std::string vec_text(const glm::vec2& v) {
			return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
		}

		std::string full_font_path(const std::string& path) {
			return Parser::get_full_font_path(path);
		}

		glm::vec2 text_size(std::u32string_view text, int font_size = 20) {
			return Vulkan::Draw_Font_2D::get_renderer_text_size(text, full_font_path(VN_FONT), font_size, 0.f);
		}

		std::vector<std::u32string> split_lines(const std::u32string& text) {
			std::vector<std::u32string> lines(1);
			for (char32_t c : text) {
				if (c == U'\n')
					lines.emplace_back();
				else
					lines.back() += c;
			}
			return lines;
		}

		/**
		 * @Note: the code points of `text` without spaces / newlines, to compare what a wrap kept.
		 */
		std::u32string without_spaces(std::u32string_view text) {
			std::u32string out;
			for (char32_t c : text) {
				if (c != U' ' && c != U'\n')
					out += c;
			}
			return out;
		}

		// ------------------------------ helper ------------------------------

		void check_helper() {
			const std::string group = "helper";
			const std::string tieng_viet_utf8 = "Ti\xE1\xBA\xBFng Vi\xE1\xBB\x87t";
			expect(group, Helper::utf8_to_u32(tieng_viet_utf8) == TIENG_VIET, "decode \"Tieng Viet\" (14 bytes)");
			expect(group, Helper::utf8_length(tieng_viet_utf8) == 10, "utf8_length == 10 code points");
			expect(group, Helper::u32_to_utf8(TIENG_VIET) == tieng_viet_utf8, "encode back to the same 14 bytes");
			expect(group, Helper::utf8_to_u32("\xC3\xAA") == U"ê", "C3 AA = U+00EA (e circumflex)");
			expect(group, Helper::utf8_to_u32("\xF0\x9F\x98\x80") == U"\U0001F600", "4-byte sequence");
			const std::u32string bad = U"�";
			expect(group, Helper::utf8_to_u32("\xC3" "A") == bad + U"A", "cut 2-byte sequence keeps the next char");
			expect(group, Helper::utf8_to_u32("\xC1\x81") == bad + bad, "overlong 'A' rejected");
			expect(group, Helper::utf8_to_u32("\xED\xA0\x80") == bad + bad + bad, "surrogate rejected");
			expect(
				group, Helper::utf8_to_u32("\xF4\x90\x80\x80").find((char32_t)0x110000) == std::u32string::npos,
				"code point above U+10FFFF rejected"
			);
			expect(group, Helper::utf8_to_u32("ab\xE1\xBA") == U"ab" + bad, "sequence cut at the end");
			expect(group, Helper::utf8_to_u32("\xAA" "b") == bad + U"b", "stray continuation byte");

			expect(
				group, Helper::nfc_vietnamese(TIENG_VIET_NFD) == TIENG_VIET,
				"NFD \"Tieng Viet\" composes to NFC: " + show(Helper::nfc_vietnamese(TIENG_VIET_NFD))
			);
			expect(
				group, Helper::utf8_to_u32_nfc(Helper::u32_to_utf8(TIENG_VIET_NFD)) == TIENG_VIET,
				"utf8_to_u32_nfc on decomposed UTF-8"
			);
			expect(group, Helper::nfc_vietnamese(U"ợ") == U"ợ", "o + dot below + horn = U+1EE3");
			expect(group, Helper::nfc_vietnamese(U"ợ") == U"ợ", "o + horn + dot below = U+1EE3");
			expect(group, Helper::nfc_vietnamese(TIENG_VIET) == TIENG_VIET, "NFC input is unchanged");
			expect(group, Helper::nfc_vietnamese(VIETNAMESE) == VIETNAMESE, "all 146 letters unchanged by NFC");
			expect(group, Helper::to_u32string(42) == U"42", "to_u32string(42)");
		}

		// ------------------------------ atlas ------------------------------

		void check_atlas() {
			const std::string group = "atlas";
			const std::string path = full_font_path(VN_FONT);
			expect(group, !path.empty(), "font file found: " + VN_FONT);
			if (path.empty())
				return;
			const Vulkan::Font& font = Vulkan::view_font(Vulkan::load_font(path));
			std::u32string missing;
			std::u32string bad_index;
			for (char32_t c : ENGLISH + VIETNAMESE) {
				auto found = font.char_to_rect_indices.find(c);
				if (found == font.char_to_rect_indices.end())
					missing += c;
				else if (found->second >= font.rects.size())
					bad_index += c;
			}
			expect(group, VIETNAMESE.size() == 146, "test pattern has 146 Vietnamese letters");
			expect(group, missing.empty(), "every English + Vietnamese code point has a rect, missing: " + show(missing));
			expect(group, bad_index.empty(), "every mapped index is inside rects, bad: " + show(bad_index));
			expect(
				group, font.rects.size() == font.char_to_rect_indices.size(),
				"no duplicate bake: rects " + std::to_string(font.rects.size()) + " == map entries " +
					std::to_string(font.char_to_rect_indices.size())
			);
			// 'e' and its Vietnamese forms must use different rects (not all falling back to one glyph).
			int e_index = font.to_char_index(U'e');
			int e_hat_index = font.to_char_index(U'ế');
			expect(group, e_index != e_hat_index, "'e' and U+1EBF use different rects");

			bool threw = false;
			int fallback = -1;
			try {
				fallback = font.to_char_index(U'あ'); // hiragana a: not in this font
			} catch (...) {
				threw = true;
			}
			expect(group, !threw, "missing glyph (U+3042) does not throw");
			expect(
				group, !threw && fallback == font.to_char_index(U'?'),
				"missing glyph falls back to '?' (got index " + std::to_string(fallback) + ")"
			);
			expect(
				group, !threw && font.to_char_index(U'�') == font.to_char_index(U'?'),
				"U+FFFD (decode error) also draws as '?'"
			);
		}

		// ------------------------------ label (auto fit) ------------------------------

		Handle make_label(const std::string& utf8) {
			Handle label = Engine::create_label();
			Engine::set_label_font_path(label, VN_FONT);
			Engine::set_label_font_size(label, 20);
			Engine::set_label_string(label, utf8);
			Engine::add_child(Engine::get_root(), label);
			return label;
		}

		std::u32string vulkan_text_of(const Handle& label) {
			const auto& [handles] = Engine::get_registry().read<Engine::Renderer_Handles>(label.id());
			if (handles.value.empty())
				return U"<no renderer>";
			return Vulkan::modify_label(handles.value[0]).get_text();
		}

		void check_label() {
			const std::string group = "label";
			const std::string nfc_utf8 = Helper::u32_to_utf8(TIENG_VIET);
			const std::string nfd_utf8 = Helper::u32_to_utf8(TIENG_VIET_NFD);
			Handle nfc_label = make_label(nfc_utf8);
			Handle nfd_label = make_label(nfd_utf8);
			Engine::process_frame(0.f);

			expect(group, Engine::get_label_string(nfc_label) == nfc_utf8, "get_label_string returns the UTF-8 set");
			expect(group, Engine::get_label_string(nfd_label) == nfd_utf8, "get_label_string keeps the input as set (NFD)");
			std::u32string drawn = vulkan_text_of(nfc_label);
			expect(
				group, drawn == TIENG_VIET,
				"Vulkan label text is 10 code points (not 14 bytes): got " + std::to_string(drawn.size()) + " " + show(drawn)
			);
			std::u32string drawn_nfd = vulkan_text_of(nfd_label);
			expect(group, drawn_nfd == TIENG_VIET, "decomposed input is drawn as NFC: " + show(drawn_nfd));

			glm::vec2 size = Engine::get_content_size(nfc_label);
			glm::vec2 want = text_size(TIENG_VIET);
			expect(group, size.x > 0.f && size.y > 0.f, "content size > 0: " + vec_text(size));
			expect(
				group, std::abs(size.x - want.x) <= 0.5f && std::abs(size.y - want.y) <= 0.5f,
				"content size " + vec_text(size) + " == measured text " + vec_text(want)
			);
			glm::vec2 size_nfd = Engine::get_content_size(nfd_label);
			expect(
				group, std::abs(size_nfd.x - size.x) <= 0.01f,
				"NFD label is as wide as NFC: " + vec_text(size_nfd) + " vs " + vec_text(size)
			);

			// one glyph per code point: "ê" is measured like one char, not like two bytes.
			float e = text_size(U"e").x;
			float e_hat = text_size(U"ê").x;
			expect(
				group, e_hat > 0.f && e_hat < 1.5f * e,
				"width of U+00EA (" + std::to_string(e_hat) + ") is one glyph, close to 'e' (" + std::to_string(e) + ")"
			);
			Engine::clean_up(nfc_label);
			Engine::clean_up(nfd_label);
			Engine::process_frame(0.f);
		}

		// ------------------------------ wrap / ellipsis ------------------------------

		/**
		 * @Note: custom-size label in a narrow box; returns render_text after one frame.
		 */
		std::u32string wrap(
			const std::u32string& text, Engine::LABEL_RENDER_FIT_MODE fit_mode, const glm::vec2& box, Handle& out_label
		) {
			out_label = make_label(Helper::u32_to_utf8(text));
			Engine::set_label_render_fit_mode(out_label, fit_mode);
			Engine::set_content_size(out_label, box);
			Engine::process_frame(0.f);
			const auto& [info] = Engine::get_registry().read<Engine::Label_Renderer_Information>(out_label.id());
			return info.render_text;
		}

		void check_wrap() {
			const std::string group = "wrap";
			// "Tiếng Việt có dấu rất đẹp, người Việt Nam viết chữ Quốc ngữ"
			const std::u32string sentence = U"Tiếng Việt có dấu rất đẹp, người "
											U"Việt Nam viết chữ Quốc ngữ";
			const float box_width = 160.f;
			Handle clip_label;
			std::u32string render = wrap(sentence, Engine::LABEL_RENDER_FIT_CLIP, {box_width, 400.f}, clip_label);
			auto lines = split_lines(render);
			expect(group, lines.size() > 1, "long sentence wraps into " + std::to_string(lines.size()) + " lines");
			for (size_t i = 0; i < lines.size(); i++) {
				float width = text_size(lines[i]).x;
				expect(
					group, width <= box_width + 0.5f,
					"line " + std::to_string(i) + " width " + std::to_string(width) + " <= " + std::to_string(box_width) +
						": " + show(lines[i])
				);
			}
			expect(
				group, without_spaces(render) == without_spaces(sentence),
				"CLIP wrap keeps every character (only spaces become newlines)"
			);
			expect(group, render.find(U'�') == std::u32string::npos, "no U+FFFD in render_text");
			expect(group, Helper::nfc_vietnamese(render) == render, "render_text stays NFC");

			// one long word wider than the box is cut by characters, never inside a code point.
			Handle word_label;
			const std::u32string long_word = U"Nghỉnghỉnghỉnghỉnghỉnghỉ";
			std::u32string cut = wrap(long_word, Engine::LABEL_RENDER_FIT_CLIP, {90.f, 400.f}, word_label);
			auto cut_lines = split_lines(cut);
			bool fits = true;
			for (const auto& line : cut_lines)
				fits = fits && text_size(line).x <= 90.f + 0.5f;
			expect(group, cut_lines.size() > 1 && fits, "long word is cut into lines that fit: " + show(cut));
			expect(group, without_spaces(cut) == long_word, "cut keeps every character of the word");

			// ellipsis: two lines tall, the last line ends with "..." and still fits.
			Handle ellipsis_label;
			float line_height = text_size(U" ").y;
			std::u32string ellipsis =
				wrap(sentence, Engine::LABEL_RENDER_FIT_ELLIPSIS, {box_width, line_height * 2.f + 1.f}, ellipsis_label);
			auto ellipsis_lines = split_lines(ellipsis);
			expect(group, ellipsis_lines.size() <= 2, "ELLIPSIS keeps at most 2 lines: " + std::to_string(ellipsis_lines.size()));
			const std::u32string& last = ellipsis_lines.back();
			expect(
				group, last.size() >= 3 && last.compare(last.size() - 3, 3, U"...") == 0, "last line ends with ...: " + show(last)
			);
			expect(group, text_size(last).x <= box_width + 0.5f, "last line with ... fits the width");

			Engine::clean_up(clip_label);
			Engine::clean_up(word_label);
			Engine::clean_up(ellipsis_label);
			Engine::process_frame(0.f);
		}

		// ------------------------------ cache ------------------------------

		void check_cache() {
			const std::string group = "cache";
			namespace fs = std::filesystem;
			const std::string source = full_font_path(VN_FONT);
			if (source.empty()) {
				expect(group, false, "font file found: " + VN_FONT);
				return;
			}
			// a copy under its own name, so this test owns its cache files and always starts with a fresh bake.
			fs::path copy = fs::path(source).parent_path() / "_unicode_cache_test.ttf";
			auto remove_files = [&copy]() {
				std::error_code error;
				for (const auto& entry : fs::directory_iterator(copy.parent_path(), error)) {
					if (entry.path().stem() == copy.stem())
						fs::remove(entry.path(), error);
				}
			};
			remove_files();
			std::error_code error;
			fs::copy_file(source, copy, fs::copy_options::overwrite_existing, error);
			expect(group, !error, "copy font for the cache test");
			if (error)
				return;
			try {
				Parser::Font fresh = Parser::parse_sdf_font(copy.string());
				Parser::Font cached = Parser::parse_sdf_font(copy.string());
				expect(group, fresh.char_to_rect_indices.size() > 0, "fresh bake has a code point map");
				expect(
					group, fresh.char_to_rect_indices == cached.char_to_rect_indices,
					"cached map == fresh map (" + std::to_string(cached.char_to_rect_indices.size()) + " vs " +
						std::to_string(fresh.char_to_rect_indices.size()) + ")"
				);
				expect(
					group, fresh.rects.size() == cached.rects.size(),
					"cached rects == fresh rects (" + std::to_string(cached.rects.size()) + ")"
				);
				expect(group, fresh.atlas == cached.atlas, "cached atlas bytes == fresh atlas bytes");
				expect(group, fresh.supported_language == cached.supported_language, "cached languages == fresh");
				expect(
					group, fresh.line_height == cached.line_height && fresh.size_atlas == cached.size_atlas,
					"cached metrics == fresh metrics"
				);
			} catch (const std::exception& exception) {
				expect(group, false, std::string("parse threw: ") + exception.what());
			}
			remove_files();
		}
	} // namespace

	// [Claude BEGIN 2026-11-17] scene for `./ui_test unicode-view` (screenshot to check the Vietnamese rendering by eye).
	namespace {
		const glm::vec3 VIEW_INK{0.13f, 0.15f, 0.19f};
		const glm::vec3 VIEW_NOTE{0.45f, 0.48f, 0.55f};

		Handle view_label(const Handle& parent, const std::string& text, int size, const glm::vec2& position) {
			Handle label = Test::make_label(parent, text, size, VIEW_INK, position, {0.f, 0.5f});
			Engine::set_label_font_path(label, VN_FONT);
			return label;
		}

		/**
		 * @Note: white box with a custom-size label filling it (top-left aligned, padding 8) and a caption above.
		 */
		void view_box(
			const Handle& page, const std::string& caption, const std::string& text, const glm::vec2& position,
			const glm::vec2& size, Engine::LABEL_RENDER_FIT_MODE fit_mode, int font_size
		) {
			Engine::set_color(view_label(page, caption, 14, {position.x, position.y + size.y + 14.f}), VIEW_NOTE);
			Handle box = make_layout(page, position, size, {1.f, 1.f, 1.f}, 6.f);
			Handle label = view_label(box, text, font_size, {0.f, 0.f});
			Engine::set_anchor(label, {0.f, 0.f});
			Engine::set_label_render_fit_mode(label, fit_mode);
			Engine::set_label_horizontal_align(label, Engine::LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_LEFT);
			Engine::set_label_vertical_align(label, Engine::LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_TOP);
			Engine::set_label_padding_horizontal(label, 8.f);
			Engine::set_label_padding_vertical(label, 8.f);
			Engine::set_content_size(label, size);
		}
	} // namespace

	void build_unicode_scene() {
		Handle page = make_page("unicode view", {0.9f, 0.91f, 0.93f});
		glm::vec2 window = Engine::get_window_size();
		float y = window.y - 26.f;
		Engine::set_color(view_label(page, "UI test: Vietnamese rendering (Be Vietnam Pro)", 16, {20.f, y}), VIEW_NOTE);

		// sizes: small sizes show whether the marks stay readable, big sizes show their placement.
		y -= 36.f;
		view_label(page, "16 px: Tiếng Việt có dấu, đẹp và rõ ràng", 16, {20.f, y});
		y -= 36.f;
		view_label(page, "24 px: Tiếng Việt có dấu, đẹp và rõ ràng", 24, {20.f, y});
		y -= 50.f;
		view_label(page, "36 px: Những con cò trắng bay lượn", 36, {20.f, y});
		y -= 64.f;
		view_label(page, "48 px: Ắ Ằ Ẳ Ẵ Ặ Ấ Ầ Ẩ Ẫ Ậ Ế Ề Ể Ễ Ệ", 48, {20.f, y});

		// every letter once (stacked marks on capitals are the tallest glyphs: check nothing is cut at the top).
		y -= 52.f;
		view_label(page, "aàáảãạ ăằắẳẵặ âầấẩẫậ eèéẻẽẹ êềếểễệ iìíỉĩị oòóỏõọ ôồốổỗộ ơờớởỡợ uùúủũụ ưừứửữự yỳýỷỹỵ đ", 20, {20.f, y});
		y -= 32.f;
		view_label(page, "AÀÁẢÃẠ ĂẰẮẲẴẶ ÂẦẤẨẪẬ EÈÉẺẼẸ ÊỀẾỂỄỆ IÌÍỈĨỊ OÒÓỎÕỌ ÔỒỐỔỖỘ ƠỜỚỞỠỢ UÙÚỦŨỤ ƯỪỨỬỮỰ YỲÝỶỸỴ Đ", 20, {20.f, y});

		// decomposed input must look exactly like the line above it; stroke must follow the marks.
		y -= 40.f;
		view_label(page, "NFC: Tiếng Việt", 24, {20.f, y});
		view_label(page, Helper::u32_to_utf8(U"NFD: Tiếng Việt"), 24, {260.f, y});
		Handle stroke = view_label(page, "Viền chữ: Quốc ngữ", 24, {520.f, y});
		Engine::set_color(stroke, {1.f, 1.f, 1.f});
		Engine::set_label_stroke_color(stroke, {0.85f, 0.2f, 0.15f});
		Engine::set_label_stroke_size(stroke, 2.f);
		// a font without Vietnamese: missing glyphs must draw as '?', and "あ" too.
		Test::make_label(page, "Anonymous Pro: Tiếng Việt / あ", 20, VIEW_INK, {820.f, y}, {0.f, 0.5f});

		// wrap / ellipsis boxes.
		const std::string paragraph =
			"Tiếng Việt là ngôn ngữ chính thức của Việt Nam. Chữ Quốc ngữ dùng bảng chữ cái Latinh "
			"với các dấu thanh: sắc, huyền, hỏi, ngã, nặng.";
		view_box(page, "CLIP, wrap (360 x 150)", paragraph, {20.f, 20.f}, {360.f, 150.f}, Engine::LABEL_RENDER_FIT_CLIP, 20);
		view_box(
			page, "ELLIPSIS, 2 lines (360 x 64)", paragraph, {410.f, 106.f}, {360.f, 64.f},
			Engine::LABEL_RENDER_FIT_ELLIPSIS, 20
		);
		view_box(
			page, "CLIP, long word cut (150 x 150)", "Nghỉnghỉnghỉnghỉnghỉnghỉ", {800.f, 20.f}, {150.f, 150.f},
			Engine::LABEL_RENDER_FIT_CLIP, 20
		);
	}
	// [Claude END]

	void repro_unicode_checks() {
		Log::info("[repro] Vietnamese / Unicode label checks (helper, atlas, label, wrap, cache)");
		unicode_failed = 0;
		auto run = [](const std::string& name, void (*check)()) {
			try {
				check();
			} catch (const std::exception& exception) {
				expect(name, false, std::string("threw: ") + exception.what());
			}
		};
		run("helper", check_helper);
		run("atlas", check_atlas);
		run("label", check_label);
		run("wrap", check_wrap);
		run("cache", check_cache);
		print_pool_diff(get_baseline(), count_pools(), "after unicode checks");
		if (unicode_failed == 0) {
			Log::info("[repro] survived unicode checks, all pass");
		} else {
			Log::info("[repro] unicode checks:", unicode_failed, "failed");
		}
	}

} // namespace Test
