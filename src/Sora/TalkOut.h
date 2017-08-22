#pragma once

#include <string>
#include <ostream>
#include <algorithm>

#include <alg/seq_align.h>
#include <Sora/MBin.h>
#include <Sora/Snt.h>
#include <Sora/Talk.h>

namespace TOut {
	static inline std::string TalkStr2OutStr(const std::string& str) {
		std::string rst;
		char buff[12];
		for (char c : str) {
			if (c >= 0x00 && c < 0x20) {
				sprintf(buff, "\\x%02X", (int)c);
				rst.append(buff);
			}
			else {
				rst.push_back(c);
			}
		}
		return rst;
	}

	using JudgeVT = int;
	static inline JudgeVT JudgeOP11(const Talk::OP& a, const Talk::OP& b) {
		JudgeVT rst = 0;
		if (a.op != b.op) {
			rst = -5;
		}
		else {
			switch (a.op)
			{
			case 'F':
				rst += a.oprnd == b.oprnd ? 15 : -15;
				break;
			case '\x2':
				rst += a.oprnd == b.oprnd ? 10 : -10;
				break;
			case 'W':case 'A':
				rst += a.oprnd == b.oprnd ? 8 : 4;
				break;
			default:
				rst += a.oprnd == b.oprnd ? 5 : 0;
				break;
				break;
			}
		}
		return rst;
	}
	static inline JudgeVT JudgeTalk11(const Talk& a, const Talk& b) {
		JudgeVT rst = 0;
		if (a.Type() == b.Type()) rst += 10;
		if (a.ChrId() == b.ChrId()) rst += 5;
		if (a.DialogsNum() == b.DialogsNum()) rst += 50;

		static const auto judgeOp = alg::StdJudge<Talk::OP, Talk::OP, JudgeVT>(JudgeOP11);
		rst += alg::seq_align_value(a.OPs().begin(), a.OPs().end(), b.OPs().begin(), b.OPs().end(), judgeOp);
		return rst;
	}

	static inline auto GetMatchedTalks(const TalksT &talksA, const TalksT &talksB) {
		static const auto judgeTalk = alg::StdJudge<Talk, Talk, JudgeVT>(JudgeTalk11);
		return alg::seq_align(talksA.begin(), talksA.end(), talksB.begin(), talksB.end(), judgeTalk);
	}

	static constexpr int InvalidTalkType = -1;
	static constexpr int InvalidChrId = Talk::InvalidChrId;
	static const Talk EmptyTalk(InvalidTalkType);
	static const Talk::DialogT EmptyDialog;
	static const Talk::LineT EmptyLine;
#define ELSE_EMPTY_LINE(output) else output << '\n'
	static inline void OutputTwoTalks(std::ostream& os1, const Talk& talk1, std::ostream& os2, const Talk& talk2) {
		os1 << '\n';
		os2 << '\n';

		if (talk1.Type() != InvalidTalkType) os1 << Talk::Str_TalkTypes[talk1.Type()] << '\n'; ELSE_EMPTY_LINE(os1);
		if (talk2.Type() != InvalidTalkType) os2 << Talk::Str_TalkTypes[talk2.Type()] << '\n'; ELSE_EMPTY_LINE(os2);

		if (talk1.ChrId() != InvalidChrId || talk2.ChrId() != InvalidChrId) {
			if (talk1.ChrId() != InvalidChrId) os1 << "0x" << std::hex << talk1.ChrId() << '\n'; ELSE_EMPTY_LINE(os1);
			if (talk2.ChrId() != InvalidChrId) os2 << "0x" << std::hex << talk2.ChrId() << '\n'; ELSE_EMPTY_LINE(os2);
		}

		if (talk1.Type() == Talk::NpcTalk || talk2.Type() == Talk::NpcTalk) {
			if (talk1.Type() != InvalidChrId) os1 << "0x" << std::hex << talk1.ChrId() << '\n'; ELSE_EMPTY_LINE(os1);
			if (talk2.Type() != InvalidChrId) os2 << "0x" << std::hex << talk2.ChrId() << '\n'; ELSE_EMPTY_LINE(os2);
		}

		os1 << '\n';
		os2 << '\n';

		for (int i = 0; i < std::max(talk1.DialogsNum(), talk2.DialogsNum()); i++) {
			const auto& dlg1 = i < talk1.DialogsNum() ? talk1[i] : EmptyDialog;
			const auto& dlg2 = i < talk2.DialogsNum() ? talk2[i] : EmptyDialog;

			
			for (size_t j = 0; j < std::max(dlg1.size(), dlg2.size()); j++) {
				const auto& line1 = j < dlg1.size() ? dlg1[j] : EmptyLine;
				const auto& line2 = j < dlg2.size() ? dlg2[j] : EmptyLine;

				os1 << TOut::TalkStr2OutStr(line1) << '\n';
				os2 << TOut::TalkStr2OutStr(line2) << '\n';
			}
			os1 << '\n';
			os2 << '\n';
		}

		os1 << ";----------------------------------------------------------------------------------\n";
		os2 << ";----------------------------------------------------------------------------------\n";
		os1 << ";----------------------------------------------------------------------------------\n";
		os2 << ";----------------------------------------------------------------------------------\n";
	}
}
