#include "SwapData.h"

namespace FormSwap
{
	namespace detail
	{
		constexpr float PI = static_cast<float>(3.1415926535897932);
		constexpr float TWO_PI = 2.0F * PI;

	    inline float deg_to_rad(float a_degrees)
		{
			return a_degrees * (PI / 180.0f);
		}

		inline float rad_to_deg(float a_radians)
		{
			return a_radians * (180.0f / PI);
		}

	    inline Transform::minMax<float> get_min_max(const std::string& a_str)
		{
			constexpr auto get_float = [](const std::string& str) {
				return string::to_num<float>(str);
			};

			if (const auto splitNum = string::split(a_str, R"(/)"); splitNum.size() > 1) {
				return { get_float(splitNum[0]), get_float(splitNum[1]) };
			} else {
				auto num = get_float(a_str);
				return { num, num };
			}
		}
	}

	Transform::relData<RE::NiPoint3> Transform::get_transform_from_string(const std::string& a_str, bool a_convertToRad)
	{
		minMax<RE::NiPoint3> transformData;

		const auto get_transform = [&](const std::string& b_str) -> RE::NiPoint2 {
			auto [min, max] = detail::get_min_max(b_str);
			return {
				a_convertToRad ? detail::deg_to_rad(min) : min,
				a_convertToRad ? detail::deg_to_rad(max) : max
			};
		};

		if (srell::cmatch match; srell::regex_search(a_str.c_str(), match, transformRegex)) {
			auto [minX, maxX] = get_transform(match[1].str());  //match[0] gets the whole string
			transformData.first.x = minX;
			transformData.second.x = maxX;

			auto [minY, maxY] = get_transform(match[2].str());
			transformData.first.y = minY;
			transformData.second.y = maxY;

			const auto [minZ, maxZ] = get_transform(match[3].str());
			transformData.first.z = minZ;
			transformData.second.z = maxZ;
		}

		return { a_str.contains('R'), transformData };
	}

	Transform::minMax<float> Transform::get_scale_from_string(const std::string& a_str)
	{
		srell::cmatch match;
		if (srell::regex_search(a_str.c_str(), match, genericRegex)) {
			return detail::get_min_max(match[1].str());
		}

		return minMax<float>{ 0.0f, 0.0f };
	}

	float Transform::get_random_value(const Input& a_input, float a_min, float a_max)
	{
		float value = a_min;

		if (!numeric::essentially_equal(a_min, a_max)) {
			value = a_input.trueRandom ? SeedRNG().Generate(a_min, a_max) :
			                             SeedRNG(a_input.refSeed).Generate(a_min, a_max);
		}

		if (a_input.clamp) {
			value = std::clamp(value, a_input.clampMin, a_input.clampMax);
		}

		return value;
	}

	RE::NiPoint3 Transform::get_random_value(const Input& a_input, const std::pair<RE::NiPoint3, RE::NiPoint3>& a_minMax)
	{
		auto& [min, max] = a_minMax;

		if (min == max) {
			return min;
		}

		return RE::NiPoint3{
			get_random_value(a_input, min.x, max.x),
			get_random_value(a_input, min.y, max.y),
			get_random_value(a_input, min.z, max.z)
		};
	}

	Transform::Transform(const std::string& a_str)
	{
		// ignore commas within parentheses
		const auto get_split_transform = [&]() -> std::vector<std::string> {
			srell::sregex_token_iterator iter(a_str.begin(),
				a_str.end(),
				stringRegex,
				-1);
			srell::sregex_token_iterator end{};
			return { iter, end };
		};

		if (clib_util::distribution::is_valid_entry(a_str)) {
			const auto transformStrs = get_split_transform();
			for (auto& transformStr : transformStrs) {
				if (transformStr.contains("pos")) {
					location = get_transform_from_string(transformStr);
				} else if (transformStr.contains("rot")) {
					rotation = get_transform_from_string(transformStr, true);
				} else if (transformStr.contains("scale")) {
					refScale = get_scale_from_string(transformStr);
				}
			}
		}
	}

	void Transform::SetTransform(RE::TESObjectREFR* a_refr) const
	{
		if (location || rotation || refScale) {
			Input input(useTrueRandom, a_refr->GetFormID());
			if (location) {
				auto& [relative, minMax] = *location;
			    const auto [x, y, z] = get_random_value(input, minMax);

			    if (relative) {
					a_refr->data.location.x += x;
					a_refr->data.location.y += y;
					a_refr->data.location.z += z;
				} else {
					a_refr->data.location.x = x;
					a_refr->data.location.y = y;
					a_refr->data.location.z = z;
				}
			}
			if (rotation) {
				input.clamp = true;
				input.clampMin = -detail::TWO_PI;
				input.clampMax = detail::TWO_PI;

				auto& [relative, minMax] = *rotation;
				const auto [x, y, z] = get_random_value(input, minMax);

			    if (relative) {
					a_refr->data.angle.x += x;
					a_refr->data.angle.y += y;
					a_refr->data.angle.z += z;
				} else {
					a_refr->data.angle.x = x;
					a_refr->data.angle.y = y;
					a_refr->data.angle.z = z;
				}
			}
			if (refScale) {
				input.clamp = true;
				input.clampMin = 0.0f;
				input.clampMax = 1000.0f;

				auto& [min, max] = *refScale;
				a_refr->refScale = static_cast<std::uint16_t>(a_refr->refScale * get_random_value(input, min, max));
			}
		}
	}

	bool Transform::IsValid() const
	{
		return location || rotation || refScale;
	}

	Traits::Traits(const std::string& a_str)
	{
		if (clib_util::distribution::is_valid_entry(a_str)) {
			if (a_str.contains("chance")) {
				if (a_str.contains("R")) {
					trueRandom = true;
				}
				if (srell::cmatch match; srell::regex_search(a_str.c_str(), match, genericRegex)) {
					chance = string::to_num<std::uint32_t>(match[1].str());
				}
			}
		}
	}

	TransformData::TransformData(const Input& a_input) :
		transform(a_input.transformStr),
		traits(a_input.traitsStr),
		record(a_input.record),
		path(a_input.path)
	{
		if (traits.trueRandom) {
			transform.useTrueRandom = true;
		}
	}

	SwapData::SwapData(FormIDOrSet a_id, const Input& a_input) :
		TransformData(a_input),
		formIDSet(std::move(a_id))
	{}

	RE::FormID TransformData::GetFormID(const std::string& a_str)
	{
		if (const auto splitID = string::split(a_str, "~"); splitID.size() == 2) {
			const auto formID = string::to_num<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
		}
		if (string::is_only_hex(a_str, true)) {
			if (const auto form = RE::TESForm::GetFormByID(string::to_num<std::uint32_t>(a_str, true))) {
				return form->GetFormID();
			}
		}
		if (const auto form = RE::TESForm::GetFormByEditorID(a_str)) {
			return form->GetFormID();
		}
		return static_cast<RE::FormID>(0);
	}

	void TransformData::GetTransforms(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, TransformData&)> a_func)
	{
		const auto formPair = string::split(a_str, "|");

		if (const auto baseFormID = GetFormID(formPair[0]); baseFormID != 0) {
			const Input input(
				formPair[1],                                        // transform
				formPair.size() > 2 ? formPair[2] : std::string{},  // traits
				a_str,
				a_path);
			TransformData transformData(input);
			a_func(baseFormID, transformData);
		} else {
			logger::error("\t\t\tfailed to process {} (BASE formID not found)", a_str);
		}
	}

	bool TransformData::IsTransformValid(const RE::TESObjectREFR* a_ref) const
	{
		auto seededRNG = SeedRNG(a_ref->GetFormID());

		if (traits.chance != 100) {
			const auto rng = traits.trueRandom ? SeedRNG().Generate<std::uint32_t>(0, 100) :
			                                     seededRNG.Generate<std::uint32_t>(0, 100);
			if (rng > traits.chance) {
				return false;
			}
		}

		return transform.IsValid();
	}

	FormIDOrSet SwapData::GetSwapFormID(const std::string& a_str)
	{
		if (a_str.contains(",")) {
			FormIDSet set;
			const auto IDStrs = string::split(a_str, ",");
			set.reserve(IDStrs.size());
			for (auto& IDStr : IDStrs) {
				if (auto formID = GetFormID(IDStr); formID != 0) {
					set.emplace(formID);
				} else {
					logger::error("\t\t\tfailed to process {} (SWAP formID not found)", IDStr);
				}
			}
			return set;
		} else {
			return GetFormID(a_str);
		}
	}

	RE::TESBoundObject* SwapData::GetSwapBase(const RE::TESObjectREFR* a_ref) const
	{
		auto seededRNG = SeedRNG(a_ref->GetFormID());

		if (traits.chance != 100) {
			const auto rng = traits.trueRandom ? SeedRNG().Generate<std::uint32_t>(0, 100) :
			                                     seededRNG.Generate<std::uint32_t>(0, 100);
			if (rng > traits.chance) {
				return nullptr;
			}
		}

		if (const auto formID = std::get_if<RE::FormID>(&formIDSet); formID) {
			return RE::TESForm::GetFormByID<RE::TESBoundObject>(*formID);
		} else {  // return random element from set
			auto& set = std::get<FormIDSet>(formIDSet);

			const auto setEnd = std::distance(set.begin(), set.end()) - 1;
			const auto randIt = traits.trueRandom ? SeedRNG().Generate<std::int64_t>(0, setEnd) :
			                                        seededRNG.Generate<std::int64_t>(0, setEnd);

			return RE::TESForm::GetFormByID<RE::TESBoundObject>(*std::next(set.begin(), randIt));
		}
	}

	void SwapData::GetForms(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, SwapData&)> a_func)
	{
		const auto formPair = string::split(a_str, "|");

		if (const auto baseFormID = GetFormID(formPair[0]); baseFormID != 0) {
			if (const auto swapFormID = GetSwapFormID(formPair[1]); !swap_empty(swapFormID)) {
				const Input input(
					formPair.size() > 2 ? formPair[2] : std::string{},  // transform
					formPair.size() > 3 ? formPair[3] : std::string{},  // traits
					a_str,
					a_path);
				SwapData swapData(swapFormID, input);
				a_func(baseFormID, swapData);
			} else {
				logger::error("\t\t\tfailed to process {} (SWAP formID not found)", a_str);
			}
		} else {
			logger::error("\t\t\tfailed to process {} (BASE formID not found)", a_str);
		}
	}
}
