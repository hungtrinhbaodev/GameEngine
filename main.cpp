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

std::vector<uint32_t> foxes;

std::vector<uint32_t> cesiummans;

std::vector<Vulkan::Model_3D_Attributes> cesiumman_attributes;

std::vector<Vulkan::Model_3D_Attributes> fox_attributes;

std::vector<std::string> fox_animations = {
	"Survey",
	"Walk",
	"Run",
};

void gen_fox_3D() {
	Vulkan::Draw_Attribute model_draw_attributes{1000, true};
	Vulkan::Model_3D_Attributes model_attributes{};
	model_attributes.path = "res/Fox_embedded.gltf";
	int animation_index = Math::random_int(0, 100) % fox_animations.size();
	model_attributes.animation_name = fox_animations[animation_index];
	model_attributes.position = {
		Math::random_float(-1.f, 1.f), Math::random_float(-1.f, 1.f), Math::random_float(-1.f, 1.f)
	};
	model_attributes.scale = {0.01f, 0.01f, 0.01f};
	foxes.push_back(Vulkan::make_model_3D(model_draw_attributes, model_attributes));
	fox_attributes.push_back(model_attributes);
}

void remove_fox_by_index(int remove_index) {
	Vulkan::remove_model_3D(foxes[remove_index]);
	foxes[remove_index] = foxes.back();
	foxes.pop_back();
	fox_attributes[remove_index] = fox_attributes.back();
	fox_attributes.pop_back();
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
	auto& fox_attribute = fox_attributes[update_index];
	auto it = std::find(fox_animations.begin(), fox_animations.end(), fox_attribute.animation_name);
	if (it != fox_animations.end()) {
		int index = it - fox_animations.begin();
		index = (index + 1) % fox_animations.size();
		fox_attribute.animation_name = fox_animations[index];
	}
	Vulkan::update_model_3D(foxes[update_index], fox_attribute);
}

void change_fox_to_cesiumman() {
	if (foxes.size() <= 0)
		return;
	int remove_index = Math::random_int(0, foxes.size() - 1);
	auto fox_attribute = fox_attributes[remove_index];
	fox_attribute.path = "res/CesiumMan.gltf";
	fox_attribute.animation_name = "0";
	fox_attribute.scale = {1.f, 1.f, 1.f};
	remove_fox_by_index(remove_index);
	cesiummans.push_back(Vulkan::make_model_3D({1000, true}, fox_attribute));
	cesiumman_attributes.push_back(fox_attribute);
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

	// Vulkan::Draw_Attribute model_draw_attributes{1000, true};
	// Vulkan::Model_3D_Attributes model_attributes{};
	// model_attributes.position = {-0.8f, 0.2f, 0.f};
	// model_attributes.path = "res/CesiumMan.gltf";
	// Vulkan::make_model_3D(model_draw_attributes, model_attributes);
	// for (int i = 0; i < 3; i++) {
	// 	// model_attributes.path = "res/Fox.glb";
	// 	// model_attributes.path = "res/cat 7.glb";
	// 	// model_attributes.path = "res/CesiumMan.gltf";
	// 	// model_attributes.position = {0, 0, 0};
	// 	Vulkan::Draw_Attribute model_draw_attributes{1000, true};
	// 	Vulkan::Model_3D_Attributes model_attributes{};
	// 	model_attributes.path = "res/MetalRoughSpheresNoTextures.glb";
	// 	model_attributes.position = {-0.9, -0.9, i * 1.8 - 1.8};
	// 	model_attributes.scale = {300.f, 300.f, 300.f};
	// 	Vulkan::make_model_3D(model_draw_attributes, model_attributes);
	// }
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
