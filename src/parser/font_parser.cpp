#include <parser/font_parser.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <core.h>
#include <log.h>
#include <profiler.h>
#include <sparse_set.h>
#include <stb_truetype.h>

namespace Parser {

	struct Glyph_SDF {
		int width = 0.f;
		int height = 0.f;
		int x_off = 0.f;
		int y_off = 0.f;
		uint8_t* bytes = nullptr;
	};

	const int MAX_PIXEL_HEIGHT = 64;

	const int START_CHAR = 32;

	const int NUM_CHAR = 95;

	const int MAX_FONT_SIZE = 64;

	const int MIN_FONT_SIZE = 6;

	const int ATLAS_PADDING = 2;

	const int GLYPH_PADDING = 16;

	const uint8_t GLYPH_ONEDGE = 128;

	const int MIN_ATLAS_SIZE = 512;

	const int MAX_ATLAS_SIZE = 2048;

	Font parse_font(std::string file_path) {
		Profiler::start_scope(SCOPE_LOAD_FONT_V1);
		Font font{};
		font.bytes = Files::read_file(file_path);
		stbtt_fontinfo font_info{};
		stbtt_InitFont(&font_info, (const unsigned char*)font.bytes.data(), 0);
		/**
		 * Parse font into texture atlas with biggest size.
		 */
		int base_atlas_size = 512;
		int max_atlas_size = 2048;
		int result = -1;
		stbtt_packedchar packed_chars[NUM_CHAR] = {};
		while (result <= 0 && base_atlas_size <= max_atlas_size) {
			font.atlas.resize(base_atlas_size * base_atlas_size);
			stbtt_pack_context pc;
			stbtt_PackBegin(&pc, font.atlas.data(), base_atlas_size, base_atlas_size, 0, ATLAS_PADDING, nullptr);
			result = stbtt_PackFontRange(
				&pc, (const unsigned char*)font.bytes.data(), font_info.fontstart, MAX_PIXEL_HEIGHT, START_CHAR,
				NUM_CHAR, packed_chars
			);
			stbtt_PackEnd(&pc);
			if (result <= 0) {
				base_atlas_size *= 2;
			}
		}
		if (result == 0) {
			throw std::runtime_error("Fail to baked char in font:" + file_path + "!");
		}
		font.size_atlas = base_atlas_size;
		font.max_char_height = MAX_PIXEL_HEIGHT;
		font.min_font_size = MIN_FONT_SIZE;
		font.max_font_size = MAX_FONT_SIZE;
		int ascent = 0, descent = 0, line_gap = 0;
		stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
		float scale = stbtt_ScaleForPixelHeight(&font_info, MAX_PIXEL_HEIGHT);
		font.line_height = (ascent - descent + line_gap) * scale;
		/**
		 * Make unordered_map map character to rect draw.
		 */
		font.start_char = (char)START_CHAR;
		for (int i = 0; i < NUM_CHAR; i++) {
			stbtt_packedchar baked = packed_chars[i];
			Char_Rect rect{};
			rect.tex_coord_x = baked.x0 / (float)base_atlas_size;
			rect.tex_coord_y = 1.f - baked.y1 / (float)base_atlas_size;
			rect.tex_coord_width = (baked.x1 - baked.x0) / (float)base_atlas_size;
			rect.tex_coord_height = (baked.y1 - baked.y0) / (float)base_atlas_size;
			rect.offset_x = baked.xoff;
			rect.offset_y = -baked.yoff - (baked.y1 - baked.y0);
			rect.bounding_width = baked.xadvance;
			font.rects.push_back(rect);
		}
		Profiler::end_scope(SCOPE_LOAD_FONT_V1);
		return font;
	}

	Font parse_font_2(const std::string& file_path) {
		Profiler::start_scope(SCOPE_LOAD_FONT);
		Font font{};
		font.bytes = Files::read_file(file_path);
		stbtt_fontinfo font_info{};
		stbtt_InitFont(&font_info, (const unsigned char*)font.bytes.data(), 0);
		float scale = stbtt_ScaleForPixelHeight(&font_info, MAX_PIXEL_HEIGHT);
		std::vector<int> glyph_indices(NUM_CHAR, -1);
		for (int i = 0; i < NUM_CHAR; i++) {
			glyph_indices[i] = stbtt_FindGlyphIndex(&font_info, (char)(START_CHAR + i));
		}
		Profiler::start_scope(SCOPE_PARSE_GLYPH);
		int number_thread = Core::global_thread_pool->get_thread_ids().size();
		int number_glyph_in_thread = std::ceil((float)NUM_CHAR / number_thread);
		std::vector<std::future<void>> results;
		std::vector<Glyph_SDF> glyph_sdfs(NUM_CHAR);
		for (int i = 0; i < number_thread; i++) {
			int start = i * number_glyph_in_thread;
			int end = start + number_glyph_in_thread;
			auto result =
				Core::global_thread_pool->enqueue([&glyph_sdfs, &glyph_indices, &font_info, start, end, scale]() {
					for (int i = start; i < end && i < NUM_CHAR; i++) {
						int glyph_index = glyph_indices[i];
						auto& glyph = glyph_sdfs[i];
						glyph.bytes = stbtt_GetGlyphSDF(
							&font_info, scale, glyph_index, GLYPH_PADDING, GLYPH_ONEDGE, 255.f / GLYPH_PADDING,
							&glyph.width, &glyph.height, &glyph.x_off, &glyph.y_off
						);
					}
				});
			results.push_back(std::move(result));
		}
		for (int i = 0; i < results.size(); i++) {
			results[i].get();
		}
		Profiler::end_scope(SCOPE_PARSE_GLYPH);
		stbrp_rect glyph_rects[NUM_CHAR] = {};
		for (int i = 0; i < NUM_CHAR; i++) {
			const auto& glyph = glyph_sdfs[i];
			glyph_rects[i] = {.w = glyph.width, .h = glyph.height};
		}
		stbrp_context context{};
		font.start_char = (char)START_CHAR;
		int base_atlas_size = MIN_ATLAS_SIZE;
		int max_atlas_size = MAX_ATLAS_SIZE;
		bool packed_success = false;
		Profiler::start_scope(SCOPE_FIND_SUITABLE_ATLAS);
		while (!packed_success && base_atlas_size <= max_atlas_size) {
			stbrp_init_target(&context, base_atlas_size, base_atlas_size, nullptr, 0);
			for (int i = 0; i < NUM_CHAR; i++) {
				glyph_rects[i].was_packed = false;
			}
			stbrp_pack_rects(&context, glyph_rects, NUM_CHAR);
			packed_success = true;
			for (int i = 0; i < NUM_CHAR; i++) {
				if (!glyph_rects[i].was_packed) {
					packed_success = false;
					break;
				}
			}
			if (!packed_success) {
				base_atlas_size *= 2;
			}
		}
		if (!packed_success) {
			throw std::runtime_error("Fail to baked char in font:" + file_path + "!");
		}
		Profiler::end_scope(SCOPE_FIND_SUITABLE_ATLAS);
		Profiler::start_scope(SCOPE_MERGE_ATLAS);
		font.size_atlas = base_atlas_size;
		font.max_char_height = MAX_PIXEL_HEIGHT;
		font.max_font_size = MAX_FONT_SIZE;
		font.center_font_size = MAX_FONT_SIZE / 2;
		font.min_font_size = MIN_FONT_SIZE;
		font.max_stroke_size = GLYPH_PADDING * 0.4;
		font.sfd_padding = GLYPH_PADDING;
		int ascent = 0, descent = 0, line_gap = 0;
		stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
		font.line_height = (ascent - descent + line_gap) * scale;
		font.atlas.resize(base_atlas_size * base_atlas_size);
		for (int i = 0; i < NUM_CHAR; i++) {
			const auto& glyph_rect = glyph_rects[i];
			const auto& glyph = glyph_sdfs[i];
			for (int row = 0; row < glyph_rect.h; row++) {
				uint8_t* dst_row = font.atlas.data() + (glyph_rect.y + row) * base_atlas_size + glyph_rect.x;
				uint8_t* src_row = glyph.bytes + row * glyph_rect.w;
				memcpy(dst_row, src_row, glyph_rect.w);
			}
		}
		for (int i = 0; i < NUM_CHAR; i++) {
			delete (glyph_sdfs[i].bytes);
		}
		for (int i = 0; i < NUM_CHAR; i++) {
			Char_Rect rect{};
			rect.tex_coord_x = glyph_rects[i].x / (float)base_atlas_size;
			rect.tex_coord_y = 1.f - (glyph_rects[i].y + glyph_rects[i].h) / (float)base_atlas_size;
			rect.tex_coord_width = glyph_rects[i].w / (float)base_atlas_size;
			rect.tex_coord_height = glyph_rects[i].h / (float)base_atlas_size;
			int advance_width = 0;
			stbtt_GetGlyphHMetrics(&font_info, glyph_indices[i], &advance_width, nullptr);
			rect.offset_x = (float)glyph_sdfs[i].x_off;
			rect.offset_y = (float)(-glyph_sdfs[i].y_off - glyph_rects[i].h);
			rect.bounding_width = advance_width * scale;
			font.rects.push_back(rect);
		}
		Profiler::end_scope(SCOPE_MERGE_ATLAS);
		Profiler::end_scope(SCOPE_LOAD_FONT);
		return font;
	}
} // namespace Parser