#pragma once
#include <handle.h>

namespace Engine {

	World_Transform_2D get_relative_world_transform(const Handle& e);

	std::vector<Handle> get_renderer_handles(const Handle& e);

	void update_image_renderer(const Handle& e);

	void update_label_renderer(const Handle& e);

	void update_scroll_layout_renderer(const Handle& e);

	void handle_enities_touched(const std::vector<Handle>& dirty_entities);

	void handle_entites_scrolling(const std::vector<Handle>& scrolling_entities);

	void handle_dirty_z_order(const std::vector<Handle>& dirty_entities);

	void handle_update_tree_draw_order(const std::vector<Handle>& dirty_entities);

	void handle_dirty_renderer(const std::vector<Handle>& dirty_entities);

	void handle_dirty_relative_size(const std::vector<Handle>& dirty_entities);

	void handle_dirty_relative_position(const std::vector<Handle>& dirty_entities);

	void handle_dirty_transform(const std::vector<Handle>& dirty_entities);

	void handle_dirty_clipping(const std::vector<Handle>& dirty_entities);

	void handle_removed_children(const std::vector<Handle>& removed_children);

} // namespace Engine