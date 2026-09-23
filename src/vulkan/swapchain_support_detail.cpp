#include <log.h>
#include <vulkan/swapchain_support_detail.h>

namespace Vulkan {

	void Swapchain_Support_Detail::log_info() const {
		Log::log_info(
			"Info swap chain of device: ", "\nformat size: ", formats.size(),
			"\npresent_modes_size: ", present_modes.size()
		);
	}

} // namespace Vulkan