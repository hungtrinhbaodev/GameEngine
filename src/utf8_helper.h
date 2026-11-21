/**
 * @Note: this file was written by Claude (Claude Code) on 2026-11-17, at the user's request.
 *
 * UTF-8 <-> char32_t conversion + NFC composition for Vietnamese.
 * - std::string stays UTF-8 everywhere in the API; decode to std::u32string only where characters
 *   matter (layout, measure, glyph lookup, caret). 1 char32_t = 1 code point.
 * - Bad input never throws: every broken byte becomes REPLACEMENT_CHAR (U+FFFD) and the good
 *   characters after it are kept.
 * - nfc_vietnamese turns "e" + U+0323 + U+0302 into "ệ" (U+1EC7) so the font only needs the
 *   precomposed letters. The table covers exactly the 146 Vietnamese letters (generated from the
 *   Unicode data, 132 pairs); other combining marks are left as they are.
 */
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace Helper {

	const char32_t REPLACEMENT_CHAR = 0xFFFD;

	const char32_t MAX_CODE_POINT = 0x10FFFF;

	/**
	 * @Note: 10xxxxxx = the 2nd / 3rd / 4th byte of a character.
	 * Stepping the caret left in a UTF-8 string = go back while this is true.
	 */
	inline bool is_utf8_continuation_byte(unsigned char c) {
		return (c >> 6) == 0x2;
	}

	inline bool is_surrogate(char32_t cp) {
		return cp >= 0xD800 && cp <= 0xDFFF;
	}

	/**
	 * @Note: start byte -> length, payload bits:
	 * 0xxxxxxx = 1 byte (7 bits), 110xxxxx = 2 (5), 1110xxxx = 3 (4), 11110xxx = 4 (3),
	 * every next byte 10xxxxxx gives 6 more bits. A character must use the shortest form
	 * (MIN_FOR_LEN), otherwise it is "overlong" and rejected.
	 */
	inline std::u32string utf8_to_u32(std::string_view s) {
		static const char32_t MIN_FOR_LEN[5] = {0, 0, 0x80, 0x800, 0x10000};
		std::u32string out;
		out.reserve(s.size());
		size_t i = 0;
		while (i < s.size()) {
			unsigned char c = (unsigned char)s[i];
			char32_t cp = 0;
			int len = 0;
			if (c < 0x80) {
				cp = c;
				len = 1;
			} else if ((c >> 5) == 0x6) {
				cp = c & 0x1F;
				len = 2;
			} else if ((c >> 4) == 0xE) {
				cp = c & 0x0F;
				len = 3;
			} else if ((c >> 3) == 0x1E) {
				cp = c & 0x07;
				len = 4;
			} else {
				// a stray 10xxxxxx or a byte that never starts a character (F8..FF).
				out.push_back(REPLACEMENT_CHAR);
				i++;
				continue;
			}
			if (i + len > s.size()) {
				// cut off at the end of the string.
				out.push_back(REPLACEMENT_CHAR);
				break;
			}
			bool ok = true;
			for (int k = 1; k < len; k++) {
				unsigned char cc = (unsigned char)s[i + k];
				if (!is_utf8_continuation_byte(cc)) {
					ok = false;
					break;
				}
				cp = (cp << 6) | (cc & 0x3F);
			}
			if (!ok || cp < MIN_FOR_LEN[len] || cp > MAX_CODE_POINT || is_surrogate(cp)) {
				// skip only the start byte: a good character right after it is still decoded.
				out.push_back(REPLACEMENT_CHAR);
				i++;
				continue;
			}
			out.push_back(cp);
			i += len;
		}
		return out;
	}

	inline void append_utf8(std::string& out, char32_t cp) {
		if (cp > MAX_CODE_POINT || is_surrogate(cp)) {
			cp = REPLACEMENT_CHAR;
		}
		if (cp < 0x80) {
			out += (char)cp;
		} else if (cp < 0x800) {
			out += (char)(0xC0 | (cp >> 6));
			out += (char)(0x80 | (cp & 0x3F));
		} else if (cp < 0x10000) {
			out += (char)(0xE0 | (cp >> 12));
			out += (char)(0x80 | ((cp >> 6) & 0x3F));
			out += (char)(0x80 | (cp & 0x3F));
		} else {
			out += (char)(0xF0 | (cp >> 18));
			out += (char)(0x80 | ((cp >> 12) & 0x3F));
			out += (char)(0x80 | ((cp >> 6) & 0x3F));
			out += (char)(0x80 | (cp & 0x3F));
		}
	}

	// [Claude 2026-11-17] number -> char32_t text, like std::to_string (digits are ASCII, so no NFC needed).
	template <typename T> inline std::u32string to_u32string(T value) {
		return utf8_to_u32(std::to_string(value));
	}

	inline std::string u32_to_utf8(std::u32string_view s) {
		std::string out;
		out.reserve(s.size());
		for (char32_t cp : s) {
			append_utf8(out, cp);
		}
		return out;
	}

	namespace Detail {

		struct Composition {
			char32_t base;
			char32_t mark;
			char32_t composed;
		};

		/**
		 * @Note: generated from Python unicodedata 13.0.0: every (base, mark) -> letter step
		 * of the 146 Vietnamese letters, sorted by (base, mark) for the binary search.
		 * Two-step letters go through their middle form: e + U+0323 -> ẹ, ẹ + U+0302 -> ệ.
		 */
		inline const Composition VIETNAMESE_COMPOSITIONS[] = {
			{0x0041, 0x0300, 0x00C0}, // À
			{0x0041, 0x0301, 0x00C1}, // Á
			{0x0041, 0x0302, 0x00C2}, // Â
			{0x0041, 0x0303, 0x00C3}, // Ã
			{0x0041, 0x0306, 0x0102}, // Ă
			{0x0041, 0x0309, 0x1EA2}, // Ả
			{0x0041, 0x0323, 0x1EA0}, // Ạ
			{0x0045, 0x0300, 0x00C8}, // È
			{0x0045, 0x0301, 0x00C9}, // É
			{0x0045, 0x0302, 0x00CA}, // Ê
			{0x0045, 0x0303, 0x1EBC}, // Ẽ
			{0x0045, 0x0309, 0x1EBA}, // Ẻ
			{0x0045, 0x0323, 0x1EB8}, // Ẹ
			{0x0049, 0x0300, 0x00CC}, // Ì
			{0x0049, 0x0301, 0x00CD}, // Í
			{0x0049, 0x0303, 0x0128}, // Ĩ
			{0x0049, 0x0309, 0x1EC8}, // Ỉ
			{0x0049, 0x0323, 0x1ECA}, // Ị
			{0x004F, 0x0300, 0x00D2}, // Ò
			{0x004F, 0x0301, 0x00D3}, // Ó
			{0x004F, 0x0302, 0x00D4}, // Ô
			{0x004F, 0x0303, 0x00D5}, // Õ
			{0x004F, 0x0309, 0x1ECE}, // Ỏ
			{0x004F, 0x031B, 0x01A0}, // Ơ
			{0x004F, 0x0323, 0x1ECC}, // Ọ
			{0x0055, 0x0300, 0x00D9}, // Ù
			{0x0055, 0x0301, 0x00DA}, // Ú
			{0x0055, 0x0303, 0x0168}, // Ũ
			{0x0055, 0x0309, 0x1EE6}, // Ủ
			{0x0055, 0x031B, 0x01AF}, // Ư
			{0x0055, 0x0323, 0x1EE4}, // Ụ
			{0x0059, 0x0300, 0x1EF2}, // Ỳ
			{0x0059, 0x0301, 0x00DD}, // Ý
			{0x0059, 0x0303, 0x1EF8}, // Ỹ
			{0x0059, 0x0309, 0x1EF6}, // Ỷ
			{0x0059, 0x0323, 0x1EF4}, // Ỵ
			{0x0061, 0x0300, 0x00E0}, // à
			{0x0061, 0x0301, 0x00E1}, // á
			{0x0061, 0x0302, 0x00E2}, // â
			{0x0061, 0x0303, 0x00E3}, // ã
			{0x0061, 0x0306, 0x0103}, // ă
			{0x0061, 0x0309, 0x1EA3}, // ả
			{0x0061, 0x0323, 0x1EA1}, // ạ
			{0x0065, 0x0300, 0x00E8}, // è
			{0x0065, 0x0301, 0x00E9}, // é
			{0x0065, 0x0302, 0x00EA}, // ê
			{0x0065, 0x0303, 0x1EBD}, // ẽ
			{0x0065, 0x0309, 0x1EBB}, // ẻ
			{0x0065, 0x0323, 0x1EB9}, // ẹ
			{0x0069, 0x0300, 0x00EC}, // ì
			{0x0069, 0x0301, 0x00ED}, // í
			{0x0069, 0x0303, 0x0129}, // ĩ
			{0x0069, 0x0309, 0x1EC9}, // ỉ
			{0x0069, 0x0323, 0x1ECB}, // ị
			{0x006F, 0x0300, 0x00F2}, // ò
			{0x006F, 0x0301, 0x00F3}, // ó
			{0x006F, 0x0302, 0x00F4}, // ô
			{0x006F, 0x0303, 0x00F5}, // õ
			{0x006F, 0x0309, 0x1ECF}, // ỏ
			{0x006F, 0x031B, 0x01A1}, // ơ
			{0x006F, 0x0323, 0x1ECD}, // ọ
			{0x0075, 0x0300, 0x00F9}, // ù
			{0x0075, 0x0301, 0x00FA}, // ú
			{0x0075, 0x0303, 0x0169}, // ũ
			{0x0075, 0x0309, 0x1EE7}, // ủ
			{0x0075, 0x031B, 0x01B0}, // ư
			{0x0075, 0x0323, 0x1EE5}, // ụ
			{0x0079, 0x0300, 0x1EF3}, // ỳ
			{0x0079, 0x0301, 0x00FD}, // ý
			{0x0079, 0x0303, 0x1EF9}, // ỹ
			{0x0079, 0x0309, 0x1EF7}, // ỷ
			{0x0079, 0x0323, 0x1EF5}, // ỵ
			{0x00C2, 0x0300, 0x1EA6}, // Ầ
			{0x00C2, 0x0301, 0x1EA4}, // Ấ
			{0x00C2, 0x0303, 0x1EAA}, // Ẫ
			{0x00C2, 0x0309, 0x1EA8}, // Ẩ
			{0x00CA, 0x0300, 0x1EC0}, // Ề
			{0x00CA, 0x0301, 0x1EBE}, // Ế
			{0x00CA, 0x0303, 0x1EC4}, // Ễ
			{0x00CA, 0x0309, 0x1EC2}, // Ể
			{0x00D4, 0x0300, 0x1ED2}, // Ồ
			{0x00D4, 0x0301, 0x1ED0}, // Ố
			{0x00D4, 0x0303, 0x1ED6}, // Ỗ
			{0x00D4, 0x0309, 0x1ED4}, // Ổ
			{0x00E2, 0x0300, 0x1EA7}, // ầ
			{0x00E2, 0x0301, 0x1EA5}, // ấ
			{0x00E2, 0x0303, 0x1EAB}, // ẫ
			{0x00E2, 0x0309, 0x1EA9}, // ẩ
			{0x00EA, 0x0300, 0x1EC1}, // ề
			{0x00EA, 0x0301, 0x1EBF}, // ế
			{0x00EA, 0x0303, 0x1EC5}, // ễ
			{0x00EA, 0x0309, 0x1EC3}, // ể
			{0x00F4, 0x0300, 0x1ED3}, // ồ
			{0x00F4, 0x0301, 0x1ED1}, // ố
			{0x00F4, 0x0303, 0x1ED7}, // ỗ
			{0x00F4, 0x0309, 0x1ED5}, // ổ
			{0x0102, 0x0300, 0x1EB0}, // Ằ
			{0x0102, 0x0301, 0x1EAE}, // Ắ
			{0x0102, 0x0303, 0x1EB4}, // Ẵ
			{0x0102, 0x0309, 0x1EB2}, // Ẳ
			{0x0103, 0x0300, 0x1EB1}, // ằ
			{0x0103, 0x0301, 0x1EAF}, // ắ
			{0x0103, 0x0303, 0x1EB5}, // ẵ
			{0x0103, 0x0309, 0x1EB3}, // ẳ
			{0x01A0, 0x0300, 0x1EDC}, // Ờ
			{0x01A0, 0x0301, 0x1EDA}, // Ớ
			{0x01A0, 0x0303, 0x1EE0}, // Ỡ
			{0x01A0, 0x0309, 0x1EDE}, // Ở
			{0x01A0, 0x0323, 0x1EE2}, // Ợ
			{0x01A1, 0x0300, 0x1EDD}, // ờ
			{0x01A1, 0x0301, 0x1EDB}, // ớ
			{0x01A1, 0x0303, 0x1EE1}, // ỡ
			{0x01A1, 0x0309, 0x1EDF}, // ở
			{0x01A1, 0x0323, 0x1EE3}, // ợ
			{0x01AF, 0x0300, 0x1EEA}, // Ừ
			{0x01AF, 0x0301, 0x1EE8}, // Ứ
			{0x01AF, 0x0303, 0x1EEE}, // Ữ
			{0x01AF, 0x0309, 0x1EEC}, // Ử
			{0x01AF, 0x0323, 0x1EF0}, // Ự
			{0x01B0, 0x0300, 0x1EEB}, // ừ
			{0x01B0, 0x0301, 0x1EE9}, // ứ
			{0x01B0, 0x0303, 0x1EEF}, // ữ
			{0x01B0, 0x0309, 0x1EED}, // ử
			{0x01B0, 0x0323, 0x1EF1}, // ự
			{0x1EA0, 0x0302, 0x1EAC}, // Ậ
			{0x1EA0, 0x0306, 0x1EB6}, // Ặ
			{0x1EA1, 0x0302, 0x1EAD}, // ậ
			{0x1EA1, 0x0306, 0x1EB7}, // ặ
			{0x1EB8, 0x0302, 0x1EC6}, // Ệ
			{0x1EB9, 0x0302, 0x1EC7}, // ệ
			{0x1ECC, 0x0302, 0x1ED8}, // Ộ
			{0x1ECD, 0x0302, 0x1ED9}, // ộ
		};

		/**
		 * @Note: canonical combining class of the marks Vietnamese uses
		 * (horn 216, dot below 220, the rest 230). 0 = not one of them.
		 */
		inline int vietnamese_mark_class(char32_t cp) {
			switch (cp) {
				case 0x031B:
					return 216;
				case 0x0323:
					return 220;
				case 0x0300:
				case 0x0301:
				case 0x0302:
				case 0x0303:
				case 0x0306:
				case 0x0309:
					return 230;
				default:
					return 0;
			}
		}

	} // namespace Detail

	/**
	 * @Note: returns 0 when (base, mark) has no Vietnamese precomposed letter.
	 */
	inline char32_t compose_vietnamese(char32_t base, char32_t mark) {
		const auto* begin = std::begin(Detail::VIETNAMESE_COMPOSITIONS);
		const auto* end = std::end(Detail::VIETNAMESE_COMPOSITIONS);
		const auto* it =
			std::lower_bound(begin, end, Detail::Composition{base, mark, 0}, [](const auto& a, const auto& b) {
				return a.base != b.base ? a.base < b.base : a.mark < b.mark;
			});
		if (it == end || it->base != base || it->mark != mark) {
			return 0;
		}
		return it->composed;
	}

	/**
	 * @Note: NFC for the Vietnamese marks. For each letter + the marks after it:
	 * 1. sort the marks by combining class (stable), so "e" U+0302 U+0323 and "e" U+0323 U+0302 both
	 *    become "ệ", like the Unicode canonical order;
	 * 2. compose them into the letter one by one; a mark that cannot compose stays after the letter,
	 *    and it blocks later marks of the same or lower class (Unicode blocking rule).
	 */
	inline std::u32string nfc_vietnamese(std::u32string_view s) {
		std::u32string out;
		out.reserve(s.size());
		std::u32string marks;
		std::u32string rest;
		size_t i = 0;
		while (i < s.size()) {
			char32_t starter = s[i++];
			marks.clear();
			while (i < s.size() && Detail::vietnamese_mark_class(s[i]) > 0) {
				marks.push_back(s[i++]);
			}
			if (marks.empty()) {
				out.push_back(starter);
				continue;
			}
			std::stable_sort(marks.begin(), marks.end(), [](char32_t a, char32_t b) {
				return Detail::vietnamese_mark_class(a) < Detail::vietnamese_mark_class(b);
			});
			rest.clear();
			int last_blocking_class = 0;
			for (char32_t mark : marks) {
				int mark_class = Detail::vietnamese_mark_class(mark);
				char32_t composed = last_blocking_class < mark_class ? compose_vietnamese(starter, mark) : 0;
				if (composed != 0) {
					starter = composed;
				} else {
					rest.push_back(mark);
					last_blocking_class = mark_class;
				}
			}
			out.push_back(starter);
			out += rest;
		}
		return out;
	}

	/**
	 * @Note: what text input (edit box, clipboard, set_label_string) should use:
	 * decode + compose, so 1 char32_t = 1 Vietnamese letter = 1 glyph = 1 caret step.
	 */
	inline std::u32string utf8_to_u32_nfc(std::string_view s) {
		return nfc_vietnamese(utf8_to_u32(s));
	}

	/**
	 * @Note: number of code points (not bytes) without decoding: count the bytes that are not 10xxxxxx.
	 * Equals utf8_to_u32(s).size() for valid UTF-8.
	 */
	inline size_t utf8_length(std::string_view s) {
		size_t count = 0;
		for (char c : s) {
			if (!is_utf8_continuation_byte((unsigned char)c)) {
				count++;
			}
		}
		return count;
	}

	/**
	 * @Note: decode UTF-8 to u32, composing Vietnamese marks into precomposed letters (same as
	 * utf8_to_u32_nfc), but also returning, for every output u32 character, the byte offset and byte
	 * length of the span in `s` it came from - so a caller holding only an index into the composed
	 * output (e.g. from a click / caret) can still recover the exact raw UTF-8 bytes that character
	 * came from, even though composing merges multiple source code points and is not otherwise
	 * invertible. `byte_start` / `byte_len` are cleared and sized to match the returned string exactly.
	 */
	inline std::u32string utf8_to_u32_nfc_mapped(
		std::string_view s, std::vector<size_t>& byte_start, std::vector<size_t>& byte_len
	) {
		static const char32_t MIN_FOR_LEN[5] = {0, 0, 0x80, 0x800, 0x10000};

		// pass 1: decode, keeping each decoded code point's own byte span.
		std::u32string decoded;
		std::vector<size_t> decoded_start, decoded_len;
		decoded.reserve(s.size());
		decoded_start.reserve(s.size());
		decoded_len.reserve(s.size());
		{
			size_t i = 0;
			while (i < s.size()) {
				size_t start = i;
				unsigned char c = (unsigned char)s[i];
				char32_t cp = 0;
				int len = 0;
				if (c < 0x80) {
					cp = c;
					len = 1;
				} else if ((c >> 5) == 0x6) {
					cp = c & 0x1F;
					len = 2;
				} else if ((c >> 4) == 0xE) {
					cp = c & 0x0F;
					len = 3;
				} else if ((c >> 3) == 0x1E) {
					cp = c & 0x07;
					len = 4;
				} else {
					decoded.push_back(REPLACEMENT_CHAR);
					decoded_start.push_back(start);
					decoded_len.push_back(1);
					i++;
					continue;
				}
				if (i + len > s.size()) {
					decoded.push_back(REPLACEMENT_CHAR);
					decoded_start.push_back(start);
					decoded_len.push_back(s.size() - i);
					break;
				}
				bool ok = true;
				for (int k = 1; k < len; k++) {
					unsigned char cc = (unsigned char)s[i + k];
					if (!is_utf8_continuation_byte(cc)) {
						ok = false;
						break;
					}
					cp = (cp << 6) | (cc & 0x3F);
				}
				if (!ok || cp < MIN_FOR_LEN[len] || cp > MAX_CODE_POINT || is_surrogate(cp)) {
					decoded.push_back(REPLACEMENT_CHAR);
					decoded_start.push_back(start);
					decoded_len.push_back(1);
					i++;
					continue;
				}
				decoded.push_back(cp);
				decoded_start.push_back(start);
				decoded_len.push_back(len);
				i += len;
			}
		}

		// pass 2: compose Vietnamese marks (same clustering as nfc_vietnamese), carrying spans through.
		std::u32string out;
		byte_start.clear();
		byte_len.clear();
		out.reserve(decoded.size());
		byte_start.reserve(decoded.size());
		byte_len.reserve(decoded.size());

		std::vector<size_t> order;
		std::u32string rest;
		std::vector<size_t> rest_start, rest_len;
		size_t i = 0;
		while (i < decoded.size()) {
			size_t starter_start = decoded_start[i];
			size_t starter_len = decoded_len[i];
			char32_t starter = decoded[i++];

			size_t marks_begin = i;
			while (i < decoded.size() && Detail::vietnamese_mark_class(decoded[i]) > 0) {
				i++;
			}
			size_t marks_end = i; // [marks_begin, marks_end) in `decoded`, still in source order.

			if (marks_begin == marks_end) {
				out.push_back(starter);
				byte_start.push_back(starter_start);
				byte_len.push_back(starter_len);
				continue;
			}

			order.resize(marks_end - marks_begin);
			for (size_t k = 0; k < order.size(); k++) {
				order[k] = marks_begin + k;
			}
			std::stable_sort(order.begin(), order.end(), [&decoded](size_t a, size_t b) {
				return Detail::vietnamese_mark_class(decoded[a]) < Detail::vietnamese_mark_class(decoded[b]);
			});

			rest.clear();
			rest_start.clear();
			rest_len.clear();
			size_t merged_end = starter_start + starter_len;
			int last_blocking_class = 0;
			for (size_t idx : order) {
				char32_t mark = decoded[idx];
				int mark_class = Detail::vietnamese_mark_class(mark);
				char32_t composed = last_blocking_class < mark_class ? compose_vietnamese(starter, mark) : 0;
				if (composed != 0) {
					starter = composed;
					merged_end = std::max(merged_end, decoded_start[idx] + decoded_len[idx]);
				} else {
					rest.push_back(mark);
					rest_start.push_back(decoded_start[idx]);
					rest_len.push_back(decoded_len[idx]);
					last_blocking_class = mark_class;
				}
			}

			out.push_back(starter);
			byte_start.push_back(starter_start);
			byte_len.push_back(merged_end - starter_start);
			out += rest;
			for (size_t k = 0; k < rest.size(); k++) {
				byte_start.push_back(rest_start[k]);
				byte_len.push_back(rest_len[k]);
			}
		}
		return out;
	}

} // namespace Helper
