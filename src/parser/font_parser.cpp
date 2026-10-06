#include <parser/font_parser.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <log.h>
#include <sparse_set.h>
#include <stb_truetype.h>

namespace Parser {

	const int MAX_PIXEL_HEIGHT = 124;

	const int START_CHAR = 32;

	const int NUM_CHAR = 95;

	const int MAX_FONT_SIZE = 64;

	const int MIN_FONT_SIZE = 6;

	Font parse_font(std::string file_path) {
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
		stbtt_bakedchar baked_char[NUM_CHAR] = {};
		while (result <= 0 && base_atlas_size <= max_atlas_size) {
			font.atlas.resize(base_atlas_size * base_atlas_size);
			result = stbtt_BakeFontBitmap(
				(const unsigned char*)font.bytes.data(), font_info.fontstart, MAX_PIXEL_HEIGHT, font.atlas.data(),
				base_atlas_size, base_atlas_size, START_CHAR, NUM_CHAR, baked_char
			);
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
		float min_offset_y = 0;
		for (int i = 0; i < NUM_CHAR; i++) {
			char current_char = (char)(START_CHAR + i);
			stbtt_bakedchar baked = baked_char[i];
			Char_Rect rect{};
			rect.tex_coord_x = baked.x0 / (float)base_atlas_size;
			rect.tex_coord_y = 1.f - baked.y1 / (float)base_atlas_size;
			rect.tex_coord_width = (baked.x1 - baked.x0) / (float)base_atlas_size;
			rect.tex_coord_height = (baked.y1 - baked.y0) / (float)base_atlas_size;
			rect.offset_x = baked.xoff;
			rect.offset_y = -baked.yoff - (baked.y1 - baked.y0);
			min_offset_y = std::min(min_offset_y, rect.offset_y);
			rect.bounding_width = baked.xadvance;
			font.rect_map[current_char] = rect;
		}
		for (int i = 0; i < NUM_CHAR; i++) {
			char current_char = (char)(START_CHAR + i);
			font.rect_map[current_char].offset_y -= min_offset_y;
		}
		return font;
	}
} // namespace Parser