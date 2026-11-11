#pragma once
#include <consts.h>
#include <functional>
#include <glm/glm.hpp>
#include <handle.h>
#include <tuple>
#include <vector>

namespace Engine {

	namespace Utils {

		void remove_entities_from(std::vector<Handle>& list, const Handle& e);

		void insert_entities_to(std::vector<Handle>& list, const Handle& e);

		void sort_entities(std::vector<Handle>& list, const std::function<bool(const Handle&, const Handle&)>& fn);

		std::vector<Handle> find_entites_satify_from(
			std::vector<Handle>& list, const std::function<bool(const Handle& e)>& with_filter
		);

		/**
		 * @Note: return function will return:
		 * tuple[0]: found entitie satify.
		 * tuple[1]: break searching.
		 */
		std::vector<Handle> find_entites_satify_from(
			std::vector<Handle>& list, const std::function<std::tuple<bool, bool>(const Handle& e)>& with_filter
		);

		glm::vec2 fit_container_position_with(
			const glm::vec2& parent_size, const glm::vec2& container_size, const glm::vec2& position
		);

	} // namespace Utils

} // namespace Engine