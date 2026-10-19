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

int main() {

	if (!glfwInit()) {
		throw std::runtime_error("fail to init glfw!");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1200, 720, "game", nullptr, nullptr);

	Vulkan::init_vulkan_core(window);

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
	// for (int i = 0; i < 1; i++) {
	// 	// model_attributes.path = "res/Fox.glb";
	// 	// model_attributes.path = "res/cat 7.glb";
	// 	// model_attributes.path = "res/CesiumMan.gltf";
	// 	// model_attributes.position = {0, 0, 0};
	// 	Vulkan::Draw_Attribute model_draw_attributes{1000, true};
	// 	Vulkan::Model_3D_Attributes model_attributes{};
	// 	model_attributes.path = "res/MetalRoughSpheresNoTextures.glb";
	// 	model_attributes.position = {i * -0.9, i * -0.9, i * 0.45};
	// 	model_attributes.scale = {300.f, 300.f, 300.f};
	// 	Vulkan::make_model_3D(model_draw_attributes, model_attributes);
	// }

	for (int i = 0; i < 300; i++) {
		// model_attributes.path = "res/Fox.glb";
		// model_attributes.path = "res/cat 7.glb";
		Vulkan::Draw_Attribute model_draw_attributes{1000, true};
		Vulkan::Model_3D_Attributes model_attributes{};
		model_attributes.path = "res/CesiumMan.gltf";
		model_attributes.position = {
			Math::random_float(-5.f, 5.f), Math::random_float(-5.f, 3.f), Math::random_float(-2.f, 2.f)
		};
		Vulkan::make_model_3D(model_draw_attributes, model_attributes);
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		Vulkan::start_frame();
		/**
		 * TODO: logic of all component will be place here in future.
		 * */

		Vulkan::draw_frame();
	}

	Vulkan::destroy_vulkan();

	glfwDestroyWindow(window);

	glfwTerminate();

#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif

	return 0;
}
