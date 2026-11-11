#pragma once

struct GLFWwindow;

/**
 * @Note: ./editor --self-test : scripted drag & drop / move / keys / import with checks and
 * screenshots in /tmp/editor_*.png, then quits (exit code 1 when a check fails).
 */
namespace Editor_Self_Test {

	void start(GLFWwindow* window);

	// one frame of the script; false when finished.
	bool tick();

	int failures();

} // namespace Editor_Self_Test
