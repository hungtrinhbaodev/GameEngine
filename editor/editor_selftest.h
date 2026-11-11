#pragma once

struct GLFWwindow;

/**
 * @Note: ./editor --self-test : scripted drag & drop / move / keys / import with checks and
 * screenshots in /tmp/editor_*.png, then quits (exit code 1 when a check fails).
 */
namespace Editor_Self_Test {

	void start(GLFWwindow* window);

	/**
	 * @Note: ./editor --perf : GPU cost of the editor UI at full GPU clock. Apple GPUs down-clock when a
	 * frame is easy, so the overlay's GPU ms at 60 fps vsync is inflated. Here a fixed load (full-window
	 * layers) keeps the GPU busy, and editor cost = (load + editor) - (load alone). Screenshots /tmp/editor_perf_*.
	 */
	void start_perf(GLFWwindow* window);

	// one frame of the script; false when finished.
	bool tick();

	int failures();

} // namespace Editor_Self_Test
