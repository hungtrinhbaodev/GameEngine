#pragma once
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

    };

}