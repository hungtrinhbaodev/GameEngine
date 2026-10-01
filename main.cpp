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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_A 0x41

int main() {
	// {
	// 	// Parser::Gltf_Model model = Parser::parse_gltf_model("res/CesiumMan.gltf", true);
	// 	Parser::Gltf_Model model = Parser::parse_gltf_model("res/cat 7.glb", true);
	// 	auto meshes_transform_by_scene = model.make_meshes_global_transform();
	// 	Log::info("What is meshes_transform_by_scene", meshes_transform_by_scene.size());
	// 	for (auto& [scene_idx, meshes_transform] : meshes_transform_by_scene) {
	// 		for (auto& [mesh_idx, transforms] : meshes_transform) {
	// 			Log::info("What is transform of", scene_idx, mesh_idx, transforms);
	// 		}
	// 	}
	// }

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

	std::vector<glm::vec2> texture_positions{};
	std::vector<std::string> texture_paths{};
	for (int i = 0; i < 15; i++) {
		texture_positions.push_back(
			glm::vec2(
				Math::random_float(0.f, Vulkan::swapchain_extent.width),
				Math::random_float(0.f, Vulkan::swapchain_extent.height)
			)
		);
		texture_paths.push_back(Math::random_float() >= 0.5f ? "res/AddonIcon7.png" : "res/AddonIcon5.png");
	}

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
		for (int i = 0; i < texture_positions.size(); i++) {
			const std::string& path = texture_paths[i];
			Vulkan::API::draw_texture_2D(path, texture_positions[i], {1.f, 1.f}, 0.f, {0.5, 0.5});
		}
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
