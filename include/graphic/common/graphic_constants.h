#pragma once

#include <string>

namespace Graphic {

    enum Using_Graphic_Implemtation {
        VULKAN,
        OPEN_GL
    };

    enum Graphic_State {
        NOT_INIT,
        RUNNING,
        CLOSED
    };

    class Graphic_Constants {

        public:

        static Using_Graphic_Implemtation GRAPHIC_IMPL;

        static std::string KEY_TRACK_TIME_QUEUE_SEND_DRAW;

        static std::string KEY_TRACK_TIME_QUEUE_SEND_COMMONS;

        static std::string KEY_TRACK_TIME_RECORD_DRAWS;

        static std::string KEY_TRACK_TIME_ACQUIRE_IMAGE;

        static std::string KEY_TRACK_TIME_FULL_DRAW;

    };

}