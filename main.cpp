#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

#include <core.h>
#include <log.h>
#include <math_custom.h>
#include <parser/gltf_parser.h>
#include <utils.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_texture_array.h>
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

	std::vector<glm::vec3> model_positions{};
	for (int i = 0; i < 1; i++) {
		model_positions.push_back(glm::vec3(0.f, 0.f, 0.f));
	}

	glm::vec2 window_size = Vulkan::Utils::get_window_size();
	std::vector<uint32_t> rectangles{};
	for (int i = 0; i < 1000; i++) {
		Vulkan::Draw_2D_Attribute draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
		Vulkan::Rectangle_Attributes rectangle_attributes{
			200, 100, {Math::random_float(0, window_size.x), Math::random_float(0, window_size.y)}
		};
		rectangle_attributes.color = {Math::random_float(), Math::random_float(), Math::random_float()};
		rectangles.push_back(Vulkan::Draw_2D::make_rectange(draw_attributes, rectangle_attributes));
	}

	std::vector<uint32_t> textures{};
	for (int i = 0; i < 1000; i++) {
		std::string path = Math::random_float() >= 0.5f ? "res/AddonIcon7.png" : "res/AddonIcon5.png";
		Vulkan::Draw_2D_Attribute draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
		Vulkan::Texture_2D_Attributes texture_attributes{
			path, {Math::random_float(0, window_size.x), Math::random_float(0, window_size.y)}
		};
		textures.push_back(Vulkan::Draw_2D::make_texture_2D(draw_attributes, texture_attributes));
	}

	std::vector<uint32_t> triangles{};
	Vulkan::Draw_2D_Attribute triangle_draw_attributes{(uint32_t)Math::random_int(1, 1000), true};
	Vulkan::Triangle_Attribultes triangle_attributes{
		{220.f, 100.f},
		{400.f, 160.f},
		{320.f, 110.f},
		{Math::random_float(), Math::random_float(), Math::random_float()}
	};
	triangles.push_back(Vulkan::Draw_2D::make_triangle(triangle_draw_attributes, triangle_attributes));
	triangle_draw_attributes = {(uint32_t)Math::random_int(1, 1000), true};
	triangle_attributes = {
		{100.f, 100.f},
		{130.f, 140.f},
		{155.f, 120.f},
		{Math::random_float(), Math::random_float(), Math::random_float()}
	};
	triangles.push_back(Vulkan::Draw_2D::make_triangle(triangle_draw_attributes, triangle_attributes));
	triangle_draw_attributes = {(uint32_t)Math::random_int(1, 1000), true};
	triangle_attributes = {
		{300.f, 450.f},
		{500.f, 600.f},
		{580.f, 120.f},
		{Math::random_float(), Math::random_float(), Math::random_float()}
	};
	triangles.push_back(Vulkan::Draw_2D::make_triangle(triangle_draw_attributes, triangle_attributes));

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Set up all component when start frame (reset frame of ring buffer, ...).
		Vulkan::Process::start_frame();

		// TODO: logic of all component will be place here in future.
		// Vulkan::API::draw_triangle_2D({220.f, 100.f}, {400.f, 160.f}, {320.f, 110.f}, {1.f, 0.f, 0.f});
		// Vulkan::API::draw_triangle_2D({100.f, 100.f}, {130.f, 140.f}, {155.f, 120.f}, {1.f, 0.f, 0.f});
		// Vulkan::API::draw_rectangle_2D(100.f, 200.f, 200.f, 100.f, {0.f, 1.f, 0.f}, 30.f, {0.2f, 0.5f});
		// Vulkan::API::draw_triangle_2D({300.f, 450.f}, {500.f, 600.f}, {580.f, 120.f}, {1.f, 0.f, 0.f});
		// Vulkan::API::draw_rectangle_2D(150.f, 250.f, 250.f, 300.f, {0.f, 0.f, 1.f});

		// Vulkan::API::draw_rectangle_2D(320.f, 300.f, 121.f, 126.f, {0.f, 1.f, 0.f}, 0.f, {0.f, 0.f});
		// for (int i = 0; i < texture_positions.size(); i++) {
		// 	const std::string& path = texture_paths[i];
		// 	Vulkan::API::draw_rectangle_2D(
		// 		texture_positions[i].x, texture_positions[i].y, 250.f, 300.f, {0.f, 0.f, 1.f}
		// 	);
		// 	// Vulkan::API::draw_texture_2D(path, texture_positions[i], {1.f, 1.f}, 0.f, {0.5, 0.5});
		// }
		// for (const glm::vec3& position : model_positions) {
		// 	Vulkan::API::draw_model_3D("res/CesiumMan.gltf", {-0.7f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {0.f, 0.f, 0.f});
		// 	Vulkan::API::draw_model_3D("res/cat 7.glb", {0.7f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {0.f, 0.f, 0.f});
		// }

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
