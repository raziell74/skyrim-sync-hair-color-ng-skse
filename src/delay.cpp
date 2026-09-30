#include "delay.h"

#include <algorithm>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace SyncHairColor {
	namespace {
		struct Item {
			std::chrono::steady_clock::time_point when;
			std::function<void()>                 fn;
		};

		std::mutex              g_mutex;
		std::condition_variable g_cv;
		std::vector<Item>       g_items;
		std::once_flag          g_startOnce;

		void Worker() {
			std::unique_lock lock(g_mutex);
			while (true) {
				if (g_items.empty()) {
					g_cv.wait(lock, [] { return !g_items.empty(); });
				}

				const auto when = std::ranges::min_element(g_items, {}, &Item::when)->when;
				if (g_cv.wait_until(lock, when, [&, when] {
						if (g_items.empty()) {
							return false;
						}
						return std::ranges::min_element(g_items, {}, &Item::when)->when < when;
					})) {
					continue;
				}

				const auto                         now = std::chrono::steady_clock::now();
				std::vector<std::function<void()>> due;
				due.reserve(g_items.size());
				std::erase_if(g_items, [&](Item& item) {
					if (item.when <= now) {
						due.push_back(std::move(item.fn));
						return true;
					}
					return false;
				});

				lock.unlock();
				for (auto& fn : due) {
					try {
						fn();
					} catch (const std::exception& ex) {
						SKSE::log::error("Delay queue task failed: {}", ex.what());
					} catch (...) {
						SKSE::log::error("Delay queue task failed");
					}
				}
				lock.lock();
			}
		}
	}

	void StartDelayQueue() {
		std::call_once(g_startOnce, [] {
			std::thread(Worker).detach();
		});
	}

	void Schedule(std::chrono::milliseconds a_delay, std::function<void()> a_fn) {
		StartDelayQueue();
		{
			std::lock_guard lock(g_mutex);
			g_items.push_back(Item{
				std::chrono::steady_clock::now() + a_delay,
				std::move(a_fn) });
		}
		g_cv.notify_one();
	}
}
