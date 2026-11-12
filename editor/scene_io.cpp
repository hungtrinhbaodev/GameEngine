#include <fstream>
#include <json.hpp>
#include <scene_io.h>
#include <sstream>

using json = nlohmann::json;

namespace Scene_Io {

	namespace {
		const std::string DEFAULT_FONT = "res/fonts/anonymous_pro_bold.ttf";

		json vec(glm::vec2 v) {
			return json::array({v.x, v.y});
		}
		json vec(glm::vec3 v) {
			return json::array({v.x, v.y, v.z});
		}
		glm::vec2 vec2_of(const json& j, glm::vec2 fallback) {
			return j.is_array() && j.size() == 2 ? glm::vec2(j[0].get<float>(), j[1].get<float>()) : fallback;
		}
		glm::vec3 vec3_of(const json& j, glm::vec3 fallback) {
			return j.is_array() && j.size() == 3 ? glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>())
												 : fallback;
		}

		Kind kind_from(const std::string& name) {
			if (name == "layout")
				return Kind::LAYOUT;
			if (name == "image")
				return Kind::IMAGE;
			if (name == "label")
				return Kind::LABEL;
			return Kind::NODE;
		}

		json node_to_json(const Handle& e, const Kind_Of& kind_of, const std::function<bool(const Handle&)>& skip) {
			Kind kind = kind_of(e);
			json node;
			node["type"] = kind_name(kind);
			node["name"] = Engine::get_name(e);
			if (Engine::get_tag(e) != 0)
				node["tag"] = Engine::get_tag(e);
			node["position"] = vec(Engine::get_position(e));
			node["anchor"] = vec(Engine::get_anchor(e));
			node["scale"] = vec(Engine::get_scale(e));
			node["rotation"] = Engine::get_rotation(e);
			node["opacity"] = Engine::get_opacity(e);
			node["color"] = vec(Engine::get_color(e));
			node["visible"] = Engine::is_visible(e);
			node["z"] = Engine::get_z_order(e);
			switch (kind) {
				case Kind::LAYOUT: {
					node["size"] = vec(Engine::get_content_size(e));
					node["layout"] = {
						{"background", Engine::is_layout_background_enabled(e)},
						{"background_color", vec(Engine::get_layout_background_color(e))},
						{"radius", Engine::get_layout_border_radius(e)},
						{"border_width", Engine::get_layout_border_width(e)},
						{"border_color", vec(Engine::get_layout_border_color(e))},
						{"clipping", Engine::is_layout_clipping_enabled(e)},
					};
					break;
				}
				case Kind::IMAGE: {
					node["image"] = {{"path", Engine::get_image_path(e)}};
					break;
				}
				case Kind::LABEL: {
					node["label"] = {
						{"text", Engine::get_label_string(e)},
						{"font", Engine::get_label_font_path(e)},
						{"size", Engine::get_label_font_size(e)},
					};
					break;
				}
				default: {
					break;
				}
			}
			json children = json::array();
			for (const auto& child : Engine::get_children(e)) {
				if (!skip(child))
					children.push_back(node_to_json(child, kind_of, skip));
			}
			if (!children.empty())
				node["children"] = children;
			return node;
		}

		Handle node_from_json(const json& node, const Handle& parent, const On_Created& on_created) {
			Kind kind = kind_from(node.value("type", "node"));
			Handle e = Handle::make_invalid_handle();
			switch (kind) {
				case Kind::LAYOUT: {
					e = Engine::create_layout();
					Engine::set_content_size(e, vec2_of(node.value("size", json()), {100.f, 100.f}));
					const json layout = node.value("layout", json::object());
					Engine::set_layout_background_enabled(e, layout.value("background", true));
					Engine::set_layout_background_color(e, vec3_of(layout.value("background_color", json()), {1.f, 1.f, 1.f}));
					Engine::set_layout_border_radius(e, layout.value("radius", 0.f));
					Engine::set_layout_border_width(e, layout.value("border_width", 0.f));
					Engine::set_layout_border_color(e, vec3_of(layout.value("border_color", json()), {0.f, 0.f, 0.f}));
					Engine::set_layout_clipping_enabled(e, layout.value("clipping", false));
					break;
				}
				case Kind::IMAGE: {
					e = Engine::create_image();
					Engine::set_image_path(e, node.value("image", json::object()).value("path", ""));
					break;
				}
				case Kind::LABEL: {
					e = Engine::create_label();
					const json label = node.value("label", json::object());
					Engine::set_label_font_path(e, label.value("font", DEFAULT_FONT));
					Engine::set_label_font_size(e, label.value("size", 24));
					Engine::set_label_string(e, label.value("text", ""));
					break;
				}
				default: {
					e = Engine::create_node();
					break;
				}
			}
			Engine::set_name(e, node.value("name", ""));
			if (node.contains("tag"))
				Engine::set_tag(e, node["tag"].get<int>());
			Engine::set_position(e, vec2_of(node.value("position", json()), {0.f, 0.f}));
			Engine::set_anchor(e, vec2_of(node.value("anchor", json()), {0.5f, 0.5f}));
			Engine::set_scale(e, vec2_of(node.value("scale", json()), {1.f, 1.f}));
			Engine::set_rotation(e, node.value("rotation", 0.f));
			Engine::set_opacity(e, node.value("opacity", 1.f));
			Engine::set_color(e, vec3_of(node.value("color", json()), {1.f, 1.f, 1.f}));
			Engine::set_visible(e, node.value("visible", true));
			Engine::set_z_order(e, node.value("z", 0u));
			Engine::add_child(parent, e);
			if (on_created)
				on_created(e, kind);
			for (const auto& child : node.value("children", json::array()))
				node_from_json(child, e, on_created);
			return e;
		}
	} // namespace

	const char* kind_name(Kind kind) {
		switch (kind) {
			case Kind::LAYOUT:
				return "layout";
			case Kind::IMAGE:
				return "image";
			case Kind::LABEL:
				return "label";
			default:
				return "node";
		}
	}

	std::string to_json(const Handle& root, const Kind_Of& kind_of, const std::function<bool(const Handle&)>& skip) {
		json children = json::array();
		for (const auto& child : Engine::get_children(root)) {
			if (!skip(child))
				children.push_back(node_to_json(child, kind_of, skip));
		}
		json document = {{"version", 1}, {"root", {{"type", "node"}, {"children", children}}}};
		return document.dump(2);
	}

	bool from_json(const std::string& text, const Handle& parent, const On_Created& on_created, std::string& error) {
		json document;
		try {
			document = json::parse(text);
			if (!document.is_object() || !document.contains("root"))
				throw std::runtime_error("missing root object");
		} catch (const std::exception& exception) {
			error = exception.what();
			return false; // nothing built: parse fully before creating any node
		}
		for (const auto& child : document["root"].value("children", json::array()))
			node_from_json(child, parent, on_created);
		return true;
	}

	bool is_valid_document(const std::string& text, std::string& error) {
		try {
			json document = json::parse(text);
			if (!document.is_object() || !document.contains("root"))
				throw std::runtime_error("missing root object");
		} catch (const std::exception& exception) {
			error = exception.what();
			return false;
		}
		return true;
	}

	std::string node_to_string(const Handle& e, const Kind_Of& kind_of, const std::function<bool(const Handle&)>& skip) {
		return node_to_json(e, kind_of, skip).dump();
	}

	Handle node_from_string(const std::string& text, const Handle& parent, const On_Created& on_created) {
		json node;
		try {
			node = json::parse(text);
		} catch (const std::exception&) {
			return Handle::make_invalid_handle();
		}
		return node_from_json(node, parent, on_created);
	}

	bool save_file(const std::string& path, const std::string& text) {
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
			return false;
		file << text;
		return file.good();
	}

	bool load_file(const std::string& path, std::string& text) {
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
			return false;
		std::stringstream buffer;
		buffer << file.rdbuf();
		text = buffer.str();
		return true;
	}

} // namespace Scene_Io
