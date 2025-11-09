#include <graphic/common/graphic.h>
#include <utility/time_utils.h>

Graphic::Graphic_State Graphic::Graphic::_graphic_state = Graphic_State::NOT_INIT;

std::mutex Graphic::Graphic::_graphic_state_mutex;

std::vector<std::thread> Graphic::Graphic::threads;

double Graphic::Graphic::global_fps = -1.0f;

double Graphic::Graphic::global_draw_time = 0.0f;

double Graphic::Graphic::global_time_prepare_data = -1.0f;

double Graphic::Graphic::global_time_draw_in_cpu = -1.0f;

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
    double last_time = glfwGetTime();
    int frame_count = 0;
    double avg_time_draw = 0;

    // running main loop graphic still end window
    while(window->is_running()) {
        
        double current_time = glfwGetTime();
        double tmp_time_draw_in_cpu = 0.0f;
        double tmp_time_prepare_data = 0.0f;
        frame_count++;

        switch (Graphic_Constants::GRAPHIC_IMPL) {

            case Using_Graphic_Implemtation::VULKAN: {

                const auto& vk_data = Vulkan_Core_Data::get();

                // update logic data
                vk_data->update_data();

                // on draw at frame with vulkan
                double start_time_draw = glfwGetTime();
                vk_data->on_draw_frame();
                avg_time_draw += glfwGetTime() - start_time_draw;

                // save internal draw time
                tmp_time_draw_in_cpu += vk_data->get_time_cpu_draw();
                tmp_time_prepare_data += vk_data->get_time_prepare_draw();

                break;
            }

            default: {

                break;
            }
        }

        if (current_time - last_time >= 1.0f) {
            global_fps = (double)frame_count / (current_time - last_time);
            global_draw_time = avg_time_draw / frame_count;
            global_time_draw_in_cpu = tmp_time_draw_in_cpu / frame_count;
            global_time_prepare_data = tmp_time_prepare_data / frame_count;
            frame_count = 0;
            last_time = current_time;
            avg_time_draw = 0;
            tmp_time_draw_in_cpu = 0;
            tmp_time_prepare_data = 0;

            std::cout << std::endl;

            auto queue_send_track = Utility::Time_Utils::get()->get_time_track_info(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_DRAW);
            std::cout << "Time track queue send draw: " << queue_send_track.first << ", " << queue_send_track.second<< ", " <<(queue_send_track.first / queue_send_track.second) << std::endl;

            auto queu_min_max_track = Utility::Time_Utils::get()->get_time_min_max_track_info(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_DRAW);
            std::cout << "Time track queue max, min send draw: " << queu_min_max_track.first << ", " << queu_min_max_track.second << std::endl;

            auto record_draw_track = Utility::Time_Utils::get()->get_time_track_info(Graphic_Constants::KEY_TRACK_TIME_RECORD_DRAWS);
            std::cout << "Time track record draw: " << record_draw_track.first << ", " << record_draw_track.second<< ", " <<(record_draw_track.first / record_draw_track.second) << std::endl;

            auto acquire_image_track = Utility::Time_Utils::get()->get_time_track_info(Graphic_Constants::KEY_TRACK_TIME_ACQUIRE_IMAGE);
            std::cout << "Time track acquire image: " << acquire_image_track.first << ", " << acquire_image_track.second<< ", " <<(acquire_image_track.first / acquire_image_track.second) << std::endl;

            Utility::Time_Utils::get()->clear_track(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_DRAW);
            Utility::Time_Utils::get()->clear_track(Graphic_Constants::KEY_TRACK_TIME_RECORD_DRAWS);
            Utility::Time_Utils::get()->clear_track(Graphic_Constants::KEY_TRACK_TIME_ACQUIRE_IMAGE);
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