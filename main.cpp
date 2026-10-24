#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

#include <core.h>
#include <log.h>
#include <math_custom.h>
#include <parser/font_parser.h>
#include <utils.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_utils.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vk_draw.h>

#define VK_A 0x41

std::vector<std::string> fox_animations = {
	"Survey",
	"Walk",
	"Run",
};

std::vector<Handle> foxes{};
std::vector<Handle> cesiummans{};

void gen_fox_3D() {
	int animation_index = Math::random_int(0, 100) % fox_animations.size();
	std::string animation_name = fox_animations[animation_index];
	glm::vec3 position = {Math::random_float(-1.f, 1.f), Math::random_float(-1.f, 1.f), Math::random_float(-1.f, 1.f)};
	Handle fox = Vulkan::create_model("res/Fox_embedded.gltf", "", animation_name, position, {0.01f, 0.01f, 0.01f});
	foxes.push_back(fox);
}

void remove_fox_by_index(int remove_index) {
	Vulkan::remove_draw(foxes[remove_index]);
	foxes[remove_index] = foxes.back();
	foxes.pop_back();
}

void remove_fox() {
	if (foxes.size() <= 0)
		return;
	int remove_index = Math::random_int(0, foxes.size() - 1);
	remove_fox_by_index(remove_index);
}

void update_fox_animation() {
	if (foxes.size() <= 0)
		return;
	int update_index = Math::random_int(0, foxes.size() - 1);
	Vulkan::Model_3D& fox_model = Vulkan::modify_model(foxes[update_index]);
	auto it = std::find(fox_animations.begin(), fox_animations.end(), fox_model.get_animation_name());
	if (it != fox_animations.end()) {
		int index = it - fox_animations.begin();
		index = (index + 1) % fox_animations.size();
		fox_model.set_animation_name(fox_animations[index]);
	}
}

void change_fox_to_cesiumman() {
	if (foxes.size() <= 0)
		return;
	int remove_index = Math::random_int(0, foxes.size() - 1);
	Vulkan::Model_3D_Description model_info = Vulkan::view_model_description(foxes[remove_index]);
	model_info.path = "res/CesiumMan.gltf";
	model_info.animation_name = "0";
	model_info.scale = {1.f, 1.f, 1.f};
	remove_fox_by_index(remove_index);
	cesiummans.push_back(Vulkan::create_model(model_info));
}

/**
 * Label 2D test.
 * L: create label, K: remove label, T: change text, P: change position, R: rotate,
 * A: change anchor, C: change color, Z: change scale, N: change font size, M: change align,
 * V: toggle visible, D: change draw index, X: change everything at once.
 */
std::vector<Handle> labels{};

std::vector<std::string> label_texts = {
	"Hello!", "Frame rate: 60", "toi la hung!\nhaha\n12323-~yy", "", "A", "Line 1\nLine 2\nLine 3",
};

// std::vector<std::string> label_texts = {
// 	"",
// };

std::vector<glm::vec2> label_anchors = {
	{0.f, 0.f}, {0.5f, 0.5f}, {1.f, 1.f}, {0.f, 1.f}, {1.f, 0.f},
};

int pick_label_index() {
	if (labels.size() <= 0)
		return -1;
	return Math::random_int(0, labels.size() - 1);
}

glm::vec2 random_label_position() {
	glm::vec2 window_size = Vulkan::Utils::get_window_size(Vulkan::device);
	return {Math::random_float(0.f, window_size.x), Math::random_float(0.f, window_size.y)};
}

glm::vec3 random_color() {
	return {Math::random_float(), Math::random_float(), Math::random_float()};
}

void gen_label() {
	Vulkan::Label_Description label_info{};
	label_info.text = label_texts[Math::random_int(0, label_texts.size() - 1)];
	label_info.font_size = Math::random_int(12, 32);
	label_info.position = random_label_position();
	label_info.color = random_color();
	label_info.align = Math::random_int(0, 2);
	labels.push_back(Vulkan::create_label(label_info));
	Log::info("[Label test] create, number labels:", labels.size(), label_info.text);
}

void remove_label() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::remove_draw(labels[index]);
	labels[index] = labels.back();
	labels.pop_back();
	Log::info("[Label test] remove, number labels:", labels.size());
}

void change_label_text() {
	int index = pick_label_index();
	if (index < 0)
		return;
	std::string text = label_texts[Math::random_int(0, label_texts.size() - 1)];
	Vulkan::modify_label(labels[index]).set_text(text);
	Log::info("[Label test] text:", text);
}

void change_label_position() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::modify_label(labels[index]).set_position(random_label_position());
}

void rotate_label() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::Label_2D& label = Vulkan::modify_label(labels[index]);
	label.set_rotation(label.get_rotation() + 15.f);
}

void change_label_anchor() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::Label_2D& label = Vulkan::modify_label(labels[index]);
	auto it = std::find(label_anchors.begin(), label_anchors.end(), label.get_anchor());
	int anchor_index = it == label_anchors.end() ? 0 : (int)(it - label_anchors.begin());
	label.set_anchor(label_anchors[(anchor_index + 1) % label_anchors.size()]);
}

void change_label_color() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::modify_label(labels[index]).set_color(random_color());
}

void change_label_scale() {
	int index = pick_label_index();
	if (index < 0)
		return;
	float scale = Math::random_float(0.5f, 2.f);
	Vulkan::modify_label(labels[index]).set_scale({scale, scale});
}

void change_label_font_size() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::modify_label(labels[index]).set_font_size(Math::random_int(10, 40));
}

void change_label_align() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::Label_2D& label = Vulkan::modify_label(labels[index]);
	label.set_align((label.get_align() + 1) % 3);
}

void toggle_label_visible() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::Label_2D& label = Vulkan::modify_label(labels[index]);
	label.set_visible(!label.is_visible());
}

void change_label_draw_index() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::modify_label(labels[index]).set_draw_index(Math::random_int(1, 1000));
}

void change_label_everything() {
	int index = pick_label_index();
	if (index < 0)
		return;
	Vulkan::Label_2D& label = Vulkan::modify_label(labels[index]);
	label.set_text(label_texts[Math::random_int(0, label_texts.size() - 1)]);
	label.set_position(random_label_position());
	label.set_rotation(Math::random_float(0.f, 360.f));
	label.set_anchor(label_anchors[Math::random_int(0, label_anchors.size() - 1)]);
	label.set_color(random_color());
	label.set_font_size(Math::random_int(10, 40));
	label.set_align(Math::random_int(0, 2));
}

/**
 * Debug scene (key B): rebuild the 18 objects of debug/draw_batching.json in the same sorted order,
 * so the draw groups and batches come out the same. Positions and colors are fixed to make every run
 * identical. Sorting draws higher draw_index first, so each object gets a decreasing draw_index.
 * Press N afterwards to print the draw groups and compare with the json.
 */
void clear_test_scene() {
	for (const Handle& fox : foxes) {
		Vulkan::remove_draw(fox);
	}
	for (const Handle& cesiumman : cesiummans) {
		Vulkan::remove_draw(cesiumman);
	}
	for (const Handle& label : labels) {
		Vulkan::remove_draw(label);
	}
	foxes.clear();
	cesiummans.clear();
	labels.clear();
}

void build_debug_scene() {
	enum Debug_Object_Type {
		DEBUG_LABEL,
		DEBUG_FOX,
		DEBUG_CESIUMMAN
	};
	struct Debug_Object {
		Debug_Object_Type type;
		std::string text = "";
	};
	const std::string text_25 = "toi la hung!\nhaha\n12323-~yy";
	const std::string text_18 = "Line 1\nLine 2\nLine 3";
	const std::string text_14 = "Frame rate: 60";
	// Same order as sorted_draws in debug/draw_batching.json. The last label in the json (21 glyphs)
	// is the fps label created by vk_core (draw index 0, so it is always sorted last).
	std::vector<Debug_Object> objects = {
		{DEBUG_LABEL, text_25},
		{DEBUG_LABEL, text_18},
		{DEBUG_FOX},
		{DEBUG_FOX},
		{DEBUG_FOX},
		{DEBUG_CESIUMMAN},
		{DEBUG_LABEL, text_18},
		{DEBUG_LABEL, text_14},
		{DEBUG_CESIUMMAN},
		{DEBUG_LABEL, text_14},
		{DEBUG_LABEL, text_25},
		{DEBUG_LABEL, "A"},
		{DEBUG_LABEL, "Hello!"},
		{DEBUG_LABEL, text_14},
		{DEBUG_LABEL, "A"},
		{DEBUG_CESIUMMAN},
		{DEBUG_FOX},
	};

	clear_test_scene();
	glm::vec2 window_size = Vulkan::Utils::get_window_size(Vulkan::device);
	uint32_t draw_index = 1000;
	int label_count = 0;
	int model_count = 0;
	for (const Debug_Object& object : objects) {
		if (object.type == DEBUG_LABEL) {
			Vulkan::Label_Description label_info{};
			label_info.draw_index = draw_index;
			label_info.text = object.text;
			label_info.font_size = 16;
			label_info.position = {40.f + (label_count % 3) * window_size.x / 3.f, 40.f + (label_count / 3) * 90.f};
			label_info.color = {1.f, 1.f - 0.08f * label_count, 0.2f + 0.07f * label_count};
			label_info.align = 1;
			labels.push_back(Vulkan::create_label(label_info));
			label_count++;
		} else {
			Vulkan::Model_3D_Description model_info{};
			model_info.draw_index = draw_index;
			model_info.position = {-1.5f + model_count * 0.5f, -0.5f, 0.f};
			if (object.type == DEBUG_FOX) {
				model_info.path = "res/Fox_embedded.gltf";
				model_info.animation_name = "Walk";
				model_info.scale = {0.01f, 0.01f, 0.01f};
				foxes.push_back(Vulkan::create_model(model_info));
			} else {
				model_info.path = "res/CesiumMan.gltf";
				model_info.animation_name = "0";
				model_info.scale = {1.f, 1.f, 1.f};
				cesiummans.push_back(Vulkan::create_model(model_info));
			}
			model_count++;
		}
		draw_index--;
	}
	Log::info("[Debug scene] built, labels:", labels.size(), "foxes:", foxes.size(), "cesiummans:", cesiummans.size());
}

void on_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch (action) {
		case GLFW_PRESS: {
			switch (key) {
				case GLFW_KEY_W: {
					gen_fox_3D();
					break;
				}
				case GLFW_KEY_S: {
					remove_fox();
					break;
				}
				case GLFW_KEY_F: {
					update_fox_animation();
					break;
				}
				case GLFW_KEY_G: {
					change_fox_to_cesiumman();
					break;
				}
				case GLFW_KEY_L: {
					gen_label();
					break;
				}
				case GLFW_KEY_K: {
					remove_label();
					break;
				}
				case GLFW_KEY_T: {
					change_label_text();
					break;
				}
				case GLFW_KEY_P: {
					change_label_position();
					break;
				}
				case GLFW_KEY_R: {
					rotate_label();
					break;
				}
				case GLFW_KEY_A: {
					change_label_anchor();
					break;
				}
				case GLFW_KEY_C: {
					change_label_color();
					break;
				}
				case GLFW_KEY_Z: {
					change_label_scale();
					break;
				}
				case GLFW_KEY_N: {
					// change_label_font_size();
					Vulkan::show_debug();
					break;
				}
				case GLFW_KEY_M: {
					change_label_align();
					break;
				}
				case GLFW_KEY_V: {
					toggle_label_visible();
					break;
				}
				case GLFW_KEY_D: {
					change_label_draw_index();
					break;
				}
				case GLFW_KEY_X: {
					change_label_everything();
					break;
				}
				case GLFW_KEY_B: {
					build_debug_scene();
					break;
				}
				default: {
					break;
				}
			}
			break;
		}
		default: {
			break;
		}
	}
}

int main() {

	if (!glfwInit()) {
		throw std::runtime_error("fail to init glfw!");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1200, 720, "game", nullptr, nullptr);

	Vulkan::init_vulkan_core(window);
	glfwSetKeyCallback(window, on_key);

	glm::vec2 window_size = Vulkan::Utils::get_window_size(Vulkan::device);
	std::vector<uint32_t> rectangles{};
	for (int i = 0; i < 0; i++) {
		Vulkan::Draw_Attribute draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
		Vulkan::Rectangle_Attributes rectangle_attributes{
			200, 100, {Math::random_float(0, window_size.x), Math::random_float(0, window_size.y)}
		};
		rectangle_attributes.color = {Math::random_float(), Math::random_float(), Math::random_float()};
		rectangles.push_back(Vulkan::make_rectangle(draw_attributes, rectangle_attributes));
	}

	std::vector<uint32_t> textures{};
	for (int i = 0; i < 0; i++) {
		std::string path = Math::random_float() >= 0.5f ? "res/AddonIcon7.png" : "res/AddonIcon5.png";
		Vulkan::Draw_Attribute draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
		Vulkan::Texture_2D_Attributes texture_attributes{
			path, {Math::random_float(0, window_size.x), Math::random_float(0, window_size.y)}
		};
		textures.push_back(Vulkan::make_texture_2D(draw_attributes, texture_attributes));
	}

	// std::vector<uint32_t> triangles{};
	// Vulkan::Draw_Attribute triangle_draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
	// Vulkan::Triangle_Attribultes triangle_attributes{{200.f, 220.f}, {430.f, 340.f}, {520.f, 120.f}};
	// triangle_attributes.color = {Math::random_float(), Math::random_float(), Math::random_float()};
	// triangles.push_back(Vulkan::make_triangle(triangle_draw_attributes, triangle_attributes));

	for (int i = 0; i < 0; i++) {
		Vulkan::Draw_Attribute font_draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
		Vulkan::Font_2D_Attributes font_attributes{};
		font_attributes.path = "res/fonts/default.otf";
		font_attributes.text = "toi la hung!\nhaha\n12323-~yy";
		font_attributes.color = {Math::random_float(), Math::random_float(), Math::random_float()};
		font_attributes.align = 1;
		font_attributes.position = {Math::random_float(0, window_size.x), Math::random_float(0, window_size.y)};
		font_attributes.font_size = Math::random_float(12, 18);
		Vulkan::make_font_2D(font_draw_attributes, font_attributes);
	}

	for (int i = 0; i < 1; i++) {
		gen_fox_3D();
	}

	double start = Utils::now();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		double current = Utils::now();
		float dt = (float)(current - start);
		dt = std::min(dt, 0.1f);
		start = current;
		Vulkan::start_frame(dt);
		Vulkan::draw_frame(dt);
	}

	Vulkan::destroy_vulkan();

	glfwDestroyWindow(window);

	glfwTerminate();

#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif

	return 0;
}
