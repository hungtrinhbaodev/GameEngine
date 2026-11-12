#pragma once
#include <engine/engine.h>
#include <functional>
#include <string>

/**
 * @Note: editor-side scene file, a stand-in for ui_api_need.txt item 31 (Engine::save_scene / load_scene).
 * Same JSON shape as the spec ({"version": 1, "root": NODE}); when the engine version lands, the editor
 * switches to it and this file goes away.
 * The engine cannot tell a node's type yet (31b get_node_type), so the caller passes it in.
 */
namespace Scene_Io {

	enum class Kind { NODE, LAYOUT, IMAGE, LABEL };

	const char* kind_name(Kind kind);

	using Kind_Of = std::function<Kind(const Handle&)>;
	// called for every node created by from_json (the editor registers its type, touch handler, ...).
	using On_Created = std::function<void(const Handle&, Kind)>;

	// the children of `root` (not root itself) as a scene document.
	std::string to_json(const Handle& root, const Kind_Of& kind_of, const std::function<bool(const Handle&)>& skip);

	// builds the document's nodes as children of `parent`. Returns false (and builds nothing) on a bad file.
	bool from_json(const std::string& text, const Handle& parent, const On_Created& on_created, std::string& error);

	// parses without building anything (check a file before clearing the current scene).
	bool is_valid_document(const std::string& text, std::string& error);

	// one node with its subtree (duplicate / copy & paste).
	std::string node_to_string(const Handle& e, const Kind_Of& kind_of, const std::function<bool(const Handle&)>& skip);
	Handle node_from_string(const std::string& text, const Handle& parent, const On_Created& on_created);

	bool save_file(const std::string& path, const std::string& text);
	bool load_file(const std::string& path, std::string& text);

} // namespace Scene_Io
