#pragma once

#include <chrono>
#include <functional>

namespace SyncHairColor {
	/** Starts the process-lifetime worker. Further calls do nothing. */
	void StartDelayQueue();

	/**
	 * Run a_fn on the delay worker after a_delay.
	 * a_fn must not touch game objects; post that work with SKSE::GetTaskInterface().
	 */
	void Schedule(std::chrono::milliseconds a_delay, std::function<void()> a_fn);
}
