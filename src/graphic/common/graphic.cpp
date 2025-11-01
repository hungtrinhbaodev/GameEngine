#include <graphic/common/graphic.h>

Graphic::Graphic_State Graphic::Graphic::_graphic_state = Graphic_State::NOT_INIT;

std::mutex Graphic::Graphic::_graphic_state_mutex;

std::vector<std::thread> Graphic::Graphic::threads;

void Graphic::Graphic::_set_graphic_state(Graphic_State state) {

    _graphic_state_mutex.lock();

    _graphic_state = state;

    _graphic_state_mutex.unlock();

}

const Graphic::Graphic_State& Graphic::Graphic::_get_graphic_state() {
    return _graphic_state;
}

void Graphic::Graphic::init(Window* window) {

    // init graphic data singleton by mode
    switch (Graphic_Constants::GRAPHIC_IMPL) {

        case Using_Graphic_Implemtation::VULKAN: {

            const auto& vk_data = Vulkan_Core_Data::get();
            vk_data->init_data(window);

            break;
        }

        default: {

            break;
        }
    }

    // update graphic state
    _set_graphic_state(Graphic_State::RUNNING);
}

void Graphic::Graphic::main(Window* window) {
    threads.emplace_back(std::thread(run, window));
}

void Graphic::Graphic::run(Window* window) {

    // running main loop graphic still end window
    while(window->is_running()) {

        switch (Graphic_Constants::GRAPHIC_IMPL) {

            case Using_Graphic_Implemtation::VULKAN: {

                const auto& vk_data = Vulkan_Core_Data::get();

                // update logic data
                vk_data->update_data();

                // on draw at frame with vulkan
                vk_data->on_draw_frame();
                break;
            }

            default: {

                break;
            }
        }

    }

    // destroy all graphic component to close app
    destroy();

    // update graphic state
    _set_graphic_state(Graphic_State::CLOSED);
}

void Graphic::Graphic::destroy() {
    // clear and clean up graphic by mode
    switch(Graphic_Constants::GRAPHIC_IMPL) {

        case Using_Graphic_Implemtation::VULKAN: {

            const auto& vk_data = Vulkan_Core_Data::get();
            
            vk_data->clear_data();

            Vulkan_Core_Data::clean_up();

            break;
        }

        default: {

            break;
        }
    }
}

bool Graphic::Graphic::is_running() {

    const auto& graphic_state = _get_graphic_state();

    return graphic_state == Graphic_State::RUNNING;
}

void Graphic::Graphic::terminate() {
    for (auto& thread : threads) {
        thread.join();
    }
}