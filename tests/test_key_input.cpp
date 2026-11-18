/**
 * @Note: this file was written by Claude (Claude Code) on 2026-11-18.
 * Focus / keyboard / char input checks: `./ui_test repro-key` (also in tests/run_all.sh).
 * Every check logs "[repro] PASS | key-input | <group> | ..." or "[repro] FAIL ..."; the suite counts a FAIL
 * line as a failed case. Groups:
 *   convert   - engine/input_glfw.h: GLFW key/action/mouse-button/mods -> engine enums (no engine state)
 *   component - engine/components.h Key_Board_Information::can_handle_key_input / can_handle_char_input,
 *               tested directly on the struct (no entity needed)
 *   focus     - engine/engine.h set_key_focus / get_focused_entitiy / set_focus_enter_caller /
 *               set_focus_exit_caller / handle_changing_focus_target
 *   dispatch  - engine/systems.h handle_entities_key_inputed: focused entity goes first, break_chain via the
 *               caller's `swallowed` out-param stops the chain, cleaned-up entities are skipped
 *   pipeline  - the real GLFW callbacks (engine/key_handler.cpp on_key_pressed / on_char_callback, called
 *               directly here instead of through a real keystroke) through one Engine::process_frame
 */
#include <algorithm>
#include <engine/components.h>
#include <engine/engine.h>
#include <engine/input_glfw.h>
#include <engine/key_handler.h>
#include <engine/systems.h>
#include <log.h>
#include <string>
#include <test_ui.h>
#include <vector>

// on_key_pressed / on_char_callback are the real GLFW callbacks registered by Engine::init_key_handler
// (engine/key_handler.cpp). They are not declared in key_handler.h (only the two getters + cleanup are),
// so the pipeline check below forward-declares them here and calls them directly with a null window - the
// window argument is unused by both functions - to simulate a real keystroke / char without needing actual
// OS input.
namespace Engine {
	void on_key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods);
	void on_char_callback(GLFWwindow* window, unsigned int code_point);
} // namespace Engine

namespace Test {

	namespace {
		int key_input_failed = 0;

		void expect(const std::string& group, bool ok, const std::string& what) {
			if (!ok)
				key_input_failed++;
			Log::info(ok ? "[repro] PASS" : "[repro] FAIL", "key-input", group, what);
		}

		std::string join(const std::vector<std::string>& values) {
			std::string out = "[";
			for (size_t i = 0; i < values.size(); i++) {
				out += values[i];
				if (i + 1 < values.size())
					out += ", ";
			}
			return out + "]";
		}

		// ------------------------------ convert ------------------------------

		void check_convert() {
			const std::string group = "convert";
			expect(group, Engine::glfw_to_key(GLFW_KEY_A) == Engine::KEY_A, "GLFW_KEY_A -> KEY_A");
			expect(group, Engine::glfw_to_key(GLFW_KEY_ENTER) == Engine::KEY_ENTER, "GLFW_KEY_ENTER -> KEY_ENTER");
			expect(group, Engine::glfw_to_key(GLFW_KEY_BACKSPACE) == Engine::KEY_BACKSPACE, "GLFW_KEY_BACKSPACE -> KEY_BACKSPACE");
			expect(group, Engine::glfw_to_key(GLFW_KEY_LEFT) == Engine::KEY_LEFT, "GLFW_KEY_LEFT -> KEY_LEFT");
			expect(group, Engine::glfw_to_key(-1) == Engine::KEY_UNKNOWN, "negative / unknown GLFW key code -> KEY_UNKNOWN");
			expect(group, Engine::glfw_to_key(999999) == Engine::KEY_UNKNOWN, "out of range GLFW key code -> KEY_UNKNOWN");

			expect(group, Engine::glfw_to_key_action(GLFW_PRESS) == Engine::KEY_ACTION_PRESS, "GLFW_PRESS -> KEY_ACTION_PRESS");
			expect(group, Engine::glfw_to_key_action(GLFW_RELEASE) == Engine::KEY_ACTION_RELEASE, "GLFW_RELEASE -> KEY_ACTION_RELEASE");
			expect(group, Engine::glfw_to_key_action(GLFW_REPEAT) == Engine::KEY_ACTION_REPEAT, "GLFW_REPEAT -> KEY_ACTION_REPEAT");
			expect(
				group, Engine::glfw_to_key_action(-1) == Engine::KEY_ACTION_RELEASE,
				"unknown action value falls back to KEY_ACTION_RELEASE, per input_glfw.h's own contract"
			);

			expect(
				group, Engine::glfw_to_mouse_button(GLFW_MOUSE_BUTTON_LEFT) == Engine::MOUSE_BUTTON_LEFT,
				"GLFW_MOUSE_BUTTON_LEFT -> MOUSE_BUTTON_LEFT"
			);
			expect(
				group, Engine::glfw_to_mouse_button(GLFW_MOUSE_BUTTON_8) == Engine::MOUSE_BUTTON_8,
				"GLFW_MOUSE_BUTTON_8 -> MOUSE_BUTTON_8"
			);
			expect(
				group, Engine::glfw_to_mouse_button(42) == Engine::MOUSE_BUTTON_UNKNOWN,
				"out of range mouse button -> MOUSE_BUTTON_UNKNOWN"
			);

			int shift_ctrl = Engine::glfw_to_key_mods(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL);
			expect(group, (shift_ctrl & Engine::KEY_MOD_SHIFT) != 0, "GLFW_MOD_SHIFT bit carried over");
			expect(group, (shift_ctrl & Engine::KEY_MOD_CONTROL) != 0, "GLFW_MOD_CONTROL bit carried over");
			expect(group, (shift_ctrl & Engine::KEY_MOD_ALT) == 0, "GLFW_MOD_ALT bit not set when not requested");

			int every_mod = GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER | GLFW_MOD_CAPS_LOCK | GLFW_MOD_NUM_LOCK;
			int expected = Engine::KEY_MOD_SHIFT | Engine::KEY_MOD_CONTROL | Engine::KEY_MOD_ALT | Engine::KEY_MOD_SUPER |
						   Engine::KEY_MOD_CAPS_LOCK | Engine::KEY_MOD_NUM_LOCK;
			expect(group, Engine::glfw_to_key_mods(every_mod) == expected, "every known GLFW mod bit round-trips to its own KEY_MOD_* bit");
			expect(group, Engine::glfw_to_key_mods(1 << 10) == 0, "an unknown mod bit is dropped, not passed through as garbage");
		}

		// ------------------------------ component ------------------------------

		void check_component() {
			const std::string group = "component";

			Engine::Key_Board_Information fresh{};
			expect(group, !fresh.has_lisenter(), "fresh Key_Board_Information has no listener");

			Engine::Key_Board_Information key_only{};
			key_only.key_input_enabled = true;
			expect(group, key_only.has_lisenter(), "key_input_enabled alone makes has_lisenter() true");
			expect(group, !key_only.can_handle_key_input(), "can_handle_key_input() false without a caller, even when enabled");
			key_only.key_input_caller = [](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool&) {};
			expect(group, key_only.can_handle_key_input(), "can_handle_key_input() true once enabled + key_input_caller are both set");

			Engine::Key_Board_Information char_only{};
			char_only.char_input_enabled = true;
			char_only.char_input_caller = [](const Handle&, char32_t, bool&) {};
			expect(
				group, char_only.can_handle_char_input(),
				"can_handle_char_input() true when char_input_enabled + char_input_caller are set, no key_input_caller "
				"involved (components.cpp Key_Board_Information::can_handle_char_input currently reads key_input_caller "
				"instead of char_input_caller - expected to FAIL until that line is fixed)"
			);

			Engine::Key_Board_Information key_caller_no_char_caller{};
			key_caller_no_char_caller.char_input_enabled = true;
			key_caller_no_char_caller.key_input_caller = [](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool&) {};
			// char_input_caller left null on purpose: only key_input_caller + char_input_enabled are set.
			expect(
				group, !key_caller_no_char_caller.can_handle_char_input(),
				"can_handle_char_input() should stay false when char_input_caller is null, even if key_input_caller "
				"and char_input_enabled are set (same root cause as above, other direction - this one currently "
				"returns true, which lets handle_entities_key_inputed call a null char_input_caller std::function)"
			);
		}

		// ------------------------------ focus ------------------------------

		void check_focus() {
			const std::string group = "focus";
			Handle a = Engine::create_node();
			Handle b = Engine::create_node();
			Engine::add_child(Engine::get_root(), a);
			Engine::add_child(Engine::get_root(), b);

			expect(
				group, !same(Engine::get_focused_entitiy(), a) && !same(Engine::get_focused_entitiy(), b),
				"freshly created entities are not already focused"
			);

			std::vector<std::string> events;
			Engine::set_focus_enter_caller(a, [&events](const Handle&) { events.push_back("a-enter"); });
			Engine::set_focus_exit_caller(a, [&events](const Handle&) { events.push_back("a-exit"); });
			Engine::set_focus_enter_caller(b, [&events](const Handle&) { events.push_back("b-enter"); });
			Engine::set_focus_exit_caller(b, [&events](const Handle&) { events.push_back("b-exit"); });

			// first ever focus in this process: nothing was focused before, so there is nothing to exit from,
			// but `a` itself gained focus and its own enter caller is expected to fire.
			Engine::set_key_focus(a);
			expect(group, same(Engine::get_focused_entitiy(), a), "get_focused_entitiy() == a right after set_key_focus(a)");
			Engine::handle_changing_focus_target();
			expect(
				group, !events.empty() && events.back() == "a-enter",
				"a's enter caller fires on the very first focus ever (handle_changing_focus_target currently "
				"gates the whole enter/exit block on is_valid(last_focus), so with no previous focus this "
				"never fires - expected to FAIL until that's fixed): got " + join(events)
			);

			events.clear();
			// move focus a -> b: a should exit, b should enter.
			Engine::set_key_focus(b);
			expect(group, same(Engine::get_focused_entitiy(), b), "get_focused_entitiy() == b after set_key_focus(b)");
			Engine::handle_changing_focus_target();
			expect(
				group, std::find(events.begin(), events.end(), "a-exit") != events.end(),
				"a's exit caller fires when focus moves away from it: got " + join(events)
			);
			expect(
				group, std::find(events.begin(), events.end(), "b-enter") != events.end(),
				"b's enter caller fires when focus moves onto it: got " + join(events)
			);

			Engine::clean_up(a);
			Engine::clean_up(b);
			Engine::process_frame(0.f);
		}

		// ------------------------------ dispatch ------------------------------

		void check_dispatch() {
			const std::string group = "dispatch";

			// keep the focused entity used by earlier/later checks out of this group's way.
			Handle dummy_focus = Engine::create_node();
			Engine::add_child(Engine::get_root(), dummy_focus);
			Engine::set_key_focus(dummy_focus);

			Handle a = Engine::create_node();
			Engine::add_child(Engine::get_root(), a);
			expect(group, !Engine::is_key_input_enabled(a), "key input starts disabled on a fresh node");
			Engine::set_key_input_enabled(a, true);
			expect(group, Engine::is_key_input_enabled(a), "set_key_input_enabled(true) sticks");
			expect(group, !Engine::is_char_input_enabled(a), "char input starts disabled on a fresh node");
			Engine::set_char_input_enabled(a, true);
			expect(group, Engine::is_char_input_enabled(a), "set_char_input_enabled(true) sticks");
			Engine::set_char_input_enabled(a, false);
			expect(group, !Engine::is_char_input_enabled(a), "set_char_input_enabled(false) sticks");

			Engine::Handled_Key_Information key_a{};
			key_a.key = Engine::KEY_A;
			key_a.key_action = Engine::KEY_ACTION_PRESS;
			key_a.key_mods = 0;

			// focused entity goes first, and a true `swallowed` stops the chain before the listener list runs.
			std::vector<std::string> calls;
			Handle focused = Engine::create_node();
			Engine::add_child(Engine::get_root(), focused);
			Engine::set_key_input_enabled(focused, true);
			Engine::set_key_input_caller(focused, [&calls](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool& swallowed) {
				calls.push_back("focused");
				swallowed = true;
			});
			Handle listener = Engine::create_node();
			Engine::add_child(Engine::get_root(), listener);
			Engine::set_key_input_enabled(listener, true);
			Engine::set_key_input_caller(listener, [&calls](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool&) {
				calls.push_back("listener");
			});
			Engine::set_key_focus(focused);
			Engine::handle_entities_key_inputed({listener}, {key_a}, {});
			expect(
				group, calls.size() == 1 && calls[0] == "focused",
				"focused entity's caller runs first and swallowing the key stops the listener list: got " + join(calls)
			);

			// listener list break_chain: first listener swallows, second one in the list must not run.
			calls.clear();
			Engine::set_key_focus(dummy_focus); // back to a no-op focus for this part.
			Handle l1 = Engine::create_node();
			Engine::add_child(Engine::get_root(), l1);
			Engine::set_key_input_enabled(l1, true);
			Engine::set_key_input_caller(l1, [&calls](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool& swallowed) {
				calls.push_back("l1");
				swallowed = true;
			});
			Handle l2 = Engine::create_node();
			Engine::add_child(Engine::get_root(), l2);
			Engine::set_key_input_enabled(l2, true);
			Engine::set_key_input_caller(l2, [&calls](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool&) {
				calls.push_back("l2");
			});
			Engine::handle_entities_key_inputed({l1, l2}, {key_a}, {});
			expect(
				group, calls.size() == 1 && calls[0] == "l1",
				"first listener swallowing the key stops the rest of the listener list: got " + join(calls)
			);

			// char dispatch: entity only has char_input_enabled + char_input_caller (no key_input_caller at all).
			calls.clear();
			Handle char_entity = Engine::create_node();
			Engine::add_child(Engine::get_root(), char_entity);
			Engine::set_char_input_enabled(char_entity, true);
			Engine::set_char_input_caller(char_entity, [&calls](const Handle&, char32_t, bool&) { calls.push_back("char"); });
			Engine::handle_entities_key_inputed({char_entity}, {}, {U'x'});
			expect(
				group, calls.size() == 1 && calls[0] == "char",
				"char_input_caller fires from char_input_enabled alone, independent of key_input_caller "
				"(see component group - expected to FAIL today): got " + join(calls)
			);

			// the dangerous direction of the same bug: char_input_enabled + key_input_caller set, char_input_caller
			// left null. can_handle_char_input() currently returns true here, so the dispatcher calls a null
			// std::function - this is wrapped in its own try/catch so it cannot take the rest of the suite down.
			Handle crash_entity = Engine::create_node();
			Engine::add_child(Engine::get_root(), crash_entity);
			Engine::set_char_input_enabled(crash_entity, true);
			Engine::set_key_input_caller(crash_entity, [](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool&) {});
			bool threw = false;
			std::string what;
			try {
				Engine::handle_entities_key_inputed({crash_entity}, {}, {U'y'});
			} catch (const std::exception& exception) {
				threw = true;
				what = exception.what();
			}
			expect(
				group, !threw,
				"char_input_enabled + key_input_caller set but char_input_caller left null must not throw "
				"when a char comes in (currently throws std::bad_function_call: " + what +
				" - same root cause as the component-group checks above)"
			);

			// a cleaned-up entity must not be called even if it is still in the list passed in.
			bool doomed_called = false;
			Handle doomed = Engine::create_node();
			Engine::add_child(Engine::get_root(), doomed);
			Engine::set_key_input_enabled(doomed, true);
			Engine::set_key_input_caller(doomed, [&doomed_called](const Handle&, Engine::KEY, Engine::KEY_ACTION, int, bool&) {
				doomed_called = true;
			});
			Engine::clean_up(doomed);
			Engine::process_frame(0.f); // clean_up is queued until the next frame.
			Engine::handle_entities_key_inputed({doomed}, {key_a}, {});
			expect(group, !doomed_called, "a cleaned-up entity's key caller is not invoked even if still passed in the list");

			Engine::clean_up(dummy_focus);
			Engine::clean_up(a);
			Engine::clean_up(focused);
			Engine::clean_up(listener);
			Engine::clean_up(l1);
			Engine::clean_up(l2);
			Engine::clean_up(char_entity);
			Engine::clean_up(crash_entity);
			Engine::process_frame(0.f);
		}

		// ------------------------------ pipeline ------------------------------

		void check_pipeline() {
			const std::string group = "pipeline";
			Handle target = Engine::create_node();
			Engine::add_child(Engine::get_root(), target);
			Engine::set_key_input_enabled(target, true);
			Engine::set_char_input_enabled(target, true);

			std::vector<std::string> calls;
			Engine::set_key_input_caller(
				target, [&calls](const Handle&, Engine::KEY key, Engine::KEY_ACTION action, int mods, bool&) {
					calls.push_back(
						"key:" + std::to_string((int)key) + ":" + std::to_string((int)action) + ":" + std::to_string(mods)
					);
				}
			);
			Engine::set_char_input_caller(target, [&calls](const Handle&, char32_t code_point, bool&) {
				calls.push_back("char:" + std::to_string((unsigned)code_point));
			});
			Engine::set_key_focus(target);
			Engine::process_frame(0.f);

			expect(group, Engine::get_keys_input_info().empty(), "no stray queued key events before the simulated keystroke");
			expect(group, Engine::get_chars_input().empty(), "no stray queued char events before the simulated keystroke");

			// simulate a real "Shift+A" keystroke the way GLFW would deliver it: a key callback and a char
			// callback, both before the next Engine::process_frame (exactly how test_main.cpp's on_key wires in).
			Engine::on_key_pressed(nullptr, GLFW_KEY_A, 0, GLFW_PRESS, GLFW_MOD_SHIFT);
			Engine::on_char_callback(nullptr, (unsigned int)U'A');
			expect(group, Engine::get_keys_input_info().size() == 1, "one key event queued after on_key_pressed");
			expect(group, Engine::get_chars_input().size() == 1, "one char event queued after on_char_callback");

			Engine::process_frame(0.016f);

			expect(group, Engine::get_keys_input_info().empty(), "key queue cleared after the frame that dispatched it");
			expect(group, Engine::get_chars_input().empty(), "char queue cleared after the frame that dispatched it");
			expect(
				group, calls.size() == 2,
				"target is both the focused entity (fast path in handle_entities_key_inputed) and a registered "
				"listener (key_input_enabled, so engine.cpp's internal key_listener_entities also contains it) - "
				"neither caller sets `swallowed`, so each fires once from the focused-entity fast path and once "
				"more from the listener-list loop: every keystroke an edit box types while focused would be "
				"applied twice. Expected 2 calls, got " + std::to_string(calls.size()) + ": " + join(calls)
			);
			std::string want_key =
				"key:" + std::to_string((int)Engine::KEY_A) + ":" + std::to_string((int)Engine::KEY_ACTION_PRESS) + ":" +
				std::to_string((int)Engine::KEY_MOD_SHIFT);
			expect(
				group, std::find(calls.begin(), calls.end(), want_key) != calls.end(),
				"key caller received KEY_A / PRESS / SHIFT through the real glfw_to_key* converters: got " + join(calls)
			);
			std::string want_char = "char:" + std::to_string((unsigned)U'A');
			expect(
				group, std::find(calls.begin(), calls.end(), want_char) != calls.end(),
				"char caller received code point 'A' through the real on_char_callback path: got " + join(calls)
			);

			Engine::clean_up(target);
			Engine::process_frame(0.f);
		}
	} // namespace

	void repro_key_input_checks() {
		Log::info("[repro] focus / keyboard / char input checks (convert, component, focus, dispatch, pipeline)");
		key_input_failed = 0;
		auto run = [](const std::string& name, void (*check)()) {
			try {
				check();
			} catch (const std::exception& exception) {
				expect(name, false, std::string("threw: ") + exception.what());
			}
		};
		run("convert", check_convert);
		run("component", check_component);
		run("focus", check_focus);
		run("dispatch", check_dispatch);
		run("pipeline", check_pipeline);
		print_pool_diff(get_baseline(), count_pools(), "after key-input checks");
		if (key_input_failed == 0) {
			Log::info("[repro] survived key-input checks, all pass");
		} else {
			Log::info("[repro] key-input checks:", key_input_failed, "failed");
		}
	}

} // namespace Test
