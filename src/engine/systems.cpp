#include <engine/components.h>
#include <engine/engine.h>
#include <engine/systems.h>
#include <files.h>
#include <math_custom.h>
#include <vulkan/draws/vk_draw_font_2D.h>
#include <vulkan/vk_draw.h>

namespace Engine {

	std::vector<Handle> get_renderer_handles(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Description, Renderer_Handles>(e.id())) {
			return {};
		}
		const auto& [description] = registry.read<Description>(e.id());
		const auto& [handles] = registry.read<Renderer_Handles>(e.id());
		switch (description.value) {
			case DESCRIPTION_TYPE::IMAGE_2D: {
				const auto& [nine_scale] = registry.read<Nine_Scale_Information>(e.id());
				if (nine_scale.enabled) {
					const auto& [nine_scale_handles] = registry.read<Nine_Scale_Renderer_Handles>(e.id());
					return nine_scale_handles.value;
				}
				return {handles.value[0]};
			}
			case DESCRIPTION_TYPE::LAYOUT_2D:
			case DESCRIPTION_TYPE::LABEL_2D: {
				return {handles.value[0]};
			}
			default: {
				return {};
			}
		}
	}

	/**
	 * @Note: with layout we need to take the anchor
	 * from (0, 0) to easy to handle ui but still
	 * world with effect need center anchor!
	 */
	World_Transform_2D get_relative_world_transform(const Handle& e) {
		Registry& registry = get_registry();
		if (registry.has_all<Description, World_Transform_2D>(e.id())) {
			const auto& [description, world_transform] = registry.read<Description, World_Transform_2D>(e.id());
			switch (description.value) {
				case DESCRIPTION_TYPE::LAYOUT_2D: {
					auto layout_world_transform = world_transform;
					const auto& [c_size] = registry.read<Size>(e.id());
					auto delta_anchor = glm::vec2(0.f, 0.f) - layout_world_transform.anchor;
					auto new_origin = delta_anchor * c_size.value;
					layout_world_transform.position +=
						Math::rotate(layout_world_transform.rotation, new_origin * layout_world_transform.scale);
					return layout_world_transform;
				}
				default: {
					return world_transform;
				}
			}
		}
		return {};
	}

	void update_world_transform(const Handle& e_parent, const Handle& e_child) {
		Registry& registry = get_registry();
		const auto& [child_transform] = registry.read<Transform_2D>(e_child.id());
		auto relative_world_transform = get_relative_world_transform(e_parent);
		registry.add(e_child.id(), std::move(relative_world_transform.transform(child_transform)));
	}

	Handle get_render_clipping_handle(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has<Description>(e.id())) {
			return Handle::make_invalid_handle();
		}
		const auto& [description] = registry.read<Description>(e.id());
		switch (description.value) {
			case DESCRIPTION_TYPE::LAYOUT_2D: {
				const auto& [handles] = registry.read<Renderer_Handles>(e.id());
				return handles.value[1];
			}
			default: {
				return Handle::make_invalid_handle();
			}
		}
	}

	void add_clipping_ancestor_to_renderer(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has<Description>(e.id())) {
			return;
		}
		const auto& [clipping_ancestor] = registry.read<Clipping_Ancestor_Information>(e.id());
		auto renderer_handles = get_renderer_handles(e);
		for (const auto& renderer_handle : renderer_handles) {
			auto& draw_attributes = Vulkan::modify_draw_attributes(renderer_handle);
			draw_attributes.set_number_clipping(clipping_ancestor.number_clipping);
			draw_attributes.set_clipping_ancestors(clipping_ancestor.clipping_ancesors);
		}
	}

	void update_layout_renderer(const Handle& e) {
		Registry& registry = get_registry();
		const auto& [layout_renderer] = registry.read<Layout_Renderer_Information>(e.id());
		auto [c_handles] = registry.get<Renderer_Handles>(e.id());
		auto& handles = c_handles.value;
		if (handles.size() <= 0) {
			handles.resize(2, Handle::make_invalid_handle());
			Vulkan::Rectangle_2D_Description description{};
			description.is_visible = false;
			handles[0] = Vulkan::create_rectangle(description);
			handles[1] = Vulkan::create_clipping({});
		}
		auto& rect = Vulkan::modify_rectangle(handles[0]);
		rect.set_visible(layout_renderer.enabled);
		if (layout_renderer.enabled) {
			rect.set_border_color(layout_renderer.border_color);
			rect.set_border_radius(layout_renderer.border_radius);
			rect.set_border_width(layout_renderer.border_width);
			rect.set_color(layout_renderer.background_color);
		}
		auto [clipping_renderer] = registry.get<Clipping_Information>(e.id());
		if (clipping_renderer.enabled) {
			auto& clipping = Vulkan::modify_clipping(handles[1]);
			clipping.set_border_radius(layout_renderer.border_radius);
			clipping.set_border_width(layout_renderer.border_width);
		}
	}

	void update_image_renderer(const Handle& e) {
		Registry& registry = get_registry();
		const auto& [image_renderer, nine_scale] =
			registry.read<Image_Renderer_Information, Nine_Scale_Information>(e.id());
		auto [renderer_handles, nine_scale_renderer_handles, inner_size, c_size] =
			registry.get<Renderer_Handles, Nine_Scale_Renderer_Handles, Inner_Renderer_Size, Size>(e.id());
		auto& nine_scale_handles = nine_scale_renderer_handles.value;
		auto& handles = renderer_handles.value;
		set_dirty(e, true, DIRTY_TRANSFORM);
		if (handles.size() <= 0) {
			Vulkan::Texture_2D_Description description{};
			description.path = Vulkan::get_default_image_path();
			description.is_visible = false;
			handles.push_back(Vulkan::create_texture(description));
		}
		auto& texture = Vulkan::modify_texture(handles[0]);
		if (!Files::is_file_exists(::Utils::get_root_path() + image_renderer.image_path)) {
			texture.set_texture_path(Vulkan::get_default_image_path());
			if (nine_scale.enabled) {
				for (const auto& handle : nine_scale_handles) {
					auto& texture = Vulkan::modify_texture(handle);
					texture.set_texture_path(Vulkan::get_default_image_path());
				}
			}
			inner_size.value = {0.f, 0.f};
			return;
		}
		texture.set_texture_path(image_renderer.image_path);
		texture.set_texture_rect(image_renderer.image_rect);
		if (nine_scale.enabled) {
			if (nine_scale_handles.size() <= 0) {
				Vulkan::Texture_2D_Description description{};
				description.path = Vulkan::get_default_image_path();
				description.is_visible = false;
				for (int i = 0; i < 9; i++) {
					nine_scale_handles.push_back(Vulkan::create_texture(description));
				}
			}
			for (int i = 0; i < nine_scale_handles.size(); i++) {
				auto& texture = Vulkan::modify_texture(nine_scale_handles[i]);
				texture.set_texture_path(image_renderer.image_path);
			}
		} else {
			inner_size.value = {texture.get_width(), texture.get_height()};
			c_size.value = inner_size.value;
		}
	}

	void update_label_renderer(const Handle& e) {
		Registry& registry = get_registry();
		const auto& [label_renderer] = registry.read<Label_Renderer_Information>(e.id());
		auto [c_handle, c_size] = registry.get<Renderer_Handles, Size>(e.id());
		auto& handles = c_handle.value;
		set_dirty(e, true, DIRTY_TRANSFORM);
		if (handles.size() <= 0) {
			Vulkan::Label_2D_Description description;
			description.is_visible = false;
			handles.push_back(Vulkan::create_label(description));
		}
		auto& label = Vulkan::modify_label(handles[0]);
		std::string font_path = "";
		if (Files::is_file_exists(::Utils::get_root_path() + label_renderer.font_path)) {
			font_path = ::Utils::get_root_path() + label_renderer.font_path;
		}
		label.set_font_path(font_path);
		label.set_text(label_renderer.text);
		label.set_font_path(label_renderer.font_path);
		label.set_font_size(label_renderer.font_size);
		label.set_stroke_color(label_renderer.stroke_color);
		label.set_stroke_size(label_renderer.stroke_size);
		c_size.value = {label.get_width(), label.get_height()};
	}

	void update_scroll_layout_renderer(const Handle& e) {
		Registry& registry = get_registry();
		auto [scroll_container_info] = registry.get<Scroll_Information>(e.id());
		update_layout_renderer(e);
		update_layout_renderer(scroll_container_info.container_handle);
	}

	void update_renderer(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has<Description>(e.id()))
			return;
		const auto& [description] = registry.read<Description>(e.id());
		switch (description.value) {
			case DESCRIPTION_TYPE::LAYOUT_2D: {
				update_layout_renderer(e);
				break;
			}
			case DESCRIPTION_TYPE::IMAGE_2D: {
				update_image_renderer(e);
				break;
			}
			case DESCRIPTION_TYPE::LABEL_2D: {
				update_label_renderer(e);
				break;
			}
			case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
				update_scroll_layout_renderer(e);
				break;
			}
			default: {
				break;
			}
		}
	}

	void update_layout_renderer_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		const auto& [c_handles] = registry.read<Renderer_Handles>(e_child.id());
		const auto& handles = c_handles.value;
		if (handles.size() <= 0)
			return;
		const auto& [c_size, layout_renderer, clipping_renderer, transform] =
			registry.read<Size, Layout_Renderer_Information, Clipping_Information, World_Transform_2D>(e_child.id());
		glm::vec2 size = c_size.value;
		glm::vec2 rect_size = size * transform.scale;
		auto& rect = Vulkan::modify_rectangle(handles[0]);
		if (rect.is_visible()) {
			rect.set_position(transform.position);
			rect.set_anchor(transform.anchor);
			rect.set_width(rect_size.x);
			rect.set_height(rect_size.y);
			rect.set_opacity(transform.opacity);
			rect.set_rotation(transform.rotation);
			rect.set_opacity(transform.opacity);
			rect.set_visible(transform.is_visible);
			float norm_scale = std::max(transform.scale.x, transform.scale.y);
			rect.set_border_radius(layout_renderer.border_radius * norm_scale);
			rect.set_draw_index(transform.draw_index);
		}
		if (clipping_renderer.enabled) {
			auto& clipping = Vulkan::modify_clipping(handles[1]);
			clipping.set_content_size(size);
			clipping.set_anchor(transform.anchor);
			clipping.set_world_position(transform.position);
			clipping.set_world_rotation(transform.rotation);
			clipping.set_world_scale(transform.scale);
		}
	}

	void update_layout_container_renderer_transform(const Handle& child) {
		Registry& registry = get_registry();
		const auto& [tree_info, scroll_container_info] =
			registry.read<Tree_Information, Scroll_Information>(child.id());
		const auto& [c_parent_size] = registry.read<Size>(tree_info.parent.id());
		const auto& parent_size = c_parent_size.value;
		const auto& container_size = scroll_container_info.size;
		auto [c_size, transform] = registry.get<Size, Transform_2D>(child.id());
		c_size.value = glm::vec2(std::max(parent_size.x, container_size.x), std::max(parent_size.y, container_size.y));
		transform.scale = glm::vec2(
			std::max(transform.scale.x, container_size.x / parent_size.x),
			std::max(transform.scale.y, container_size.y / parent_size.y)
		);
		transform.position = glm::vec2(
			std::max(parent_size.x - container_size.x, transform.position.x),
			std::max(parent_size.y - container_size.y, transform.position.y)
		);
		update_layout_renderer_transform(child);
	}

	void update_image_render_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		const auto& [transform, renderer_handle, nice_scale_renderer_handles, c_size] =
			registry.read<World_Transform_2D, Renderer_Handles, Nine_Scale_Renderer_Handles, Size>(e_child.id());
		auto [nine_scale] = registry.get<Nine_Scale_Information>(e_child.id());
		const auto& handles = renderer_handle.value;
		const auto& nice_scale_handles = nice_scale_renderer_handles.value;
		auto& texture = Vulkan::modify_texture(handles[0]);
		auto update_texture_attributes = [](Vulkan::Texture_2D& texture, const World_Transform_2D& transform) {
			texture.set_anchor(transform.anchor);
			texture.set_color(transform.color);
			texture.set_position(transform.position);
			texture.set_opacity(transform.opacity);
			texture.set_rotation(transform.rotation);
			texture.set_visible(transform.is_visible);
			texture.set_scale(transform.scale);
			texture.set_draw_index(transform.draw_index);
		};
		if (nine_scale.enabled) {
			texture.set_visible(false);
			nine_scale.update_piece_transforms(
				c_size.value, transform, {texture.get_width(), texture.get_height()}, texture.get_texture_rect()
			);
			for (int i = 0; i < nice_scale_handles.size() && i < nine_scale.pieces_transforms.size(); i++) {
				auto& piece_texture = Vulkan::modify_texture(nice_scale_handles[i]);
				const auto& piece_transform = nine_scale.pieces_transforms[i];
				const auto& piece_rect = nine_scale.pieces_rects[i];
				piece_texture.set_texture_rect(piece_rect);
				update_texture_attributes(piece_texture, piece_transform);
			}
		} else {
			for (const auto& handle : nice_scale_handles) {
				auto& piece_texture = Vulkan::modify_texture(handle);
				piece_texture.set_visible(false);
			}
			update_texture_attributes(texture, transform);
		}
	}

	void update_label_render_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		const auto& [transform, c_handles] = registry.read<World_Transform_2D, Renderer_Handles>(e_child.id());
		const auto& handles = c_handles.value;
		auto& label = Vulkan::modify_label(handles[0]);
		label.set_anchor(transform.anchor);
		label.set_color(transform.color);
		label.set_position(transform.position);
		label.set_opacity(transform.opacity);
		label.set_rotation(transform.rotation);
		label.set_visible(transform.is_visible);
		label.set_scale(transform.scale);
		label.set_draw_index(transform.draw_index);
	}

	void update_renderer_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		if (!registry.has<Description>(e_child.id())) {
			return;
		}
		const auto& [description] = registry.read<Description>(e_child.id());
		switch (description.value) {
			case DESCRIPTION_TYPE::LAYOUT_2D: {
				update_layout_renderer_transform(e_child);
				break;
			}
			case DESCRIPTION_TYPE::IMAGE_2D: {
				update_image_render_transform(e_child);
				break;
			}
			case DESCRIPTION_TYPE::LABEL_2D: {
				update_label_render_transform(e_child);
				break;
			}
			case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
				update_layout_renderer_transform(e_child);
				break;
			}
			case DESCRIPTION_TYPE::INNER_CONTAINER: {
				update_layout_container_renderer_transform(e_child);
				break;
			}
			default: {
				break;
			}
		}
	}

	void update_touch_transform_area(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Touching_Information, Size>(e.id()))
			return;
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		if (!touch_info.enabled)
			return;
		const auto& [c_size, world_transform] = registry.read<Size, World_Transform_2D>(e.id());
		touch_info.transformed_rect = Math::transform_rect(
			c_size.value, world_transform.position, world_transform.scale, world_transform.rotation,
			world_transform.anchor
		);
		set_dirty(e, touch_info.touched_order != world_transform.draw_index, DIRTY_TOUCHING);
		touch_info.touched_order = world_transform.draw_index;
	}

	void update_scroll_transform_area(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Scroll_Information, Size>(e.id())) {
			return;
		}
	}

	void handle_enities_touched(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		std::vector<Handle> discard_touch_entities;
		for (const Handle& e : dirty_entities) {
			if (!registry.has_all<Touching_Information>(e.id()))
				continue;
			const auto& [touch_info] = registry.read<Touching_Information>(e.id());
			if (touch_info.caller == nullptr)
				continue;
			if (!touch_info.enabled) {
				switch (touch_info.touch_state) {
					case TOUCH_STATE::TOUCH_BEGAN:
					case TOUCH_STATE::TOUCH_MOVE: {
						discard_touch_entities.push_back(e);
						touch_info.caller(e, TOUCH_STATE::TOUCH_ENDED, touch_info.touch_world_position);
						break;
					}
					default: {
						break;
					}
				}
				continue;
			}
			touch_info.caller(e, touch_info.touch_state, touch_info.touch_world_position);
		}
	}

	void handle_dirty_z_order(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		for (const Handle& e : dirty_entities) {
			if (!registry.has_all<Tree_Information, Transform_2D>(e.id()))
				continue;
			auto [tree_info] = registry.get<Tree_Information>(e.id());
			std::stable_sort(
				tree_info.children.begin(), tree_info.children.end(), [&registry](const Handle& a, const Handle& b) {
					const auto& [transform_a] = registry.read<Transform_2D>(a.id());
					const auto& [transform_b] = registry.read<Transform_2D>(b.id());
					return transform_a.z_order < transform_b.z_order;
				}
			);
		}
	}

	void handle_dirty_renderer(const std::vector<Handle>& dirty_entities) {
		for (const Handle& e : dirty_entities) {
			update_renderer(e);
		}
	}

	void handle_dirty_relative_size(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		for (const Handle& e : dirty_entities) {
			if (!registry.has_all<Tree_Information, Size>(e.id()))
				continue;

			const auto& [tree_info] = registry.read<Tree_Information>(e.id());
			auto [transform, c_size] = registry.get<Transform_2D, Size>(e.id());
			const auto& parent = tree_info.parent;
			if (parent.is_invalid())
				continue;
			glm::vec2 parent_size{0.f};
			if (registry.has<Size>(parent.id())) {
				const auto& [c_parent_size] = registry.read<Size>(parent.id());
				parent_size = c_parent_size.value;
			}
			glm::vec2 size = c_size.value;
			glm::vec2 position = transform.position;
			glm::vec2 anchor = transform.anchor;
			if (registry.has<Size_Pinning>(e.id())) {
				const auto& [size_pinning] = registry.read<Size_Pinning>(e.id());
				if (size_pinning.enabled) {
					size = glm::max(
						parent_size -
							glm::vec2(size_pinning.left + size_pinning.right, size_pinning.top + size_pinning.bottom),
						0.f
					);
					position = glm::min(glm::vec2(size_pinning.left, size_pinning.bottom), parent_size - size);
					position += anchor * size;
				}
			}
			if (registry.has<Size_Percent>(e.id())) {
				const auto& [size_percent] = registry.read<Size_Percent>(e.id());
				if (size_percent.enabled) {
					size = parent_size * size_percent.ratio_size;
				}
			}
			set_dirty(e, transform.position != position || c_size.value != size, DIRTY_TRANSFORM);
			transform.position = position;
			c_size.value = size;
		}
	}

	void handle_dirty_relative_position(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		for (const Handle& e : dirty_entities) {
			if (!registry.has_all<Tree_Information, Size_Percent>(e.id()))
				continue;
			const auto& [tree_info, size_percent] = registry.read<Tree_Information, Size_Percent>(e.id());
			auto [transform] = registry.get<Transform_2D>(e.id());
			const auto& parent = tree_info.parent;
			if (parent.is_invalid())
				continue;
			glm::vec2 parent_size = get_content_size(parent);
			if (size_percent.enabled) {
				glm::vec2 position = size_percent.ratio_size * parent_size;
				set_dirty(e, transform.position != position, DIRTY_TRANSFORM);
				transform.position = position;
			}
		}
	}

	void handle_dirty_transform(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		for (const Handle& e : dirty_entities) {
			if (!registry.has<Tree_Information>(e.id()))
				continue;
			const auto& [tree_info] = registry.read<Tree_Information>(e.id());
			auto parent = tree_info.parent;
			const auto& root = get_root();
			if (e.id() == root.id()) {
				parent = root;
			}
			update_world_transform(parent, e);
			update_renderer_transform(e);
			update_touch_transform_area(e);
		}
	}

	void handle_dirty_clipping(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		for (const Handle& e : dirty_entities) {
			if (!registry.has_all<Tree_Information, Clipping_Ancestor_Information>(e.id()))
				continue;
			const auto& [tree_info] = registry.read<Tree_Information>(e.id());
			auto parent = tree_info.parent;
			bool is_parent_clipping = false;
			Handle clipping_handle = Handle::make_invalid_handle();
			if (!registry.has<Clipping_Ancestor_Information>(parent.id()))
				continue;
			if (registry.has<Clipping_Information>(parent.id())) {
				const auto& [parent_clipping] = registry.read<Clipping_Information>(parent.id());
				is_parent_clipping = parent_clipping.enabled;
				clipping_handle = get_render_clipping_handle(parent);
			}
			const auto& [parent_clipping_ancestor] = registry.read<Clipping_Ancestor_Information>(parent.id());
			auto [child_clipping_ancestor] = registry.get<Clipping_Ancestor_Information>(e.id());
			child_clipping_ancestor.number_clipping = parent_clipping_ancestor.number_clipping;
			child_clipping_ancestor.clipping_ancesors = parent_clipping_ancestor.clipping_ancesors;
			if (is_parent_clipping && !clipping_handle.is_invalid()) {
				child_clipping_ancestor.add_clipping_ancestor(clipping_handle);
			}
			auto renderer_handles = get_renderer_handles(e);
			for (const auto& renderer_handle : renderer_handles) {
				auto& draw_attributes = Vulkan::modify_draw_attributes(renderer_handle);
				draw_attributes.set_number_clipping(child_clipping_ancestor.number_clipping);
				draw_attributes.set_clipping_ancestors(child_clipping_ancestor.clipping_ancesors);
			}
		}
	}

	void handle_removed_children(const std::vector<Handle>& removed_children) {
		Registry& registry = get_registry();
		for (const Handle& e : removed_children) {
			auto renderer_handles = get_renderer_handles(e);
			for (const auto& renderer_handle : renderer_handles) {
				auto& draw_attributes = Vulkan::modify_draw_attributes(renderer_handle);
				draw_attributes.set_visible(false);
			}
			auto [tree_info] = registry.get<Tree_Information>(e.id());
			tree_info.valid_child = false;
		}
	}

} // namespace Engine
