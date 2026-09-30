#include "hooks.h"

namespace SyncHairColor::Hooks {
	void Install() {
		// Engine entry trampolines removed:
		// - AttachBipedObject conflicts with skee64's Write6Branch (relative jmp stolen → AV).
		// - UpdateNPCMorphs/UpdateNPCMorph SE IDs were wrongly assumed from UpdateNeck adjacency;
		//   crash showed ID 24209 is called from UpdateNeck (24207) and trampoline jumped to
		//   unmapped memory.
		// Hair tint races after RaceMenu/3D rebuild are handled by NiNodeUpdateEvent + retries.
		SKSE::log::info("Engine trampoline hooks disabled; using NiNodeUpdateEvent sync path");
	}
}
