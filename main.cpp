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
#include <vulkan/vk_draw_2D.h>

#define VK_A 0x41

int main() {

	if (!glfwInit()) {
		throw std::runtime_error("fail to init glfw!");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1200, 720, "game", nullptr, nullptr);

	Vulkan::Init::init_vulkan_core(window, Core::global_thread_pool, Core::global_scheduler);

	glm::vec2 window_size = Vulkan::Utils::get_window_size();
	std::vector<uint32_t> rectangles{};
	for (int i = 0; i < 5; i++) {
		Vulkan::Draw_2D_Attribute draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
		Vulkan::Rectangle_Attributes rectangle_attributes{
			200, 100, {Math::random_float(0, window_size.x), Math::random_float(0, window_size.y)}
		};
		rectangle_attributes.color = {Math::random_float(), Math::random_float(), Math::random_float()};
		rectangles.push_back(Vulkan::Draw_2D::make_rectange(draw_attributes, rectangle_attributes));
	}

	std::vector<uint32_t> textures{};
	for (int i = 0; i < 5; i++) {
		std::string path = Math::random_float() >= 0.5f ? "res/AddonIcon7.png" : "res/AddonIcon5.png";
		Vulkan::Draw_2D_Attribute draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
		Vulkan::Texture_2D_Attributes texture_attributes{
			path, {Math::random_float(0, window_size.x), Math::random_float(0, window_size.y)}
		};
		textures.push_back(Vulkan::Draw_2D::make_texture_2D(draw_attributes, texture_attributes));
	}

	Vulkan::Draw_2D_Attribute font_draw_attributes{1000, true};
	Vulkan::Font_2D_Attributes font_attributes{};
	font_attributes.path = "res/fonts/default.otf";
	font_attributes.text = "toi la hung!\nhaha\n12323-~yy";
	font_attributes.color = {Math::random_float(), Math::random_float(), Math::random_float()};
	font_attributes.align = 2;
	font_attributes.position = {0, 0};
	font_attributes.scale = {1.f, 1.f};
	font_attributes.rotation = 0;
	font_attributes.font_size = 12;
	Vulkan::Draw_2D::make_font_2D(font_draw_attributes, font_attributes);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Set up all component when start frame (reset frame of ring buffer, ...).
		Vulkan::Process::start_frame();

		// TODO: logic of all component will be place here in future.

		// Draw all information of this current frame.
		Vulkan::Process::draw_frame();

		// Reset all cache or work need to using in this frame.
		Vulkan::Process::end_frame();
	}

	Vulkan::Destroy::destroy_vulkan();

	glfwDestroyWindow(window);

	glfwTerminate();

#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif

	return 0;
}
