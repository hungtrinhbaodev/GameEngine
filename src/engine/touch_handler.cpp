#include <consts.h>
#include <engine/engine.h>
#include <engine/structs.h>
#include <engine/touch_handler.h>
#include <glm/glm.hpp>
#include <id_generator.h>
#include <log.h>
#include <sparse_set.h>

namespace Engine {

	const float RATE_CANCEL_MOVE = 0.5f;

	const uint32_t MAX_LOG_RAISE = 10;

	Id_Generator id_generator;

	Sparse_Set<Handled_Touches_Information> handling_touches;

	std::vector<Handled_Touches_Information> handled_touches;

	Sparse_Set<bool> handling_entities;

	uint32_t window_unique_touch_id = ::Const::MAX_UINT32;

	uint32_t current_log_count = 0;

	bool is_canceling = false;

	uint32_t spawn_touch(const glm::vec2& touch_world_position) {
		uint32_t touch_id = id_generator.gen_id();
		Handled_Touches_Information touch_info{};
		touch_info.touch_state = TOUCH_STATE::TOUCH_INIT;
		touch_info.touch_world_position = touch_world_position;
		handling_touches.insert(touch_id, touch_info);
		return touch_id;
	}

	void release_touch(uint32_t touch_id, const glm::vec2& touch_world_position) {
		if (!handling_touches.has(touch_id))
			return;
		auto& touch_info = handling_touches.modify(touch_id);
		touch_info.touch_state = TOUCH_STATE::TOUCH_ENDED;
		touch_info.touch_world_position = touch_world_position;
	}

	void on_mouse_button(GLFWwindow* window, int button, int action, int mods) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT: {
				// spawn new touch id handle this.
				double x, y;
				glfwGetCursorPos(window, &x, &y);
				glm::vec2 screen_size = get_window_size();
				glm::vec2 world_pos = glm::vec2((float)x, screen_size.y - (float)y);
				if (action == GLFW_PRESS) {
					window_unique_touch_id = spawn_touch(world_pos);
				} else if (action == GLFW_RELEASE) {
					/**
					 * @Note maybe the touch canceled because of
					 * touch rate or outside screen!
					 */
					if (window_unique_touch_id == ::Const::MAX_UINT32) {
						break;
					}
					release_touch(window_unique_touch_id, world_pos);
				}
				break;
			}
			default: {
				break;
			}
		}
	}

	void on_cursor_position(GLFWwindow* window, double x, double y) {
		if (!handling_touches.has(window_unique_touch_id))
			return;
		auto& touch_info = handling_touches.modify(window_unique_touch_id);
		bool is_canceling = false;
		const auto& last_touch = touch_info.touch_world_position;
		glm::vec2 screen_size = get_window_size();
		glm::vec2 cur_touch{(float)x, screen_size.y - (float)y};
		if (glm::distance(cur_touch, last_touch) >= RATE_CANCEL_MOVE * std::max(screen_size.x, screen_size.y) ||
			std::min(cur_touch.x, cur_touch.y) < 0.f || cur_touch.x > screen_size.x || cur_touch.y > screen_size.y) {
			is_canceling = true;
		}
		if (is_canceling) {
			touch_info.touch_state = TOUCH_STATE::TOUCH_CANCEL;
		}
		switch (touch_info.touch_state) {
			case TOUCH_STATE::TOUCH_BEGAN:
			case TOUCH_STATE::TOUCH_MOVE: {
				touch_info.touch_world_position = cur_touch;
				break;
			}
		}
	}

	void init_touch_handler(GLFWwindow* window) {
		glfwSetMouseButtonCallback(window, on_mouse_button);
		glfwSetCursorPosCallback(window, on_cursor_position);
	}

	void handle_touches_logic() {
		handled_touches.clear();
		std::vector<uint32_t> removed_touch_ids;
		for (const auto& touch_id : handling_touches.keys()) {
			auto& touch_info = handling_touches.modify(touch_id);
			switch (touch_info.touch_state) {
				case TOUCH_STATE::TOUCH_INIT: {
					touch_info.entities.clear();
					auto enitites = Engine::find_entites_listen_touch_at_(touch_info.touch_world_position);
					for (const auto& e : enitites) {
						if (handling_entities.has(e.id()))
							continue;
						handling_entities.insert(e.id(), true);
						touch_info.entities.push_back(e);
					}
					touch_info.touch_state = TOUCH_STATE::TOUCH_BEGAN;
					break;
				}
				case TOUCH_STATE::TOUCH_BEGAN: {
					touch_info.touch_state = TOUCH_MOVE;
					break;
				}
				case TOUCH_STATE::TOUCH_CANCEL:
				case TOUCH_STATE::TOUCH_ENDED: {
					touch_info.touch_state = TOUCH_STATE::TOUCH_ENDED;
					removed_touch_ids.push_back(touch_id);
					break;
				}
				default: {
					break;
				}
			}
			handled_touches.push_back(touch_info);
		}
		for (const auto& removed_id : removed_touch_ids) {
			if (removed_id == window_unique_touch_id) {
				window_unique_touch_id = ::Const::MAX_UINT32;
			}
			const auto& handling_touch = handling_touches.get(removed_id);
			for (const auto& e : handling_touch.entities) {
				handling_entities.erase(e.id());
			}
			handling_touches.erase(removed_id);
			id_generator.release_id(removed_id);
		}
	}

	void discard_touches_entities(const std::vector<Handle>& discard_entities) {
		for (const Handle& e : discard_entities) {
			if (!handling_entities.has(e.id()))
				continue;
			for (auto& touch_info : handling_touches.modify_values()) {
				for (int i = 0; i < touch_info.entities.size(); i++) {
					if (touch_info.entities[i].id() == e.id()) {
						touch_info.entities[i] = touch_info.entities.back();
						touch_info.entities.pop_back();
						i--;
					}
				}
			}
			handling_entities.erase(e.id());
		}
	}

	const std::vector<Handled_Touches_Information>& get_handled_touches() {
		return handled_touches;
	}

} // namespace Engine