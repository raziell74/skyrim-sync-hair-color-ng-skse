#pragma once
#include "util.h"

using namespace RE;
namespace SyncHairColor {
	class SyncManager {
	public:
		inline static std::vector<RE::Actor*> IsSyncing;

		/** Coalesce many NPC-driven requests into one player SyncBipedHairTint after a short quiet window. */
		static void RequestDebouncedPlayerHairTint();

		/**
		 * Shared sync request used by NiNode / engine hooks.
		 * Immediate sync when 3D is ready, plus delayed retries. Optionally debounce player refresh for NPC updates.
		 */
		static void RequestActorHairTintSync(
			RE::Actor*    a_actor,
			std::int16_t  a_delayMs = 150,
			std::int16_t  a_attempts = 3,
			bool          a_debouncePlayerIfNpc = true);
	};

	class EquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent> {
	public:
		static void Install() {
			ScriptEventSourceHolder::GetSingleton()->GetEventSource<TESEquipEvent>()->AddEventSink(GetSingleton());
			SKSE::log::info("Registered {}", typeid(RE::TESEquipEvent).name());
		}
		static EquipEventHandler* GetSingleton() {
			static EquipEventHandler singleton;
			return &singleton;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const TESEquipEvent* a_event, RE::BSTEventSource<TESEquipEvent>* a_eventSource) override;
	};

	class CellAttachEventHandler : public RE::BSTEventSink<RE::TESCellAttachDetachEvent> {
	public:
		static void Install() {
			ScriptEventSourceHolder::GetSingleton()->GetEventSource<TESCellAttachDetachEvent>()->AddEventSink(GetSingleton());
			SKSE::log::info("Registered {}", typeid(RE::TESCellAttachDetachEvent).name());
		}
		static CellAttachEventHandler* GetSingleton() {
			static CellAttachEventHandler singleton;
			return &singleton;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const TESCellAttachDetachEvent* a_event, RE::BSTEventSource<TESCellAttachDetachEvent>* a_eventSource) override;
	};

	class MagicEffectApplyEventHandler : public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent> {
	public:
		static void Install() {
			ScriptEventSourceHolder::GetSingleton()->GetEventSource<TESActiveEffectApplyRemoveEvent>()->AddEventSink(GetSingleton());
			SKSE::log::info("Registered {}", typeid(RE::TESActiveEffectApplyRemoveEvent).name());
		}
		static MagicEffectApplyEventHandler* GetSingleton() {
			static MagicEffectApplyEventHandler singleton;
			return &singleton;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const TESActiveEffectApplyRemoveEvent* a_event, RE::BSTEventSource<TESActiveEffectApplyRemoveEvent>* a_eventSource) override;
	};

	class NiNodeUpdateEventHandler : public RE::BSTEventSink<SKSE::NiNodeUpdateEvent> {
	public:
		static void Install() {
			if (auto* source = SKSE::GetNiNodeUpdateEventSource()) {
				source->AddEventSink(GetSingleton());
				SKSE::log::info("Registered SKSE::NiNodeUpdateEvent");
			} else {
				SKSE::log::warn("NiNodeUpdateEvent source unavailable");
			}
		}
		static NiNodeUpdateEventHandler* GetSingleton() {
			static NiNodeUpdateEventHandler singleton;
			return &singleton;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const SKSE::NiNodeUpdateEvent* a_event, RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* a_eventSource) override;
	};
}
