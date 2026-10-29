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

	struct Font_SFD_Meta_Data_Cache {
		int max_font_size = 0;
		int max_char_height = 0;
		float sfd_padding = 0;
		std::filesystem::file_time_type last_time_save;
		std::uintmax_t file_size;
		bool operator==(const Font_SFD_Meta_Data_Cache other) {
			return max_font_size == other.max_font_size && max_char_height == other.max_char_height &&
				   sfd_padding == other.sfd_padding && last_time_save == other.last_time_save &&
				   file_size == other.file_size;
		}
		friend std::ostream& operator<<(std::ostream& os, const Font_SFD_Meta_Data_Cache& meta_data) {
			os << "{max_font_size: " << meta_data.max_font_size << ", max_char_height: " << meta_data.max_char_height
			   << ", sfd_padding: " << meta_data.sfd_padding
			   << ", last_time_save: " << (long long)meta_data.last_time_save.time_since_epoch().count()
			   << ", file_size: " << (size_t)meta_data.file_size << "}" << std::endl;
			return os;
		}
	};

	const int MAX_PIXEL_HEIGHT = 72;

	const int START_CHAR = 32;

	const int NUM_CHAR = 95;

	const int MAX_FONT_SIZE = 64;

	const int MIN_FONT_SIZE = 6;

	const int ATLAS_PADDING = 2;

	const int GLYPH_PADDING = 16;

	const uint8_t GLYPH_ONEDGE = 128;

	const int MIN_ATLAS_SIZE = 512;

	const int MAX_ATLAS_SIZE = 2048;

	const std::string META_DATA_EXTENSION_NAME = ".font_meta";

	const std::string BIN_DATA_EXTENSION_NAME = ".bin_cached";

	Font_SFD_Meta_Data_Cache get_meta_data_from(const std::string& font_path) {
		Font_SFD_Meta_Data_Cache meta_data{};
		meta_data.max_char_height = MAX_PIXEL_HEIGHT;
		meta_data.sfd_padding = GLYPH_PADDING;
		meta_data.max_font_size = MAX_FONT_SIZE;
		meta_data.last_time_save = std::filesystem::last_write_time(font_path);
		meta_data.file_size = std::filesystem::file_size(font_path);
		return meta_data;
	}

	std::string get_meta_data_path_from(const std::string& font_path) {
		return Utils::get_folder_path_from(font_path) + Utils::get_file_name_from(font_path) + META_DATA_EXTENSION_NAME;
	}

	std::string get_bin_data_path_from(const std::string& font_path) {
		return Utils::get_folder_path_from(font_path) + Utils::get_file_name_from(font_path) + BIN_DATA_EXTENSION_NAME;
	}

	void save_bin_cached_font(const std::string& font_path, Font& font) {
		std::vector<char> to_bytes{};
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, start_char), sizeof(char));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, max_char_height), sizeof(int));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, size_atlas), sizeof(int));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, min_font_size), sizeof(int));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, center_font_size), sizeof(int));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, max_font_size), sizeof(int));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, line_height), sizeof(float));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, max_stroke_size), sizeof(float));
		Utils::put_byte_from(&font, to_bytes, offsetof(Font, sfd_padding), sizeof(float));
		short num_rect = (short)font.rects.size();
		Utils::put_byte_from(&num_rect, to_bytes, 0, sizeof(short));
		for (int i = 0; i < num_rect; i++) {
			Utils::put_byte_from(font.rects.data(), to_bytes, sizeof(Char_Rect) * i, sizeof(Char_Rect));
		}
		uint32_t atlas_size = font.atlas.size();
		Utils::put_byte_from(&atlas_size, to_bytes, 0, sizeof(uint32_t));
		Utils::put_byte_from(font.atlas.data(), to_bytes, 0, atlas_size);
		auto bin_file_name = get_bin_data_path_from(font_path);
		Files::save_bytes_to_file(bin_file_name, to_bytes);
	}

	Font load_bin_cached_font(const std::string& font_path) {
		auto bin_file_name = get_bin_data_path_from(font_path);
		std::vector<char> bytes = Files::read_file(bin_file_name);
		size_t offset = (size_t)0;
		Font font{};
		offset += Utils::read_byte_from<char>(bytes, &font.start_char, offset);
		offset += Utils::read_byte_from<int>(bytes, &font.max_char_height, offset);
		offset += Utils::read_byte_from<int>(bytes, &font.size_atlas, offset);
		offset += Utils::read_byte_from<int>(bytes, &font.min_font_size, offset);
		offset += Utils::read_byte_from<int>(bytes, &font.center_font_size, offset);
		offset += Utils::read_byte_from<int>(bytes, &font.max_font_size, offset);
		offset += Utils::read_byte_from<float>(bytes, &font.line_height, offset);
		offset += Utils::read_byte_from<float>(bytes, &font.max_stroke_size, offset);
		offset += Utils::read_byte_from<float>(bytes, &font.sfd_padding, offset);
		short num_rect = 0;
		offset += Utils::read_byte_from<short>(bytes, &num_rect, offset);
		for (int i = 0; i < num_rect; i++) {
			Char_Rect rect{};
			offset += Utils::read_byte_from<Char_Rect>(bytes, &rect, offset);
			font.rects.push_back(rect);
		}
		uint32_t atlas_size = 0;
		offset += Utils::read_byte_from<uint32_t>(bytes, &atlas_size, offset);
		font.atlas.resize(atlas_size);
		void* dst = font.atlas.data();
		void* src = bytes.data() + offset;
		memcpy(dst, src, atlas_size);
		Profiler::end_scope(SCOPE_LOAD_FONT);
		return font;
	}

	bool is_font_cached_on_disk(const std::string& font_path) {
		auto meta_data_file_name = get_meta_data_path_from(font_path);
		if (!Files::is_file_exists(meta_data_file_name)) {
			return false;
		}
		auto bin_data_file_name = get_bin_data_path_from(font_path);
		if (!Files::is_file_exists(bin_data_file_name)) {
			return false;
		}
		Font_SFD_Meta_Data_Cache current_meta_data = get_meta_data_from(font_path);
		Font_SFD_Meta_Data_Cache last_meta_data{};
		auto meta_data_bytes = Files::read_file(meta_data_file_name);
		memcpy(&last_meta_data, meta_data_bytes.data(), sizeof(Font_SFD_Meta_Data_Cache));
		return current_meta_data == last_meta_data;
	}

	Font parse_font(std::string file_path) {
		Profiler::start_scope(SCOPE_LOAD_FONT_V1);
		Font font{};
		std::vector<char> bytes = Files::read_file(file_path);
		stbtt_fontinfo font_info{};
		stbtt_InitFont(&font_info, (const unsigned char*)bytes.data(), 0);
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
				&pc, (const unsigned char*)bytes.data(), font_info.fontstart, MAX_PIXEL_HEIGHT, START_CHAR, NUM_CHAR,
				packed_chars
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
		if (is_font_cached_on_disk(file_path)) {
			return load_bin_cached_font(file_path);
		}
		std::vector<char> bytes = Files::read_file(file_path);
		stbtt_fontinfo font_info{};
		stbtt_InitFont(&font_info, (const unsigned char*)bytes.data(), 0);
		float scale = stbtt_ScaleForPixelHeight(&font_info, MAX_PIXEL_HEIGHT);
		std::vector<int> glyph_indices(NUM_CHAR, -1);
		for (int i = 0; i < NUM_CHAR; i++) {
			glyph_indices[i] = stbtt_FindGlyphIndex(&font_info, (char)(START_CHAR + i));
		}
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
		font.size_atlas = base_atlas_size;
		font.max_char_height = MAX_PIXEL_HEIGHT;
		font.max_font_size = MAX_FONT_SIZE;
		font.center_font_size = MAX_FONT_SIZE / 2;
		font.min_font_size = MIN_FONT_SIZE;
		font.max_stroke_size = GLYPH_PADDING * 0.8;
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
		Font_SFD_Meta_Data_Cache meta_data = get_meta_data_from(file_path);
		auto meta_data_path = get_meta_data_path_from(file_path);
		std::vector<char> meta_data_bytes(sizeof(Font_SFD_Meta_Data_Cache));
		memcpy(meta_data_bytes.data(), &meta_data, sizeof(Font_SFD_Meta_Data_Cache));
		Files::save_bytes_to_file(meta_data_path, meta_data_bytes);
		save_bin_cached_font(file_path, font);
		Profiler::end_scope(SCOPE_LOAD_FONT);
		return font;
	}

	void test(const std::string& file_path) {
		Font font = parse_font_2(file_path);
		save_bin_cached_font(file_path, font);
		Font font_result = load_bin_cached_font(file_path);
		Log::info("What is font_result", font_result);
	}
} // namespace Parser