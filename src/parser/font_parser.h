#pragma once
#include <files.h>
#include <iostream>
#include <map>
#include <utils.h>

namespace Parser {

	const std::string SCOPE_LOAD_FONT_V1 = "LOAD_FONT_V1";

	const std::string SCOPE_LOAD_FONT = "LOAD_FONT";

	const std::string SCOPE_PARSE_GLYPH = "PARSE_GLYPH";

	const std::string SCOPE_FIND_SUITABLE_ATLAS = "FIND_SUITABLE_ATLAS";

	const std::string SCOPE_MERGE_ATLAS = "MERGE_ATLAS";

	struct Char_Rect {
		float tex_coord_x = 0.f, tex_coord_y = 0.f;
		float tex_coord_width = 0.f, tex_coord_height = 0.f;
		float bounding_width = 0.f;
		float offset_x = 0.f, offset_y = 0.f;
		inline friend std::ostream& operator<<(std::ostream& os, const Char_Rect& rect) {
			os << "{Char_Rect: x: " << rect.tex_coord_x << ", y: " << rect.tex_coord_y
			   << ", width: " << rect.tex_coord_width << ", height: " << rect.tex_coord_height
			   << ", bounding_width: " << rect.bounding_width << ", offset_x: " << rect.offset_x
			   << ", offset_y: " << rect.offset_y << "}";
			return os;
		}
	};

	struct Font {
		std::vector<Char_Rect> rects;
		std::vector<uint8_t> atlas;
		char start_char = 0;
		int max_char_height = 0;
		int size_atlas = 0;
		int min_font_size = 0;
		int center_font_size = 0;
		int max_font_size = 0;
		float line_height = 0.f;
		float max_stroke_size = 0.f;
		float sfd_padding = 0.f;
		inline friend std::ostream& operator<<(std::ostream& os, const Font& font) {
			os << "{Font: start_char: " << (int)font.start_char << ", max_char_height: " << font.max_char_height
			   << ", size_atlas: " << font.size_atlas << ", min_font_size: " << font.min_font_size
			   << ", center_font_size: " << font.center_font_size << ", max_font_size: " << font.max_font_size
			   << ", line_height: " << font.line_height << ", max_stroke_size: " << font.max_stroke_size
			   << ", sfd_padding: " << font.sfd_padding << ", rects size: " << font.rects.size()
			   << ", rects last: " << font.rects.back() << ", atlas: " << font.atlas.size() << "}";
			return os;
		}
	};

	Font parse_font(std::string file_path);

	Font parse_font_2(const std::string& file_path);

	void test(const std::string& file_path);

} // namespace Parser