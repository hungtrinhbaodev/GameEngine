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
	glm::vec3 position = {
		Math::random_float(-2.5f, 2.5f), Math::random_float(-2.f, 2.f), Math::random_float(-2.f, 2.f)
	};
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
 * 2D draw tests, each type only has create, remove and update everything at random.
 * Label:     L create, K remove, J update all.
 * Texture:   1 create, 2 remove, 3 update all.
 * Rectangle: 7 create, 8 remove, 9 update all.
 * Triangle:  4 create, 5 remove, 6 update all.
 */
std::vector<Handle> labels{};

std::vector<Handle> textures{};

std::vector<Handle> rectangles{};

std::vector<Handle> triangles{};

std::vector<std::string> label_texts = {
	"Hello!", "Frame rate: 60", "toi la hung!\nhaha\n12323-~yy", "", "A", "Line 1\nLine 2\nLine 3",
};

std::vector<glm::vec2> anchors_2D = {
	{0.f, 0.f}, {0.5f, 0.5f}, {1.f, 1.f}, {0.f, 1.f}, {1.f, 0.f},
};

std::vector<std::string> texture_paths = {
	"res/texture/texture1.png", "res/texture/texture2.png", "res/texture/texture3.png",
	"res/texture/texture4.png", "res/AddonIcon5.png",		"res/AddonIcon7.png",
};

std::vector<Geometry::Texture_Rect_2D> texture_rects = {
	{0.f, 0.f, 1.f, 1.f},
	{0.f, 0.f, 0.5f, 1.f},
	{0.f, 0.f, 1.f, 0.5f},
	{0.25f, 0.25f, 0.5f, 0.5f},
};

template <typename T> const T& random_item(const std::vector<T>& values) {
	return values[Math::random_int(0, values.size() - 1)];
}

int pick_index(const std::vector<Handle>& handles) {
	if (handles.size() <= 0)
		return -1;
	return Math::random_int(0, handles.size() - 1);
}

void remove_random_draw(std::vector<Handle>& handles) {
	int index = pick_index(handles);
	if (index < 0)
		return;
	Vulkan::remove_draw(handles[index]);
	handles[index] = handles.back();
	handles.pop_back();
}

glm::vec2 random_position_2D() {
	glm::vec2 window_size = Vulkan::Utils::get_window_size(Vulkan::device);
	return {Math::random_float(0.f, window_size.x), Math::random_float(0.f, window_size.y)};
}

glm::vec3 random_color() {
	return {Math::random_float(), Math::random_float(), Math::random_float()};
}

/**
 * @Note: mostly visible so most updates can still be seen on screen.
 */
bool random_visible() {
	return Math::random_int(0, 4) > 0;
}

/**
 * Label 2D test.
 */
void gen_label() {
	Vulkan::Label_2D_Description label_info{};
	label_info.text = random_item(label_texts);
	label_info.font_size = Math::random_int(12, 32);
	label_info.position = random_position_2D();
	label_info.color = random_color();
	label_info.align = Math::random_int(0, 2);
	labels.push_back(Vulkan::create_label(label_info));
	Log::info("[Label test] create, number labels:", labels.size(), label_info.text);
}

void remove_label() {
	remove_random_draw(labels);
	Log::info("[Label test] remove, number labels:", labels.size());
}

void update_label_everything() {
	int index = pick_index(labels);
	if (index < 0)
		return;
	Vulkan::Label_2D& label = Vulkan::modify_label(labels[index]);
	label.set_text(random_item(label_texts));
	label.set_font_size(Math::random_int(10, 40));
	label.set_align(Math::random_int(0, 2));
	label.set_position(random_position_2D());
	label.set_rotation(Math::random_float(0.f, 360.f));
	label.set_anchor(random_item(anchors_2D));
	label.set_color(random_color());
	float scale = Math::random_float(0.5f, 2.f);
	label.set_scale({scale, scale});
	label.set_visible(random_visible());
	label.set_draw_index(Math::random_int(1, 1000));
}

/**
 * Texture 2D test.
 */
void gen_texture() {
	Vulkan::Texture_2D_Description texture_info{};
	texture_info.path = random_item(texture_paths);
	texture_info.position = random_position_2D();
	texture_info.color = {1.f, 1.f, 1.f};
	textures.push_back(Vulkan::create_texture(texture_info));
	Log::info("[Texture test] create, number textures:", textures.size(), texture_info.path);
}

void remove_texture() {
	remove_random_draw(textures);
	Log::info("[Texture test] remove, number textures:", textures.size());
}

void update_texture_everything() {
	int index = pick_index(textures);
	if (index < 0)
		return;
	Vulkan::Texture_2D& texture = Vulkan::modify_texture(textures[index]);
	texture.set_texture_path(random_item(texture_paths));
	texture.set_texture_rect(random_item(texture_rects));
	texture.set_position(random_position_2D());
	texture.set_rotation(Math::random_float(0.f, 360.f));
	texture.set_anchor(random_item(anchors_2D));
	texture.set_color(random_color());
	float scale = Math::random_float(0.5f, 2.f);
	texture.set_scale({scale, scale});
	texture.set_visible(random_visible());
	texture.set_draw_index(Math::random_int(1, 1000));
}

/**
 * Rectangle 2D test.
 */
void gen_rectangle() {
	Vulkan::Rectangle_2D_Description rectangle_info{};
	rectangle_info.width = Math::random_float(50.f, 300.f);
	rectangle_info.height = Math::random_float(50.f, 300.f);
	rectangle_info.anchor = {0.5f, 0.5f};
	rectangle_info.position = random_position_2D();
	rectangle_info.color = random_color();
	rectangles.push_back(Vulkan::create_rectangle(rectangle_info));
	Log::info("[Rectangle test] create, number rectangles:", rectangles.size());
}

void remove_rectangle() {
	remove_random_draw(rectangles);
	Log::info("[Rectangle test] remove, number rectangles:", rectangles.size());
}

void update_rectangle_everything() {
	int index = pick_index(rectangles);
	if (index < 0)
		return;
	Vulkan::Rectangle_2D& rectangle = Vulkan::modify_rectangle(rectangles[index]);
	rectangle.set_width(Math::random_float(50.f, 300.f));
	rectangle.set_height(Math::random_float(50.f, 300.f));
	rectangle.set_position(random_position_2D());
	rectangle.set_rotation(Math::random_float(0.f, 360.f));
	rectangle.set_anchor(random_item(anchors_2D));
	rectangle.set_color(random_color());
	rectangle.set_visible(random_visible());
	rectangle.set_draw_index(Math::random_int(1, 1000));
}

/**
 * Triangle 2D test.
 */
glm::vec2 random_triangle_point(const glm::vec2& center) {
	return center + glm::vec2{Math::random_float(-150.f, 150.f), Math::random_float(-150.f, 150.f)};
}

/**
 * @Note: the engine only accepts clockwise triangles (Math::is_valid_triangle_with_clockwise),
 * so swap the last two points when the random winding is the other way, and retry flat triangles.
 */
std::array<glm::vec2, 3> random_triangle_points() {
	glm::vec2 center = random_position_2D();
	std::array<glm::vec2, 3> points{};
	do {
		points = {random_triangle_point(center), random_triangle_point(center), random_triangle_point(center)};
		if (!Math::is_valid_triangle_with_clockwise(points[0], points[1], points[2])) {
			std::swap(points[1], points[2]);
		}
	} while (!Math::is_valid_triangle_with_clockwise(points[0], points[1], points[2]));
	return points;
}

void gen_triangle() {
	std::array<glm::vec2, 3> points = random_triangle_points();
	Vulkan::Triangle_2D_Description triangle_info{};
	triangle_info.first = points[0];
	triangle_info.second = points[1];
	triangle_info.third = points[2];
	triangle_info.color = random_color();
	triangles.push_back(Vulkan::create_triangle(triangle_info));
	Log::info("[Triangle test] create, number triangles:", triangles.size());
}

void remove_triangle() {
	remove_random_draw(triangles);
	Log::info("[Triangle test] remove, number triangles:", triangles.size());
}

void update_triangle_everything() {
	int index = pick_index(triangles);
	if (index < 0)
		return;
	std::array<glm::vec2, 3> points = random_triangle_points();
	Vulkan::Triangle_2D& triangle = Vulkan::modify_triangle(triangles[index]);
	triangle.set_first_point(points[0]);
	triangle.set_second_point(points[1]);
	triangle.set_third_point(points[2]);
	triangle.set_color(random_color());
	triangle.set_visible(random_visible());
	triangle.set_draw_index(Math::random_int(1, 1000));
}

/**
 * Debug scene (key B): rebuild the 18 objects of debug/draw_batching.json in the same sorted order,
 * so the draw groups and batches come out the same. Positions and colors are fixed to make every run
 * identical. Sorting draws higher draw_index first, so each object gets a decreasing draw_index.
 * Textures and rectangles are added after the json objects.
 * Press N afterwards to print the draw groups and compare with the json.
 */
void clear_test_scene() {
	for (const std::vector<Handle>* handles : {&foxes, &cesiummans, &labels, &textures, &rectangles, &triangles}) {
		for (const Handle& handle : *handles) {
			Vulkan::remove_draw(handle);
		}
	}
	foxes.clear();
	cesiummans.clear();
	labels.clear();
	textures.clear();
	rectangles.clear();
	triangles.clear();
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
			Vulkan::Label_2D_Description label_info{};
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
	// Textures: the first two share a texture (one batch), the third uses another.
	std::vector<std::string> debug_texture_paths = {texture_paths[0], texture_paths[0], texture_paths[1]};
	for (int i = 0; i < (int)debug_texture_paths.size(); i++) {
		Vulkan::Texture_2D_Description texture_info{};
		texture_info.draw_index = draw_index;
		texture_info.path = debug_texture_paths[i];
		texture_info.position = {window_size.x - 250.f, 60.f + i * 200.f};
		textures.push_back(Vulkan::create_texture(texture_info));
		draw_index--;
	}
	// Rectangles: three neighbours in the sorted order (one batch), each with its own size and color.
	for (int i = 0; i < 3; i++) {
		Vulkan::Rectangle_2D_Description rectangle_info{};
		rectangle_info.draw_index = draw_index;
		rectangle_info.width = 120.f + i * 40.f;
		rectangle_info.height = 80.f;
		rectangle_info.anchor = {0.5f, 0.5f};
		rectangle_info.position = {window_size.x * 0.5f + (i - 1) * 200.f, window_size.y - 120.f};
		rectangle_info.color = {0.2f + 0.3f * i, 0.8f - 0.25f * i, 0.4f};
		rectangles.push_back(Vulkan::create_rectangle(rectangle_info));
		draw_index--;
	}
	// Triangles: three neighbours in the sorted order (one batch), above the rectangles.
	for (int i = 0; i < 3; i++) {
		glm::vec2 center = {window_size.x * 0.5f + (i - 1) * 200.f, window_size.y - 260.f};
		Vulkan::Triangle_2D_Description triangle_info{};
		triangle_info.draw_index = draw_index;
		triangle_info.first = center + glm::vec2{-60.f, -50.f};
		triangle_info.second = center + glm::vec2{0.f, 60.f};
		triangle_info.third = center + glm::vec2{60.f, -50.f};
		triangle_info.color = {0.4f, 0.3f + 0.3f * i, 0.9f - 0.3f * i};
		triangles.push_back(Vulkan::create_triangle(triangle_info));
		draw_index--;
	}
	Log::info(
		"[Debug scene] built, labels:", labels.size(), "foxes:", foxes.size(), "cesiummans:", cesiummans.size(),
		"textures:", textures.size(), "rectangles:", rectangles.size(), "triangles:", triangles.size()
	);
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
				case GLFW_KEY_J: {
					update_label_everything();
					break;
				}
				case GLFW_KEY_1: {
					gen_texture();
					break;
				}
				case GLFW_KEY_2: {
					remove_texture();
					break;
				}
				case GLFW_KEY_3: {
					update_texture_everything();
					break;
				}
				case GLFW_KEY_4: {
					gen_triangle();
					break;
				}
				case GLFW_KEY_5: {
					remove_triangle();
					break;
				}
				case GLFW_KEY_6: {
					update_triangle_everything();
					break;
				}
				case GLFW_KEY_7: {
					gen_rectangle();
					break;
				}
				case GLFW_KEY_8: {
					remove_rectangle();
					break;
				}
				case GLFW_KEY_9: {
					update_rectangle_everything();
					break;
				}
				case GLFW_KEY_N: {
					Vulkan::show_debug();
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

	for (int i = 0; i < 3; i++) {
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
