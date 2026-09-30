#include "skee.h"

namespace SyncHairColor::SKEE {
	namespace {
		IPluginInterface* g_tintMask{ nullptr };
		IPluginInterface* g_override{ nullptr };
		bool              g_available{ false };
	}

	// Definitions exist only so the mirror types link; they are never invoked on our side.
	IPluginInterface::~IPluginInterface() = default;
	std::uint32_t IPluginInterface::GetVersion() { return 0; }
	void          IPluginInterface::Save(void*, std::uint32_t) {}
	bool          IPluginInterface::Load(void*, std::uint32_t) { return false; }
	void          IPluginInterface::Revert() {}

	IPluginInterface* IInterfaceMap::QueryInterface(const char*) { return nullptr; }
	bool              IInterfaceMap::AddInterface(const char*, IPluginInterface*) { return false; }
	IPluginInterface* IInterfaceMap::RemoveInterface(const char*) { return nullptr; }

	void Install() {
		g_tintMask = nullptr;
		g_override = nullptr;
		g_available = false;

		auto* messaging = SKSE::GetMessagingInterface();
		if (!messaging) {
			SKSE::log::warn("SKEE exchange skipped: no messaging interface");
			return;
		}

		InterfaceExchangeMessage exchange{};
		messaging->Dispatch(
			InterfaceExchangeMessage::kMessage_ExchangeInterface,
			&exchange,
			sizeof(exchange),
			"skee");

		if (!exchange.interfaceMap) {
			SKSE::log::info("skee64 not present (or interface exchange failed)");
			return;
		}

		g_tintMask = exchange.interfaceMap->QueryInterface("TintMask");
		g_override = exchange.interfaceMap->QueryInterface("Override");
		g_available = g_tintMask || g_override;

		SKSE::log::info(
			"skee64 interfaces: TintMask={} Override={}",
			g_tintMask != nullptr,
			g_override != nullptr);
	}

	bool IsAvailable() noexcept {
		return g_available;
	}

	IPluginInterface* GetTintMaskInterface() noexcept {
		return g_tintMask;
	}

	IPluginInterface* GetOverrideInterface() noexcept {
		return g_override;
	}
}
