#include <consts.h>
#include <engine/components.h>
#include <engine/engine.h>
#include <engine/input_glfw.h>
#include <engine/structs.h>
#include <engine/touch_handler.h>
#include <engine/utils.h>
#include <glm/glm.hpp>
#include <id_generator.h>
#include <log.h>
#include <sparse_set.h>

namespace Engine {

	Handle_Generator handle_generator;

	Sparse_Set<Handled_Touches_Information> handling_touches;

	std::vector<Handled_Touches_Information> handled_touches;

	uint32_t window_unique_touch_id = ::Const::MAX_UINT32;

	uint32_t spawn_touch(const glm::vec2& touch_world_position, int button, int mods) {
		Handle handle = handle_generator.gen_handle();
		Handled_Touches_Information touch_info{};
		touch_info.id = handle;
		touch_info.touch_state = TOUCH_STATE::TOUCH_INIT;
		touch_info.touch_world_position = touch_world_position;
		touch_info.touch_type = glfw_to_mouse_button(button);
		touch_info.touch_mods = glfw_to_key_mods(mods);
		handling_touches.insert(handle.id(), touch_info);
		return handle.id();
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
					window_unique_touch_id = spawn_touch(world_pos, button, mods);
				} else if (action == GLFW_RELEASE) {
					/**
					 * @Note maybe the touch canceled because of
					 * touch rate or outside screen!
					 */
					if (window_unique_touch_id == ::Const::MAX_UINT32) {
						break;
					}
					auto& touch_info = handling_touches.modify(window_unique_touch_id);
					switch (touch_info.touch_state) {
						case TOUCH_STATE::TOUCH_INIT: {
							touch_info.touch_state = TOUCH_STATE::TOUCH_PENDING_RELEASE;
							break;
						}
						default: {
							release_touch(window_unique_touch_id, world_pos);
							break;
						}
					}
				}
				break;
			}
			default: {
				break;
			}
		}
	}

	void on_cursor_touch_position(GLFWwindow* window, double x, double y) {
		if (!handling_touches.has(window_unique_touch_id))
			return;
		auto& touch_info = handling_touches.modify(window_unique_touch_id);
		glm::vec2 screen_size = get_window_size();
		glm::vec2 cur_touch{(float)x, screen_size.y - (float)y};
		switch (touch_info.touch_state) {
			case TOUCH_STATE::TOUCH_BEGAN:
			case TOUCH_STATE::TOUCH_MOVE: {
				touch_info.touch_world_position = cur_touch;
				break;
			}
			default: {
				break;
			}
		}
	}

	void init_touch_handler(GLFWwindow* window) {
		glfwSetMouseButtonCallback(window, on_mouse_button);
	}

	void handle_touches_logic() {
		handled_touches.clear();
		std::vector<uint32_t> removed_touch_ids;
		for (const auto& touch_id : handling_touches.keys()) {
			auto& touch_info = handling_touches.modify(touch_id);
			switch (touch_info.touch_state) {
				case TOUCH_STATE::TOUCH_INIT: {
					touch_info.touch_state = TOUCH_STATE::TOUCH_BEGAN;
					break;
				}
				case TOUCH_STATE::TOUCH_PENDING_RELEASE: {
					touch_info.touch_state = TOUCH_STATE::TOUCH_BEGAN;
					touch_info.remove_next_frame = false;
					break;
				}
				case TOUCH_STATE::TOUCH_BEGAN: {
					if (touch_info.remove_next_frame) {
						touch_info.touch_state = TOUCH_STATE::TOUCH_ENDED;
					} else {
						touch_info.touch_state = TOUCH_MOVE;
					}
					touch_info.remove_next_frame = false;
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
			const auto& touch_info = handling_touches.get(removed_id);
			handle_generator.release_handle(touch_info.id);
			handling_touches.erase(removed_id);
		}
	}

	const std::vector<Handled_Touches_Information>& get_handled_touches() {
		return handled_touches;
	}

} // namespace Engine