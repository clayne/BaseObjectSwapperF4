#pragma once

#include "SwapData.h"

namespace FormSwap
{
	class Manager : public ISingleton<Manager>
	{
	public:
		void LoadFormsOnce();

		void PrintConflicts() const;

		SwapFormResult GetSwapData(RE::TESObjectREFR* a_ref, const RE::TESForm* a_base, const RE::BGSMaterialSwap* a_materialSwap);

		SwapFormResult GetSwapFormConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base, const RE::BGSMaterialSwap* a_materialSwap);
		std::optional<ObjectProperties> GetObjectPropertiesConditional(const RE::TESObjectREFR* a_ref, const RE::TESForm* a_base, const RE::BGSMaterialSwap* a_materialSwap);

		void InsertLeveledItemRef(const RE::TESObjectREFR* a_refr);
		bool IsLeveledItemRefSwapped(const RE::TESObjectREFR* a_refr) const;

	private:
		void LoadForms();

		// members
		FormIDMap<SwapFormDataVec> swapRefs{};
		FormIDMap<std::vector<SwapFormDataConditional>> swapFormsConditional{};
		FormIDMap<SwapFormDataVec> swapForms{};

		FormIDMap<ObjectDataVec> refProperties{};
		FormIDMap<std::vector<ObjectDataConditional>> refPropertiesConditional{};

		Set<RE::FormID> swappedLeveledItemRefs{};

		bool hasConflicts{ false };
		std::once_flag init{};
	};
}
