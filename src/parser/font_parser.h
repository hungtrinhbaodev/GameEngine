#pragma once
#include <files.h>
#include <iostream>
#include <map>
#include <utils.h>

namespace Parser {

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
		std::vector<char> bytes;
		std::unordered_map<char, Char_Rect> rect_map{};
		std::vector<uint8_t> atlas{};
		int max_char_height = 0;
		int size_atlas = 0;
		int min_font_size = 0;
		int max_font_size = 0;
		float line_height = 0.f;
	};

	Font parse_font(std::string file_path);

} // namespace Parser