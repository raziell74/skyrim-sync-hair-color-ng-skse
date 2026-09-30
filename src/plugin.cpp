#include "log.h"
#include "delay.h"
#include "util.h"
#include "event.h"
#include "skee.h"
#include "hooks.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
	switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			SyncHairColor::EquipEventHandler::GetSingleton()->Install();
			SyncHairColor::CellAttachEventHandler::GetSingleton()->Install();
			SyncHairColor::MagicEffectApplyEventHandler::GetSingleton()->Install();
			SyncHairColor::NiNodeUpdateEventHandler::GetSingleton()->Install();
			break;
		case SKSE::MessagingInterface::kPostLoad:
			SyncHairColor::SKEE::Install();
			break;
		case SKSE::MessagingInterface::kPostPostLoad:
			// Install after skee64 so AttachBipedObject chains through RaceMenu's hook when present.
			SyncHairColor::Hooks::Install();
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			SKSE::log::info("Syncing hair tint for player");
			ActorUtil::Tint::DetachedHairTintSync(RE::TESForm::LookupByID(0x14)->As<RE::Actor>(), 1500, 8);
			break;
		case SKSE::MessagingInterface::kNewGame:
			break;
		case SKSE::MessagingInterface::kSaveGame:
			break;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
	SKSE::Init(skse);
	SetupLog();
	SyncHairColor::StartDelayQueue();

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}

	SKSE::log::info("SyncHairColor Initialized");

	return true;
}
