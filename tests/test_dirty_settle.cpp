/**
 * @Note: this file was written by Claude (Claude Code) on 2026-11-20.
 * Dirty-flag settle checks: `./ui_test repro-dirty` (also in tests/run_all.sh).
 * After a scene finishes building and a few frames run with ZERO external changes in between (no
 * set_position / set_content_size / touch / anything - just Engine::process_frame on an idle scene),
 * every entity's Dirty_Flag must settle to 0 and STAY at 0. If something keeps re-marking itself dirty
 * every frame regardless of whether anything actually changed, that's a silent perpetual recompute loop -
 * wasted CPU/GPU work forever - and exactly the kind of bug a new feature (like the edit box) can
 * introduce by, e.g., calling set_dirty(..., true, ...) unconditionally instead of gating it on "did the
 * value actually change".
 * Runs this across every scene builder currently in the test harness for broad coverage.
 */
#include <engine/components.h>
#include <engine/engine.h>
#include <engine/systems.h>
#include <functional>
#include <log.h>
#include <string>
#include <test_ui.h>
#include <vector>

namespace Test {

	namespace {
		int dirty_settle_failed = 0;

		void expect(const std::string& group, bool ok, const std::string& what) {
			if (!ok)
				dirty_settle_failed++;
			Log::info(ok ? "[repro] PASS" : "[repro] FAIL", "dirty-settle", group, what);
		}

		// every entity with a Dirty_Flag whose value isn't 0, as "id:value" pairs, capped so one bad
		// frame doesn't flood the log.
		std::string list_dirty_entities(int max_entries = 8) {
			std::string out;
			int count = 0;
			Engine::get_registry().each<Engine::Dirty_Flag>([&](uint32_t id, Engine::Dirty_Flag& flag) {
				if (flag.value == 0 || count >= max_entries)
					return;
				out += std::to_string(id) + ":" + std::to_string(flag.value) + " ";
				count++;
			});
			return out;
		}

		size_t count_dirty_entities() {
			size_t count = 0;
			Engine::get_registry().each<Engine::Dirty_Flag>([&count](uint32_t, Engine::Dirty_Flag& flag) {
				if (flag.value != 0)
					count++;
			});
			return count;
		}

		void check_scene_settles(const std::string& name, const std::function<void()>& build) {
			build();
			// up to 3 frames of legitimate cascade: a stage late in one frame's pipeline can only set a
			// flag an earlier stage already walked past this same frame, so it gets picked up next frame.
			for (int i = 0; i < 3; i++) {
				Engine::process_frame(0.f);
			}
			size_t settled_count = count_dirty_entities();
			expect(
				name, settled_count == 0,
				"settles to 0 dirty entities within 3 idle frames after building: still " +
					std::to_string(settled_count) + " dirty [" + list_dirty_entities() + "]"
			);
			// hold it idle for several MORE frames: a genuine perpetual loop keeps re-marking something
			// dirty every single frame forever, not just lingering from the initial build.
			bool stayed_clean = true;
			std::string offender;
			for (int i = 0; i < 5; i++) {
				Engine::process_frame(0.f);
				size_t count = count_dirty_entities();
				if (count != 0) {
					stayed_clean = false;
					offender = "frame " + std::to_string(i) + ": " + std::to_string(count) + " dirty [" +
							   list_dirty_entities() + "]";
					break;
				}
			}
			expect(
				name, stayed_clean,
				"stays at 0 dirty entities for 5 more idle frames (no perpetual re-dirty loop): " + offender
			);
		}
	} // namespace

	void repro_dirty_settle_checks() {
		Log::info("[repro] dirty-flag settle checks (an idle scene must reach, and stay at, 0 dirty entities)");
		dirty_settle_failed = 0;
		auto run = [](const std::string& name, const std::function<void()>& build) {
			try {
				clear_scene();
				Engine::process_frame(0.f); // clean_up from the previous scene is queued until the next frame.
				check_scene_settles(name, build);
			} catch (const std::exception& exception) {
				expect(name, false, std::string("threw: ") + exception.what());
			}
		};
		run("modal", build_modal_scene);
		run("picker", build_picker_scene);
		run("lab", build_lab_scene);
		run("label", build_label_scene);
		run("unicode", build_unicode_scene);
		clear_scene();
		Engine::process_frame(0.f);
		print_pool_diff(get_baseline(), count_pools(), "after dirty-settle checks");
		if (dirty_settle_failed == 0) {
			Log::info("[repro] survived dirty-settle checks, all pass");
		} else {
			Log::info("[repro] dirty-settle checks:", dirty_settle_failed, "failed");
		}
	}

} // namespace Test
