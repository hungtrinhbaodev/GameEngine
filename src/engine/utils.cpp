#include <engine/utils.h>

namespace Engine {
	namespace Utils {
		void remove_entities_from(std::vector<Handle>& list, const Handle& e) {
			for (int i = 0; i < list.size(); i++) {
				if (list[i].id() == e.id()) {
					list[i] = list.back();
					list.pop_back();
					i--;
				}
			}
		}

		void insert_entities_to(std::vector<Handle>& list, const Handle& e) {
			bool found = false;
			for (int i = 0; i < list.size(); i++) {
				if (list[i].id() == e.id()) {
					found = true;
					break;
				}
			}
			if (!found) {
				list.push_back(e);
			}
		}

		void sort_entities(std::vector<Handle>& list, const std::function<bool(const Handle&, const Handle&)>& fn) {
			std::sort(list.begin(), list.end(), fn);
		}

		std::vector<Handle> find_entites_satify_from(
			std::vector<Handle>& list, const std::function<bool(const Handle& e)>& with_filter
		) {
			auto wrapper_filter = [&with_filter](const Handle& e) { return std::make_tuple(with_filter(e), false); };
			return find_entites_satify_from(list, wrapper_filter);
		}

		std::vector<Handle> find_entites_satify_from(
			std::vector<Handle>& list, const std::function<std::tuple<bool, bool>(const Handle& e)>& with_filter
		) {
			std::vector<Handle> found_entities{};
			for (const Handle& e : list) {
				auto [found, stop] = with_filter(e);
				if (found) {
					found_entities.push_back(e);
				}
				if (stop)
					break;
			}
			return found_entities;
		}

		glm::vec2 fit_container_position_with(
			const glm::vec2& parent_size, const glm::vec2& container_size, const glm::vec2& position
		) {
			return glm::vec2(
				std::min(std::max(parent_size.x - container_size.x, position.x), 0.f),
				std::min(std::max(parent_size.y - container_size.y, position.y), 0.f)
			);
		}

		void add_self_clipping_to_clipping_ancestors(
			uint32_t& number_valid_handle, std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>& clipping_handle_ancestors,
			const Handle& self_clipping
		) {
			if (number_valid_handle >= ::Const::MAX_DEPTH_CLIPPING) {
				for (int i = 0; i < clipping_handle_ancestors.size() - 1; i++) {
					clipping_handle_ancestors[i] = clipping_handle_ancestors[i + 1];
				}
				clipping_handle_ancestors[number_valid_handle - 1] = self_clipping;
			} else {
				clipping_handle_ancestors[number_valid_handle++] = self_clipping;
			}
		}
	} // namespace Utils
} // namespace Engine