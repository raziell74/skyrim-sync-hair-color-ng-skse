#pragma once

#include <map>

namespace SyncHairColor::SKEE {
	/** Opaque skee64 plugin interface — we only store pointers, never construct. */
	class IPluginInterface {
	public:
		IPluginInterface() = default;
		virtual ~IPluginInterface();

		virtual std::uint32_t GetVersion();
		virtual void          Save(void* a_intfc, std::uint32_t a_version);
		virtual bool          Load(void* a_intfc, std::uint32_t a_version);
		virtual void          Revert();
	};

	/**
	 * ABI mirror of skee64 IInterfaceMap (inherits std::map like upstream).
	 * Never construct — only used to call into skee's live instance.
	 */
	class IInterfaceMap : public std::map<const char*, IPluginInterface*> {
	public:
		virtual IPluginInterface* QueryInterface(const char* a_name);
		virtual bool              AddInterface(const char* a_name, IPluginInterface* a_interface);
		virtual IPluginInterface* RemoveInterface(const char* a_name);
	};

	struct InterfaceExchangeMessage {
		enum : std::uint32_t {
			kMessage_ExchangeInterface = 0x9E3779B9
		};

		IInterfaceMap* interfaceMap{ nullptr };
	};

	void Install();

	[[nodiscard]] bool              IsAvailable() noexcept;
	[[nodiscard]] IPluginInterface* GetTintMaskInterface() noexcept;
	[[nodiscard]] IPluginInterface* GetOverrideInterface() noexcept;
}
