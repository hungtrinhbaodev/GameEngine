#include <graphic/vulkan_implementation/vulkan_render_data.h>

Graphic::Vulkan_Render_Data::Vulkan_Render_Data() {

    _vk_indices_buffer = new Mesh_Buffer();

    _vk_vertices_buffer = new Mesh_Buffer();

}

void Graphic::Vulkan_Render_Data::init(
    Vulkan_Assets_Manager* vk_asset_mgr
) {

    _vk_asset_mgr = vk_asset_mgr;

    _vk_indices_buffer->make(
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    _vk_vertices_buffer->make(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
}

Graphic::Vulkan_Model_User_Data Graphic::Vulkan_Render_Data::add_model(
    std::string path_mesh, 
    std::string path_texture,
    glm::mat4 transform
) {

    std::unique_lock<std::mutex> lock(_models_lock);

    std::string model_key = Vulkan_Model::make_key(path_mesh, path_texture);

    if (_vk_models.find(model_key) == _vk_models.end()) {
        
        _vk_models[model_key] = _vk_asset_mgr->load_vk_model(
            Core::Resource_Load_Mode::ASYNC,
            path_mesh,
            path_texture,
            _vk_vertices_buffer,
            _vk_indices_buffer
        );

        _vk_instance_buffers[model_key] = new Instance_Buffer();
        _vk_instance_buffers[model_key]->make(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
    }

    long model_id = _id_generate.gen_id();
    _vk_instance_buffers[model_key]->add_data(model_id, transform);

    return Vulkan_Model_User_Data {
        _vk_models[model_key]->get_key(),
        model_id,
        _vk_models[model_key]
    };
}

void Graphic::Vulkan_Render_Data::update_model(const Vulkan_Model_User_Data& model_data, glm::mat4 transform) {

    std::string model_key = model_data.model_key;
    long model_id = model_data.model_id;

    if (_vk_instance_buffers.find(model_key) == _vk_instance_buffers.end()) {
        throw std::runtime_error("Fail to update model in render data: model key is not found!");
    }

    _vk_instance_buffers[model_key]->update_data(model_id, transform);
}

void Graphic::Vulkan_Render_Data::delete_model(const Vulkan_Model_User_Data& model_data) {

    std::unique_lock<std::mutex> lock(_models_lock);

    std::string model_key = model_data.model_key;
    long model_id = model_data.model_id;

    if (_vk_instance_buffers.find(model_key) == _vk_instance_buffers.end()) {
        throw std::runtime_error("Fail to delete model in render data: model key is not found!");
    }    

    _vk_instance_buffers[model_key]->delete_data(model_id);

    if (_vk_instance_buffers[model_key]->get_using_size() <= 0) {

        _vk_instance_buffers[model_key]->request_using_buffer();

        _vk_instance_buffers[model_key]->destroy();

        _vk_instance_buffers[model_key]->release_using_buffer();
        
        delete(_vk_instance_buffers[model_key]);

        _vk_instance_buffers.erase(model_key);
        
        _vk_indices_buffer->delete_data(model_key);

        _vk_vertices_buffer->delete_data(model_key);

        _vk_models.erase(model_key);
    }
}

std::map<Graphic::Vulkan_Draw_ID, std::map<std::string, std::vector<std::shared_ptr<Graphic::Vulkan_Model>>>> Graphic::Vulkan_Render_Data::get_render_models() {

    std::unique_lock<std::mutex> lock(_models_lock);

    std::map<Vulkan_Draw_ID, std::map<std::string, std::vector<std::shared_ptr<Vulkan_Model>>>> render_data;
    for (auto& [_, model] : _vk_models) {
        if (model->get_loaded_state() != Core::Resource_Loaded_State::LOADED) continue;
        Vulkan_Draw_ID draw_ID = model->get_draw_id();
        if (render_data.find(draw_ID) == render_data.end()) {
            render_data[draw_ID] = {};
        }
        switch (model->get_draw_id()) {
            case Vulkan_Draw_ID::OBJECT_DEFAULT: {
                // Object default has no texture
                // so it's key is empty
                render_data[draw_ID][""].push_back(model);
                break;
            }
            default: {
                std::shared_ptr<Vulkan_Texture> texture = model->get_vk_texture(); 
                const std::string& texture_key = texture->get_key();
                if (render_data[draw_ID].find(texture_key) == render_data[draw_ID].end()) {
                    render_data[draw_ID][texture_key] = {};
                }
                render_data[draw_ID][texture_key].push_back(model);
                break;
            }
        }
    }
    return render_data;
}

Graphic::Mesh_Buffer* Graphic::Vulkan_Render_Data::get_vertices_buffer() {
    return _vk_vertices_buffer;
}

Graphic::Mesh_Buffer* Graphic::Vulkan_Render_Data::get_indices_buffer() {
    return _vk_indices_buffer;
}

Graphic::Instance_Buffer* Graphic::Vulkan_Render_Data::get_instances_buffer(std::string key) {
    if (_vk_instance_buffers.find(key) == _vk_instance_buffers.end()) {
        throw std::runtime_error("Fail to get instance buffer: undefined key model!");
    }
    return _vk_instance_buffers[key];
}

void Graphic::Vulkan_Render_Data::destroy() {
    _vk_indices_buffer->destroy();

    _vk_vertices_buffer->destroy();

    for (auto& [_, instances_buffer] : _vk_instance_buffers) {
        instances_buffer->destroy();
        delete(instances_buffer);
    }
}

Graphic::Vulkan_Render_Data::~Vulkan_Render_Data() {
    delete(_vk_indices_buffer);

    delete(_vk_vertices_buffer);
}