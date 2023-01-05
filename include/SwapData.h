#pragma once

namespace FormSwap
{
	using FormIDStr = std::variant<RE::FormID, std::string>;

	template <class K, class D>
	using Map = robin_hood::unordered_flat_map<K, D>;
	template <class T>
	using Set = robin_hood::unordered_flat_set<T>;

	using FormIDSet = Set<RE::FormID>;
	using FormIDOrSet = std::variant<RE::FormID, FormIDSet>;
	inline bool swap_empty(const FormIDOrSet& a_set)
	{
		if (const auto formID = std::get_if<RE::FormID>(&a_set); formID) {
			return *formID == 0;
		} else {
			return std::get<FormIDSet>(a_set).empty();
		}
	}

	inline srell::regex genericRegex{ R"(\((.*?)\))" };

	inline SeedRNG staticRNG;

	class Transform
	{
	public:
		template <class T>
		using minMax = std::pair<T, T>;
		template <class T>
		using relData = std::pair<bool, minMax<T>>;  //relative vs absolute

		Transform() = default;
		explicit Transform(const std::string& a_str);

		void SetTransform(RE::TESObjectREFR* a_refr) const;
		bool IsValid() const;

		bool operator==(Transform const& a_rhs) const
		{
			return location && a_rhs.location || rotation && a_rhs.location || refScale && a_rhs.refScale;
		}

	private:
		[[nodiscard]] static relData<RE::NiPoint3> get_transform_from_string(const std::string& a_str, bool a_convertToRad = false);
		[[nodiscard]] static minMax<float> get_scale_from_string(const std::string& a_str);

		struct Input
		{
			bool trueRandom{ false };
			RE::FormID refSeed{ 0 };

			bool clamp{ false };
			float clampMin{ 0.0f };
			float clampMax{ 0.0f };
		};

		static float get_random_value(const Input& a_input, float a_min, float a_max);
		static RE::NiPoint3 get_random_value(const Input& a_input, const std::pair<RE::NiPoint3, RE::NiPoint3>& a_minMax);

		// members
		std::optional<relData<RE::NiPoint3>> location{ std::nullopt };
		std::optional<relData<RE::NiPoint3>> rotation{ std::nullopt };
		std::optional<minMax<float>> refScale{ std::nullopt };

		bool useTrueRandom{ false };

		static inline srell::regex transformRegex{ R"(\((.*?),(.*?),(.*?)\))" };
		static inline srell::regex stringRegex{ R"(,\s*(?![^()]*\)))" };

		friend class TransformData;
	};

	struct Traits
	{
		Traits() = default;
		explicit Traits(const std::string& a_str);

		// members
		bool trueRandom{ false };
		std::uint32_t chance{ 100 };
	};

	class TransformData
	{
	public:
		struct Input
		{
			std::string transformStr;
			std::string traitsStr;
			std::string record;
			std::string path;
		};

		TransformData() = delete;
		explicit TransformData(const Input& a_input);

		[[nodiscard]] static RE::FormID GetFormID(const std::string& a_str);
		bool IsTransformValid(const RE::TESObjectREFR* a_ref) const;

		static void GetTransforms(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, TransformData&)> a_func);

		// members
		Transform transform{};
		Traits traits{};

		std::string record{};
		std::string path{};
	};

	class SwapData : public TransformData
	{
	public:
		SwapData() = delete;
		SwapData(FormIDOrSet a_id, const Input& a_input);

		[[nodiscard]] static FormIDOrSet GetSwapFormID(const std::string& a_str);
		RE::TESBoundObject* GetSwapBase(const RE::TESObjectREFR* a_ref) const;

		static void GetForms(const std::string& a_path, const std::string& a_str, std::function<void(RE::FormID, SwapData&)> a_func);

		// members
		FormIDOrSet formIDSet{};
	};
}
