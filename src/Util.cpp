#include "Util.h"

namespace util
{
	std::vector<std::string> split_with_regex(const std::string& a_str, const srell::regex& a_regex)
	{
		srell::sregex_token_iterator iter(a_str.begin(),
			a_str.end(),
			a_regex,
			-1);
		srell::sregex_token_iterator end{};
		return { iter, end };
	}

	RE::FormID GetFormID(const std::string& a_str)
	{
		if (const auto splitID = string::split(a_str, "~"); splitID.size() == 2) {
			const auto formID = string::to_num<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
		}
		if (string::is_only_hex(a_str, true)) {
			const auto formID = string::to_num<RE::FormID>(a_str, true);
			if (const auto form = RE::TESForm::GetFormByID(formID); !form) {
				logger::error("\t\tFilter [{}] INFO - unable to find form, treating filter as cell formID", a_str);
			}
			return formID;
		}
		if (const auto form = RE::TESForm::GetFormByEditorID(a_str)) {
			return form->GetFormID();
		}
		return static_cast<RE::FormID>(0);
	}

	FormIDOrSet GetSwapFormID(const std::string& a_str)
	{
		if (a_str.contains(",")) {
			FormIDSet set;
			const auto IDStrs = string::split(a_str, ",");
			set.reserve(IDStrs.size());
			for (auto& IDStr : IDStrs) {
				if (auto formID = GetFormID(IDStr); formID != 0) {
					set.emplace(formID);
				} else {
					logger::error("\t\t\tfail : [{}] (SWAP formID not found)", IDStr);
				}
			}
			return set;
		} else {
			return GetFormID(a_str);
		}
	}
}
