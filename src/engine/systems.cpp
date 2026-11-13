#include <engine/components.h>
#include <engine/engine.h>
#include <engine/scroll_handler.h>
#include <engine/systems.h>
#include <engine/utils.h>
#include <files.h>
#include <math_custom.h>
#include <tuple>
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
				if (handles.value.size() <= 0)
					return {};
				return {handles.value[0]};
			}
			case DESCRIPTION_TYPE::INNER_CONTAINER:
			case DESCRIPTION_TYPE::SCROLL_LAYOUT:
			case DESCRIPTION_TYPE::LAYOUT_2D:
			case DESCRIPTION_TYPE::LABEL_2D: {
				if (handles.value.size() <= 0)
					return {};
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
				case DESCRIPTION_TYPE::INNER_CONTAINER:
				case DESCRIPTION_TYPE::SCROLL_LAYOUT:
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

	Handle get_clipping_renderer_handle(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has<Description>(e.id())) {
			return Handle::make_invalid_handle();
		}
		const auto& [description] = registry.read<Description>(e.id());
		switch (description.value) {
			case DESCRIPTION_TYPE::SCROLL_LAYOUT:
			case DESCRIPTION_TYPE::LABEL_2D:
			case DESCRIPTION_TYPE::IMAGE_2D:
			case DESCRIPTION_TYPE::LAYOUT_2D: {
				const auto& [handles] = registry.read<Renderer_Handles>(e.id());
				if (handles.value.size() <= 1) {
					return Handle::make_invalid_handle();
				}
				return handles.value[1];
			}
			default: {
				return Handle::make_invalid_handle();
			}
		}
	}

	CLIPPING_STATE get_clipping_state_by_ancestors(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Clipping_Ancestor_Information, Rect>(e.id())) {
			return CLIPPING_STATE::CLIPPING_FULL_OUT;
		}
		const auto& [clipping_ancestor, rect] = registry.read<Clipping_Ancestor_Information, Rect>(e.id());
		for (int i = 0; i < clipping_ancestor.number_clipping; i++) {
			const auto& clipping_parent = clipping_ancestor.clipping_ancestors[i];
			if (!registry.has<Rect>(clipping_parent.id()))
				continue;
			const auto& [parent_rect] = registry.read<Rect>(clipping_parent.id());
			CLIPPING_STATE clipping_state = rect.get_cliping_state_with(parent_rect);
			switch (clipping_state) {
				case CLIPPING_STATE::CLIPPING_FULL_OUT: {
					return CLIPPING_STATE::CLIPPING_FULL_OUT;
				}
				case CLIPPING_STATE::CLIPPING_FULL_IN: {
					break;
				}
				default: {
					return CLIPPING_STATE::CLIPPING_INTERECT;
				}
			}
		}
		return CLIPPING_STATE::CLIPPING_FULL_IN;
	}

	bool is_clipping_by_ancestor(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Clipping_Ancestor_Information, Rect>(e.id())) {
			return false;
		}
		const auto& [clipping_ancestor, rect] = registry.read<Clipping_Ancestor_Information, Rect>(e.id());
		for (int i = 0; i < clipping_ancestor.number_clipping; i++) {
			const auto& clipping_parent = clipping_ancestor.clipping_ancestors[i];
			if (!registry.has<Rect>(clipping_parent.id()))
				continue;
			const auto& [parent_rect] = registry.read<Rect>(clipping_parent.id());
			bool is_clipping = rect.is_clipped_by(parent_rect);
			if (is_clipping) {
				return true;
			}
		}
		return false;
	}

	std::tuple<uint32_t, std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>> validate_clipping_ancestors(
		std::tuple<int, std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>> clipping_ancestor
	) {
		std::array<Handle, ::Const::MAX_DEPTH_CLIPPING> clipping_handle_ancestor{};
		uint32_t number_valid_handle = 0;
		auto [number_clipping, clipping_ancestors] = clipping_ancestor;
		for (int i = 0; i < number_clipping; i++) {
			auto clipping_handle = get_clipping_renderer_handle(clipping_ancestors[i]);
			if (!clipping_handle.is_invalid()) {
				clipping_handle_ancestor[number_valid_handle] = clipping_handle;
				number_valid_handle++;
			}
		}
		return std::make_tuple(number_valid_handle, clipping_handle_ancestor);
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
		set_dirty(e, rect.is_visible() != layout_renderer.enabled, DIRTY_TRANSFORM);
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

	void update_clipped_state(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Clipping_Ancestor_Information, Rect>(e.id())) {
			return;
		}
		const auto& [clipping_ancestors] = registry.read<Clipping_Ancestor_Information>(e.id());
		CLIPPING_STATE clipping_state = get_clipping_state_by_ancestors(e);
		auto renderers = get_renderer_handles(e);
		switch (clipping_state) {
			case CLIPPING_STATE::CLIPPING_FULL_IN: {
				auto [number_valid_handle, clipping_handle_ancestors] =
					validate_clipping_ancestors(clipping_ancestors.get_self_clipping_info());
				for (const auto& handle : renderers) {
					auto& draw_attributes = Vulkan::modify_draw_attributes(handle);
					draw_attributes.set_number_clipping(number_valid_handle);
					draw_attributes.set_clipping_ancestors(clipping_handle_ancestors);
				}
				break;
			}
			case CLIPPING_STATE::CLIPPING_FULL_OUT: {
				for (const auto& handle : renderers) {
					auto& draw_attributes = Vulkan::modify_draw_attributes(handle);
					draw_attributes.set_visible(false);
				}
				break;
			}
			default: {
				auto [number_valid_handle, clipping_handle_ancestors] =
					validate_clipping_ancestors(clipping_ancestors.get_clipping_ancestors_info());
				for (const auto& handle : renderers) {
					auto& draw_attributes = Vulkan::modify_draw_attributes(handle);
					draw_attributes.set_number_clipping(number_valid_handle);
					draw_attributes.set_clipping_ancestors(clipping_handle_ancestors);
				}
				break;
			}
		}
	}

	void update_image_renderer(const Handle& e) {
		Registry& registry = get_registry();
		auto [image_renderer, renderer_handles, nine_scale_renderer_handles, inner_size, c_size, nine_scale] =
			registry.get<
				Image_Renderer_Information, Renderer_Handles, Nine_Scale_Renderer_Handles, Inner_Renderer_Size, Size,
				Nine_Scale_Information>(e.id());
		auto& nine_scale_handles = nine_scale_renderer_handles.value;
		auto& handles = renderer_handles.value;
		/**
		 * @Note: incase use multi renderer hanle component
		 * and switch by mode remember to update the dirty
		 * clipping to the handle render can have the ancestor
		 * clipping information!
		 */
		set_dirty(e, true, DIRTY_TRANSFORM | DIRTY_CLIPPING);
		if (handles.size() <= 0) {
			Vulkan::Texture_2D_Description description{};
			description.path = Vulkan::get_default_image_path();
			description.is_visible = false;
			handles.push_back(Vulkan::create_texture(description));
			handles.push_back(Vulkan::create_clipping({}));
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
			if (image_renderer.fit_mode == IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_AUTO) {
				c_size.value = {0.f, 0.f};
			}
			image_renderer.texture_changing = false;
			nine_scale.is_manual_update_inner_size = false;
			set_dirty(e, true, DIRTY_RELATIVE_SIZE);
			return;
		}
		texture.set_texture_path(image_renderer.image_path);
		texture.set_texture_rect(image_renderer.image_rect);
		inner_size.value = {texture.get_width(), texture.get_height()};
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
			if (image_renderer.texture_changing && !nine_scale.is_manual_update_inner_size) {
				nine_scale.renderer_size = inner_size.value;
			}
		}
		if (image_renderer.fit_mode == IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_AUTO) {
			set_dirty(e, c_size.value != inner_size.value, DIRTY_RELATIVE_SIZE);
			c_size.value = inner_size.value;
		}
		image_renderer.texture_changing = false;
		nine_scale.is_manual_update_inner_size = false;
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
			handles.push_back(Vulkan::create_clipping({}));
		}
		auto& label = Vulkan::modify_label(handles[0]);
		std::string font_path = "";
		if (Files::is_file_exists(::Utils::get_root_path() + label_renderer.font_path)) {
			font_path = ::Utils::get_root_path() + label_renderer.font_path;
		}
		label.set_font_path(font_path);
		label.set_text(label_renderer.text);
		label.set_font_size(label_renderer.font_size);
		label.set_stroke_color(label_renderer.stroke_color);
		label.set_stroke_size(label_renderer.stroke_size);
		if (label_renderer.fit_mode == LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO) {
			glm::vec2 size = {label.get_width(), label.get_height()};
			set_dirty(e, c_size.value != size, DIRTY_RELATIVE_SIZE);
			c_size.value = size;
		} else {
			label.set_align((int)label_renderer.horizontal_align);
		}
	}

	void update_scroll_layout_renderer(const Handle& e) {
		Registry& registry = get_registry();
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		update_layout_renderer(e);
		update_layout_renderer(scroll_info.container_handle);
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
		auto renderer_handles = get_renderer_handles(e);
		const auto& [draw_order] = registry.read<Draw_Order>(e.id());
		for (const auto& handle : renderer_handles) {
			auto& draw_attributes = Vulkan::modify_draw_attributes(handle);
			draw_attributes.set_draw_index(draw_order.value);
		}
	}

	void update_layout_renderer_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		const auto& [c_handles] = registry.read<Renderer_Handles>(e_child.id());
		const auto& handles = c_handles.value;
		const auto& [c_size, layout_renderer, clipping_renderer, transform, draw_order] =
			registry.read<Size, Layout_Renderer_Information, Clipping_Information, World_Transform_2D, Draw_Order>(
				e_child.id()
			);
		auto [c_rect] = registry.get<Rect>(e_child.id());
		glm::vec2 size = c_size.value;
		glm::vec2 rect_size = size * transform.scale;
		auto& rect = Vulkan::modify_rectangle(handles[0]);
		rect.set_position(transform.position);
		rect.set_anchor(transform.anchor);
		rect.set_width(rect_size.x);
		rect.set_height(rect_size.y);
		rect.set_opacity(transform.opacity);
		rect.set_rotation(transform.rotation);
		rect.set_opacity(transform.opacity);
		rect.set_visible(transform.is_visible && layout_renderer.enabled);
		float norm_scale = std::max(transform.scale.x, transform.scale.y);
		rect.set_border_radius(layout_renderer.border_radius * norm_scale);
		rect.set_draw_index(draw_order.value);
		if (clipping_renderer.enabled) {
			auto& clipping = Vulkan::modify_clipping(handles[1]);
			clipping.set_content_size(size);
			clipping.set_anchor(transform.anchor);
			clipping.set_world_position(transform.position);
			clipping.set_world_rotation(transform.rotation);
			clipping.set_world_scale(transform.scale);
		}
		c_rect.transformed_rect =
			Math::transform_rect(size, transform.position, transform.scale, transform.rotation, transform.anchor);
	}

	void update_scroll_layout_renderer_transform(const Handle& child) {
		update_layout_renderer_transform(child);
		Registry& registry = get_registry();
		const auto& [c_size, scroll_info] = registry.read<Size, Scroll_Information>(child.id());
		const auto& parent_size = c_size.value;
		const auto& container_size = scroll_info.size;
		auto [c_container_size, transform] = registry.get<Size, Transform_2D>(scroll_info.container_handle.id());
		glm::vec2 claim_size =
			glm::vec2(std::max(parent_size.x, container_size.x), std::max(parent_size.y, container_size.y));
		transform.position = Utils::fit_container_position_with(parent_size, claim_size, transform.position);
		c_container_size.value = claim_size;
		auto [touch_container_size] = registry.get<Size>(scroll_info.container_touch_handle.id());
		touch_container_size.value = parent_size;
	}

	void update_image_renderer_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		const auto& [transform, renderer_handle, nice_scale_renderer_handles, draw_order, image_inner_size] =
			registry.read<
				World_Transform_2D, Renderer_Handles, Nine_Scale_Renderer_Handles, Draw_Order, Inner_Renderer_Size>(
				e_child.id()
			);
		auto [nine_scale, c_rect, clipping_ancestors, c_size, image_renderer] =
			registry.get<Nine_Scale_Information, Rect, Clipping_Ancestor_Information, Size, Image_Renderer_Information>(
				e_child.id()
			);
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
		};
		auto get_fit_mode_scale_ratio = [&renderer = image_renderer](const auto& inner_size, const auto& render_size) {
			glm::vec2 scale{1.f};
			bool is_clipping = false;
			switch (renderer.fit_mode) {
				case IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_AUTO: {
					scale = {1.f, 1.f};
					break;
				}
				case IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_STRETCH: {
					scale = {render_size.x / inner_size.x, render_size.y / inner_size.y};
					break;
				}
				case IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_FILL: {
					float max = std::max(render_size.x / inner_size.x, render_size.y / inner_size.y);
					scale = {max, max};
					is_clipping = max < 1.f;
					break;
				}
				case IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_FIT: {
					float min = std::min(render_size.x / inner_size.x, render_size.y / inner_size.y);
					scale = {min, min};
					break;
				}
				default: {
					break;
				}
			}
			return std::make_tuple(scale, is_clipping);
		};
		glm::vec2 inner_size{0.f, 0.f};
		if (nine_scale.enabled) {
			inner_size = nine_scale.renderer_size;
		} else {
			inner_size = image_inner_size.value;
		}
		if (image_renderer.fit_mode == IMAGE_RENDER_FIT_MODE::IMAGE_RENDER_FIT_AUTO) {
			c_size.value = inner_size;
		}
		auto [scale, is_clipping] = get_fit_mode_scale_ratio(inner_size, c_size.value);
		image_renderer.update_transform_inner_image(c_size.value, scale, transform);
		clipping_ancestors.update_self_clipping(is_clipping ? e_child : Handle::make_invalid_handle());
		World_Transform_2D inner_world_transform = transform.transform(image_renderer.inner_image_transform);
		auto [number_valid_clipping, valid_clipping_ancestors] =
			validate_clipping_ancestors(clipping_ancestors.get_clipping_ancestors_info());
		if (nine_scale.enabled) {
			texture.set_visible(false);
			if (nine_scale.renderer_size.x <= 0 || nine_scale.renderer_size.y <= 0) {
				for (int i = 0; i < nice_scale_handles.size() && i < nine_scale.pieces_transforms.size(); i++) {
					auto& piece_texture = Vulkan::modify_texture(nice_scale_handles[i]);
					piece_texture.set_visible(false);
				}
				return;
			}
			nine_scale.update_piece_transforms(
				nine_scale.renderer_size, inner_world_transform, {texture.get_width(), texture.get_height()},
				image_renderer.image_rect
			);
			inner_size = nine_scale.renderer_size;
			for (int i = 0; i < nice_scale_handles.size() && i < nine_scale.pieces_transforms.size(); i++) {
				auto& piece_texture = Vulkan::modify_texture(nice_scale_handles[i]);
				const auto& piece_transform = nine_scale.pieces_transforms[i];
				const auto& piece_rect = nine_scale.pieces_rects[i];
				piece_texture.set_texture_rect(piece_rect);
				piece_texture.set_number_clipping(number_valid_clipping);
				piece_texture.set_clipping_ancestors(valid_clipping_ancestors);
				update_texture_attributes(piece_texture, piece_transform);
			}
		} else {
			inner_size = image_inner_size.value;
			for (const auto& handle : nice_scale_handles) {
				auto& piece_texture = Vulkan::modify_texture(handle);
				piece_texture.set_visible(false);
			}
			texture.set_number_clipping(number_valid_clipping);
			texture.set_clipping_ancestors(valid_clipping_ancestors);
			update_texture_attributes(texture, inner_world_transform);
		}
		if (is_clipping) {
			auto& clipping = Vulkan::modify_clipping(handles[1]);
			clipping.set_anchor(transform.anchor);
			clipping.set_content_size(c_size.value);
			clipping.set_world_rotation(transform.rotation);
			clipping.set_world_position(transform.position);
			clipping.set_world_scale(transform.scale);
		}
		c_rect.transformed_rect = Math::transform_rect(
			c_size.value, transform.position, transform.scale, transform.rotation, transform.anchor
		);
	}

	void update_label_renderer_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		const auto& [transform, c_handles] = registry.read<World_Transform_2D, Renderer_Handles>(e_child.id());
		auto [label_renderer, clipping_ancestors, c_size, c_inner_size] =
			registry.get<Label_Renderer_Information, Clipping_Ancestor_Information, Size, Inner_Renderer_Size>(
				e_child.id()
			);
		auto update_label_transform = [](Vulkan::Font_2D& label, const World_Transform_2D& transform) {
			label.set_anchor(transform.anchor);
			label.set_color(transform.color);
			label.set_position(transform.position);
			label.set_opacity(transform.opacity);
			label.set_rotation(transform.rotation);
			label.set_visible(transform.is_visible);
			label.set_scale(transform.scale);
		};
		const auto& handles = c_handles.value;
		auto& label = Vulkan::modify_label(handles[0]);
		auto [c_rect] = registry.get<Rect>(e_child.id());
		if (label_renderer.fit_mode != LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO) {
			label_renderer.update_inner_text_by(c_size.value, transform);
			auto inner_transform = transform.transform(label_renderer.inner_label_transform);
			label.set_text(label_renderer.render_text);
			update_label_transform(label, inner_transform);
			if (label_renderer.clipping_enabled) {
				auto& clipping = Vulkan::modify_clipping(handles[1]);
				clipping.set_anchor(transform.anchor);
				clipping.set_content_size(c_size.value);
				clipping.set_world_rotation(transform.rotation);
				clipping.set_world_position(transform.position);
				clipping.set_world_scale(transform.scale);
			}
			clipping_ancestors.update_self_clipping(Handle::make_invalid_handle());
			if (label_renderer.clipping_enabled) {
				clipping_ancestors.update_self_clipping(e_child);
			}
			c_inner_size.value = glm::vec2(label.get_width(), label.get_height());
		} else {
			c_inner_size.value = c_size.value;
			clipping_ancestors.update_self_clipping(Handle::make_invalid_handle());
			update_label_transform(label, transform);
		}
		auto [number_valid_handle, clipping_handle_ancestors] =
			validate_clipping_ancestors(clipping_ancestors.get_clipping_ancestors_info());
		// always upload: when clipping turns off, the label must drop its own clip and keep only ancestors.
		label.set_number_clipping(number_valid_handle);
		label.set_clipping_ancestors(clipping_handle_ancestors);
		c_rect.transformed_rect = Math::transform_rect(
			c_size.value, transform.position, transform.scale, transform.rotation, transform.anchor
		);
	}

	void update_renderer_transform(const Handle& e_child) {
		Registry& registry = get_registry();
		if (!registry.has<Description>(e_child.id())) {
			return;
		}
		const auto& [description] = registry.read<Description>(e_child.id());
		auto renderer_handles = get_renderer_handles(e_child);
		if (renderer_handles.size() <= 0)
			return;
		switch (description.value) {
			case DESCRIPTION_TYPE::LAYOUT_2D:
			case DESCRIPTION_TYPE::INNER_CONTAINER:
			case DESCRIPTION_TYPE::SCROLL_LISTENER_CONTAINER: {
				update_layout_renderer_transform(e_child);
				break;
			}
			case DESCRIPTION_TYPE::IMAGE_2D: {
				update_image_renderer_transform(e_child);
				break;
			}
			case DESCRIPTION_TYPE::LABEL_2D: {
				update_label_renderer_transform(e_child);
				break;
			}
			case DESCRIPTION_TYPE::SCROLL_LAYOUT: {
				update_scroll_layout_renderer_transform(e_child);
				break;
			}
			default: {
				break;
			}
		}
	}

	void update_touch_transform_area(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Touching_Information, Size, World_Transform_2D>(e.id()))
			return;
		auto [touch_info] = registry.get<Touching_Information>(e.id());
		if (!touch_info.enabled)
			return;
		const auto& [c_size, world_transform] = registry.read<Size, World_Transform_2D>(e.id());
		touch_info.transformed_rect = Math::transform_rect(
			c_size.value, world_transform.position, world_transform.scale, world_transform.rotation,
			world_transform.anchor
		);
	}

	void update_scroll_transform_area(const Handle& e) {
		Registry& registry = get_registry();
		if (!registry.has_all<Scroll_Information, Size, World_Transform_2D>(e.id()))
			return;
		auto [scroll_info] = registry.get<Scroll_Information>(e.id());
		if (!scroll_info.enabled)
			return;
		const auto& [c_size, world_transform] = registry.read<Size, World_Transform_2D>(e.id());
		scroll_info.transformed_rect = Math::transform_rect(
			c_size.value, world_transform.position, world_transform.scale, world_transform.rotation,
			world_transform.anchor
		);
	}

	void handle_enities_touched(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		Sparse_Set<Handle> discard_touch_entities;
		Handle claimed_entity = Handle::make_invalid_handle();
		auto touching_caller = [&registry](const Handle& e) {
			const auto& [touch_info] = registry.read<Touching_Information>(e.id());
			if (touch_info.caller != nullptr) {
				if (!touch_info.enabled) {
					switch (touch_info.touch_state) {
						case TOUCH_STATE::TOUCH_BEGAN:
						case TOUCH_STATE::TOUCH_MOVE: {
							touch_info.caller(
								e, TOUCH_STATE::TOUCH_ENDED, touch_info.touch_world_position, touch_info.touch_type,
								touch_info.touch_mods
							);
							break;
						}
						default: {
							break;
						}
					}
				} else {
					touch_info.caller(
						e, touch_info.touch_state, touch_info.touch_world_position, touch_info.touch_type,
						touch_info.touch_mods
					);
				}
			}
			if (touch_info.click_caller != nullptr &&
				is_world_position_in_entity_rect(e, touch_info.touch_world_position)) {
				switch (touch_info.touch_state) {
					case TOUCH_STATE::TOUCH_ENDED: {
						touch_info.click_caller(
							e, touch_info.touch_world_position, touch_info.touch_type, touch_info.touch_mods
						);
						break;
					}
					default: {
						break;
					}
				}
			}
		};
		for (int i = 0; i < dirty_entities.size(); i++) {
			const auto& e = dirty_entities[i];
			if (!is_valid(e) || !registry.has_all<Touching_Information>(e.id())) {
				discard_touch_entities.insert(e.id(), e);
				continue;
			}
			const auto& [touch_info] = registry.read<Touching_Information>(e.id());
			if (touch_info.caller == nullptr && touch_info.click_caller == nullptr)
				continue;
			if (touch_info.caller != nullptr && !touch_info.enabled) {
				discard_touch_entities.insert(e.id(), e);
			}
			touching_caller(e);
			if (touch_info.claim_touch) {
				claimed_entity = e;
				break;
			}
		}
		if (is_valid(claimed_entity)) {
			for (const auto& e : dirty_entities) {
				if (e.id() != claimed_entity.id()) {
					auto [touch_info] = registry.get<Touching_Information>(e.id());
					touch_info.touch_state = TOUCH_STATE::TOUCH_CANCEL;
					touching_caller(e);
					discard_touch_entities.insert(e.id(), e);
				}
			}
		}
		for (const auto& e : dirty_entities) {
			auto [touch_info] = registry.get<Touching_Information>(e.id());
			touch_info.claim_touch = false;
		}
		discard_handling_touches_entities(discard_touch_entities.values());
	}

	void handle_entites_scrolling(const std::vector<Handle>& scrolling_entities) {
		Registry& registry = get_registry();
		for (const Handle& e : scrolling_entities) {
			if (!is_valid(e) || !registry.has<Scroll_Information>(e.id()))
				continue;
			const auto& [scroll_info] = registry.read<Scroll_Information>(e.id());
			if (scroll_info.caller == nullptr)
				continue;
			scroll_info.caller(e, get_scroll_distance());
		}
		reset_scrolling();
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

	void handle_update_tree_draw_order(const std::vector<Handle>& dirty_entities) {
		Registry& registry = get_registry();
		for (uint32_t i = 0; i < dirty_entities.size(); i++) {
			const auto& e = dirty_entities[i];
			auto [draw_order] = registry.get<Draw_Order>(e.id());
			draw_order.value = i;
			if (registry.has<Touching_Information>(e.id())) {
				auto [touch_info] = registry.get<Touching_Information>(e.id());
				touch_info.touched_order = i;
				set_dirty(e, true, DIRTY_TOUCHING);
			}
			if (registry.has<Scroll_Information>(e.id())) {
				auto [scroll_info] = registry.get<Scroll_Information>(e.id());
				scroll_info.scroll_order = i;
				set_dirty(e, true, DIRTY_SCROLLING);
			}
			auto renderer_handles = get_renderer_handles(e);
			for (auto& handle : renderer_handles) {
				auto& draw_attributes = Vulkan::modify_draw_attributes(handle);
				draw_attributes.set_draw_index(draw_order.value);
			}
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
					set_dirty(e, transform.position != position || c_size.value != size, DIRTY_TRANSFORM);
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
			if (!registry.has_all<Tree_Information, Position_Percent>(e.id()))
				continue;
			const auto& [tree_info, position_percent] = registry.read<Tree_Information, Position_Percent>(e.id());
			auto [transform] = registry.get<Transform_2D>(e.id());
			const auto& parent = tree_info.parent;
			if (parent.is_invalid())
				continue;
			glm::vec2 parent_size = get_content_size(parent);
			if (position_percent.enabled) {
				glm::vec2 position = position_percent.ratio_position * parent_size;
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
			update_clipped_state(e);
			update_touch_transform_area(e);
			update_scroll_transform_area(e);
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
			if (!registry.has<Clipping_Ancestor_Information>(parent.id()))
				continue;
			if (registry.has<Clipping_Information>(parent.id())) {
				const auto& [parent_clipping] = registry.read<Clipping_Information>(parent.id());
				is_parent_clipping = parent_clipping.enabled;
			}
			const auto& [parent_clipping_ancestor] = registry.read<Clipping_Ancestor_Information>(parent.id());
			auto [child_clipping_ancestor] = registry.get<Clipping_Ancestor_Information>(e.id());
			child_clipping_ancestor.merge_with(parent_clipping_ancestor, is_parent_clipping, parent);
			update_clipped_state(e);
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
