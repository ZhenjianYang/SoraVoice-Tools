#pragma once

#include <vector>
#include <functional>

namespace alg {
	using nullptr_t = decltype(nullptr);

	template<typename Type1, typename Type2, typename JudgeValueType>
	class StdJudge {
	public:
		using JudgeFun11 = std::function<JudgeValueType(const Type1&, const Type2&)>;
		using JudgeFun01 = std::function<JudgeValueType(const nullptr_t&, const Type2&)>;
		using JudgeFun10 = std::function<JudgeValueType(const Type1&, const nullptr_t&)>;

		const JudgeFun11 judgefun_11;
		const JudgeFun01 judgefun_01;
		const JudgeFun10 judgefun_10;

	public:
		StdJudge(JudgeFun11 fun11, JudgeFun01 fun01, JudgeFun10 fun10) :
			judgefun_11(fun11), judgefun_01(fun01), judgefun_10(fun10) { }

		StdJudge(JudgeFun11 fun11 = [](const Type1& a, const Type2& b) -> JudgeValueType { return a == b ? 1 : 0; },
				JudgeValueType val01 = 0, JudgeValueType val10 = 0) :
			judgefun_11(fun11),
			judgefun_01([val01](const nullptr_t&, const Type2&) { return val01; }),
			judgefun_10([val10](const Type1&, const nullptr_t&) { return val10; }) { }

		JudgeValueType operator()(const Type1& a, const Type2& b) const { return judgefun_11(a, b); };
		JudgeValueType operator()(const nullptr_t& a, const Type2& b) const { return judgefun_01(a, b); }
		JudgeValueType operator()(const Type1& a, const nullptr_t& b) const { return judgefun_10(a, b); }

		static StdJudge get(JudgeFun11 fun11, JudgeFun01 fun01, JudgeFun11 fun10) {
			return StdJudge(fun11, fun01, fun10);
		}
		static StdJudge get(JudgeFun11 fun11 = [](const Type1& a, const Type2& b) -> JudgeValueType { return a == b ? 1 : 0; },
							JudgeValueType val01 = 0, JudgeValueType val10 = 0) {
			return StdJudge(fun11, val01, val10);
		}
	};

	template<typename RandomIt1, typename RandomIt2, typename Judge>
	auto seq_align_value(RandomIt1 it1_begin, RandomIt1 it1_end,
			RandomIt2 it2_begin, RandomIt2 it2_end,
			Judge judge) {
		using Type1 = typename std::iterator_traits<RandomIt1>::value_type;
		using Type2 = typename std::iterator_traits<RandomIt2>::value_type;
		using DiffType1 = decltype(it1_end - it1_begin);
		using DiffType2 = decltype(it2_end - it2_begin);

		using JudgeValueType = typename std::result_of<Judge(const Type1&, const Type2&)>::type;

		const DiffType1 len1 = it1_end - it1_begin;
		const DiffType2 len2 = it2_end - it2_begin;

		std::vector<JudgeValueType> dp((unsigned)len2 + 1);

		dp[len2] = JudgeValueType(0);
		for (DiffType2 j = len2 - 1; j >= 0; j--) {
			dp[j] = dp[j + 1] + judge(nullptr, *(it2_begin + j));
		}

		for (DiffType1 i = len1 - 1; i >= 0; i--) {
			JudgeValueType dp_rd = dp[len2];
			dp[len2] = dp[len2] + judge(*(it1_begin + i), nullptr);

			for (DiffType2 j = len2 - 1; j >= 0; j--) {
				JudgeValueType tmp11 = dp_rd + judge(*(it1_begin + i), *(it2_begin + j));//DownRight
				JudgeValueType tmp01 = dp[j + 1] + judge(nullptr, *(it2_begin + j));//Right
				JudgeValueType tmp10 = dp[j] + judge(*(it1_begin + i), nullptr);//Down

				dp_rd = dp[j];

				dp[j] = tmp11;
				if(tmp01 > dp[j]) dp[j] = tmp01;
				if(tmp10 > dp[j]) dp[j] = tmp10;
			}
		}

		return dp[0];
	}

	template<typename RandomIt1, typename RandomIt2>
		auto seq_align_value(RandomIt1 it1_begin, RandomIt1 it1_end,
				RandomIt2 it2_begin, RandomIt2 it2_end) {
		using Type1 = typename std::iterator_traits<RandomIt1>::value_type;
		using Type2 = typename std::iterator_traits<RandomIt2>::value_type;
		using JudgeValueType = int;

		return seq_align_value(it1_begin, it1_end, it2_begin, it2_end,
				StdJudge<Type1, Type2, JudgeValueType>());
	}

	template<typename RandomIt1, typename RandomIt2, typename Judge>
	auto seq_align(RandomIt1 it1_begin, RandomIt1 it1_end,
			RandomIt2 it2_begin, RandomIt2 it2_end,
			Judge judge) {
		using Type1 = typename std::iterator_traits<RandomIt1>::value_type;
		using Type2 = typename std::iterator_traits<RandomIt2>::value_type;
		using DiffType1 = decltype(it1_end - it1_begin);
		using DiffType2 = decltype(it2_end - it2_begin);

		using JudgeValueType = typename std::result_of<Judge(const Type1&, const Type2&)>::type;

		const DiffType1 len1 = it1_end - it1_begin;
		const DiffType2 len2 = it2_end - it2_begin;

		enum class Move {
			None,
			Down,      //10 (|)
			Right,     //01 (-)
			DownRight, //11 (\)
		};
		std::vector<std::vector<std::pair<JudgeValueType, Move>>> dp{ (unsigned)len1 + 1, { (unsigned)len2 + 1, { JudgeValueType(0), Move::None } } };

		dp[len1][len2] = { 0, Move::None };
		for (DiffType2 j = len2 - 1; j >= 0; j--) {
			dp[len1][j].first = dp[len1][j + 1].first + judge(nullptr, *(it2_begin + j));
			dp[len1][j].second = Move::Right;
		}

		for (DiffType1 i = len1 - 1; i >= 0; i--) {
			dp[i][len2].first = dp[i + 1][len2].first + judge(*(it1_begin + i), nullptr);
			dp[i][len2].second = Move::Down;

			for (DiffType2 j = len2 - 1; j >= 0; j--) {
				dp[i][j].first = dp[i + 1][j + 1].first + judge(*(it1_begin + i), *(it2_begin + j));
				dp[i][j].second = Move::DownRight;

				auto tfirst = dp[i][j + 1].first + judge(nullptr, *(it2_begin + j));
				if (tfirst > dp[i][j].first) {
					dp[i][j].first = tfirst;
					dp[i][j].second = Move::Right;
				}

				tfirst = dp[i + 1][j].first + judge(*(it1_begin + i), nullptr);
				if (tfirst > dp[i][j].first) {
					dp[i][j].first = tfirst;
					dp[i][j].second = Move::Down;
				}
			}
		}

		std::pair<JudgeValueType, std::vector<std::pair<RandomIt1, RandomIt2>>> rst{ dp[0][0].first, { } };
		auto &rst1 = rst.second;

		DiffType1 i = 0;
		DiffType2 j = 0;
		while (i != len1 || j != len2) {
			switch (dp[i][j].second)
			{
			case Move::Down:
				rst1.push_back({ it1_begin + i, it2_end });
				i++;
				break;
			case Move::Right:
				rst1.push_back({ it1_end, it2_begin + j });
				j++;
				break;
			default: //case Move::DownRight:
				rst1.push_back({ it1_begin + i, it2_begin + j });
				i++; j++;
				break;
			}
		}

		return rst;
	}

	template<typename RandomIt1, typename RandomIt2>
	inline auto seq_align(RandomIt1 it1_begin, RandomIt1 it1_end,
			RandomIt2 it2_begin, RandomIt2 it2_end) {
		using Type1 = typename std::iterator_traits<RandomIt1>::value_type;
		using Type2 = typename std::iterator_traits<RandomIt2>::value_type;
		using JudgeValueType = int;

		return seq_align(it1_begin, it1_end, it2_begin, it2_end,
				StdJudge<Type1, Type2, JudgeValueType>());
	}
}
