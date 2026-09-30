#include "event.h"

#include "delay.h"

#include <atomic>

namespace {
	std::atomic<std::uint64_t> g_playerTintDebounceGen{ 0 };

	void ClearIsSyncing(RE::Actor* a_actor) {
		if (!a_actor || SyncHairColor::SyncManager::IsSyncing.empty()) {
			return;
		}
		auto& v = SyncHairColor::SyncManager::IsSyncing;
		v.erase(std::remove(v.begin(), v.end(), a_actor), v.end());
	}
}

namespace SyncHairColor {
	void SyncManager::RequestDebouncedPlayerHairTint() {
		const auto v = ++g_playerTintDebounceGen;
		std::thread([v]() {
			// Quiet window: burst of NPC cell syncs coalesces to one player tint refresh.
			std::this_thread::sleep_for(std::chrono::milliseconds(85));
			if (g_playerTintDebounceGen.load(std::memory_order_acquire) != v) {
				return;
			}
			SKSE::GetTaskInterface()->AddTask([]() {
				if (auto* player = RE::PlayerCharacter::GetSingleton()) {
					ActorUtil::Tint::SyncBipedHairTint(player);
				}
			});
		}).detach();
	}

	namespace {
		void ThunkRequestDebouncedPlayerHairTint() {
			SyncManager::RequestDebouncedPlayerHairTint();
		}
	}

	void SyncManager::RequestActorHairTintSync(
		RE::Actor*   a_actor,
		std::int16_t a_delayMs,
		std::int16_t a_attempts,
		bool         a_debouncePlayerIfNpc) {
		if (!a_actor || a_attempts <= 0) {
			return;
		}
		if (std::find(IsSyncing.begin(), IsSyncing.end(), a_actor) != IsSyncing.end()) {
			return;
		}

		IsSyncing.push_back(a_actor);

		const bool debounce = a_debouncePlayerIfNpc && !a_actor->IsPlayerRef();

		if (a_actor->Get3D(false)) {
			SKSE::GetTaskInterface()->AddTask([a_actor]() {
				ActorUtil::Tint::SyncBipedHairTint(a_actor);
			});
		}

		if (debounce) {
			ActorUtil::Tint::DetachedHairTintSync(a_actor, a_delayMs, a_attempts, &ThunkRequestDebouncedPlayerHairTint);
		} else {
			ActorUtil::Tint::DetachedHairTintSync(a_actor, a_delayMs, a_attempts);
		}

		const auto clearAfterMs = static_cast<std::int32_t>(a_delayMs) * a_attempts + 50;
		std::thread([a_actor, clearAfterMs]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(clearAfterMs));
			SKSE::GetTaskInterface()->AddTask([a_actor]() {
				ClearIsSyncing(a_actor);
			});
		}).detach();
	}

	RE::BSEventNotifyControl NiNodeUpdateEventHandler::ProcessEvent(
		const SKSE::NiNodeUpdateEvent*              a_event,
		RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* a_eventSource) {
		using Result = RE::BSEventNotifyControl;

		if (!a_event || !a_event->reference) {
			return Result::kContinue;
		}

		RE::Actor* actorRef = a_event->reference->As<RE::Actor>();
		if (!actorRef || actorRef->GetFormType() != RE::FormType::NPC) {
			return Result::kContinue;
		}

		SyncManager::RequestActorHairTintSync(actorRef, 150, 3, true);
		return Result::kContinue;
	}

	RE::BSEventNotifyControl EquipEventHandler::ProcessEvent(const TESEquipEvent* a_event, RE::BSTEventSource<TESEquipEvent>* a_eventSource) {
		using Result = RE::BSEventNotifyControl;

		RE::Actor* actorRef = a_event->actor->As<RE::Actor>();
		if (!actorRef) {
			return Result::kContinue;
		}

		if (!actorRef->Get3D(false)) {
			ClearIsSyncing(actorRef);
			return Result::kContinue;
		}

		if (std::find(SyncManager::IsSyncing.begin(), SyncManager::IsSyncing.end(), actorRef) != SyncManager::IsSyncing.end()) {
			return Result::kContinue;
		}

		SKSE::GetTaskInterface()->AddTask([actorRef]() {
			ActorUtil::Tint::SyncBipedHairTint(actorRef);
		});

		SyncManager::IsSyncing.push_back(actorRef);

		// Delayed sync: a burst of equips can apply before the first tint sticks.
		Schedule(std::chrono::milliseconds(250), [actorRef]() {
			SKSE::GetTaskInterface()->AddTask([actorRef]() {
				if (!actorRef->Get3D(false)) {
					if (spdlog::should_log(spdlog::level::debug)) {
						SKSE::log::debug("Actor {} [{}] has no 3D", actorRef->GetActorBase()->GetName(), FormUtil::Form::GetFormConfigString(actorRef));
					}
					ClearIsSyncing(actorRef);
					return;
				}

				ActorUtil::Tint::SyncBipedHairTint(actorRef);
				ClearIsSyncing(actorRef);
			});
		});

		return Result::kContinue;
	}

	RE::BSEventNotifyControl CellAttachEventHandler::ProcessEvent(const TESCellAttachDetachEvent* a_event, RE::BSTEventSource<TESCellAttachDetachEvent>* a_eventSource) {
		using Result = RE::BSEventNotifyControl;

		TESObjectREFR* reference = a_event->reference.get();
		if (!reference) {
			return Result::kContinue;
		}

		RE::Actor* actorRef = reference->As<RE::Actor>();
		if (!actorRef || actorRef->GetFormType() != RE::FormType::NPC) {
			return Result::kContinue;
		}

		if (!actorRef->Get3D(false)) {
			ClearIsSyncing(actorRef);
			return Result::kContinue;
		}

		if (std::find(SyncManager::IsSyncing.begin(), SyncManager::IsSyncing.end(), actorRef) != SyncManager::IsSyncing.end()) {
			return Result::kContinue;
		}

		SyncManager::IsSyncing.push_back(actorRef);

		// Detach tint sync to a thread to ensure Actors 3D is fully loaded
		std::thread([](RE::Actor* actorRef) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			if (!actorRef->Get3D(false)) {
				SKSE::log::debug("Actor {} [{}] has no 3D", actorRef->GetActorBase()->GetName(), FormUtil::Form::GetFormConfigString(actorRef));

				if (!actorRef->IsPlayerRef()) {
					ActorUtil::Tint::DetachedHairTintSync(actorRef, 250, 3, &ThunkRequestDebouncedPlayerHairTint);
				} else {
					ActorUtil::Tint::DetachedHairTintSync(actorRef);
				}

				ClearIsSyncing(actorRef);
				return;
			}

			SKSE::GetTaskInterface()->AddTask([actorRef]() {
				SKSE::log::debug("Delayed sync on Actor {} [{}]", actorRef->GetActorBase()->GetName(), FormUtil::Form::GetFormConfigString(actorRef));
				ActorUtil::Tint::SyncBipedHairTint(actorRef);
				if (!actorRef->IsPlayerRef()) {
					SyncManager::RequestDebouncedPlayerHairTint();
				}
				ClearIsSyncing(actorRef);
			});
		}, actorRef).detach();

		return Result::kContinue;
	}

	RE::BSEventNotifyControl MagicEffectApplyEventHandler::ProcessEvent(const TESActiveEffectApplyRemoveEvent* a_event, RE::BSTEventSource<TESActiveEffectApplyRemoveEvent>* a_eventSource) {
		using Result = RE::BSEventNotifyControl;

		if (!a_event || !a_event->target) {
			return Result::kContinue;
		}

		RE::Actor* actorRef = a_event->target->As<RE::Actor>();
		if (!actorRef || actorRef->GetFormType() != RE::FormType::NPC) {
			return Result::kContinue;
		}

		if (!actorRef->Get3D(false)) {
			ClearIsSyncing(actorRef);
			return Result::kContinue;
		}

		if (std::find(SyncManager::IsSyncing.begin(), SyncManager::IsSyncing.end(), actorRef) != SyncManager::IsSyncing.end()) {
			return Result::kContinue;
		}

		SyncManager::IsSyncing.push_back(actorRef);

		// isApplied == false means the active effect is being removed/fading out.
		if (!a_event->isApplied) {
			std::thread([](RE::Actor* actorRef) {
				std::this_thread::sleep_for(std::chrono::milliseconds(200));

				if (!actorRef->Get3D(false)) {
					ClearIsSyncing(actorRef);
					return;
				}

				SKSE::GetTaskInterface()->AddTask([actorRef]() {
					ActorUtil::Tint::SyncBipedHairTint(actorRef);
					ClearIsSyncing(actorRef);
				});
			}, actorRef).detach();
		} else {
			SKSE::GetTaskInterface()->AddTask([actorRef]() {
				ActorUtil::Tint::SyncBipedHairTint(actorRef);
				ClearIsSyncing(actorRef);
			});
		}

		return Result::kContinue;
	}
}
