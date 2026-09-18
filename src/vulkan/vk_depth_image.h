#pragma once

namespace Vulkan {

	namespace Init {
		void _init_depth_image();
	}

	namespace Process {
		void _recreate_depth_image();
	}

	namespace Destroy {
		void _destroy_depth_image();
	}

} // namespace Vulkan