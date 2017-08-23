#pragma once

#include <string>
#include <ostream>
#include <algorithm>

#include <alg/seq_align.h>
#include <Sora/MBin.h>
#include <Sora/Snt.h>
#include <Sora/Talk.h>

namespace TOut {
	using nullptr_t = decltype(nullptr);
	static inline std::string TalkStr2OutStr(const std::string& str) {
		std::string rst;
		char buff[16];
		size_t i = 0;
		using byte = unsigned char;
		while (i < str.length()) {
			if(str[i] == OP::SCPSTR_CODE_ITEM) {
				std::sprintf(buff, "\\x%02X\\x%02X\\x%02X", (byte)str[i], (byte)str[i+1], (byte)str[i+2]);
				rst.append(buff);
				i += 3;
			} else if (str[i] == OP::SCPSTR_CODE_COLOR) {
				std::sprintf(buff, "\\x%02X\\x%02X", (byte)str[i], (byte)str[i+1]);
				rst.append(buff);
				i += 2;
			} else if (str[i] >= 0 && str[i] < 0x20) {
				std::sprintf(buff, "\\x%02X", (byte)str[i]);
				rst.append(buff);
				i += 1;
			} else  {
				rst.push_back(str[i]);
				i += 1;
			}
		}
		return rst;
	}

	using JudgeVT = int;
	class JudgeOPT {
	public:
		JudgeVT operator()(const OP& a, const OP& b) const {
			if(a.op != b.op) {
				return -50;
			} else {
				switch (a.op)
				{
				case 'F':
					return a.oprnd == b.oprnd ? 15 : -15;
				case OP::SCPSTR_CODE_ENTER:
					return 10;
				case OP::SCPSTR_CODE_CLEAR:
					return 15;
				case OP::SCPSTR_CODE_COLOR:
					return a.oprnd == b.oprnd ? 20 : -20;
				case 'W':case 'A':
					return a.oprnd == b.oprnd ? 8 : 4;
				default:
					return a.oprnd == b.oprnd ? 5 : 0;
				}
			}
		}
		JudgeVT operator()(const OP& a, const nullptr_t&) const {
			switch(a.op) {
			case 'F': return -10;
			case OP::SCPSTR_CODE_ENTER : return -20;
			case OP::SCPSTR_CODE_COLOR : return -30;
			case 'S': return -5;
			default : return -3;
			}
		}
		JudgeVT operator()(const nullptr_t&, const OP& b) const {
			return operator()(b, nullptr);
		}
	};
	static const JudgeOPT JudgeOP;

	class JudgeDialogT {
	public:
		JudgeVT operator()(const Dialog& a, const Dialog& b) const {
			JudgeVT rst = 0;
			if (a.Parent().Type() == b.Parent().Type()) {
				rst += 5;
			}
			if (a.Parent().ChrId() == b.Parent().ChrId()) {
				rst += 10;
			}
			if (a.No() == b.No()) {
				rst += 2;
			}

			rst += alg::seq_align_value(a.Ops().begin(), a.Ops().end(), b.Ops().begin(), b.Ops().end(), JudgeOP);
			return rst;
		}
		JudgeVT operator()(const Dialog& a, const nullptr_t&) const {
			return -30;
		}
		JudgeVT operator()(const nullptr_t&, const Dialog& b) const {
			return operator()(b, nullptr);
		}
	};
	static const JudgeDialogT JudgeDialog;
	class JudgePtrDialogT {
	public:
		JudgeVT operator()(const PtrDialog& a, const PtrDialog& b) const {
			return JudgeDialog(*a, *b);
		}
		JudgeVT operator()(const PtrDialog& a, const nullptr_t&) const {
			return JudgeDialog(*a, nullptr);
		}
		JudgeVT operator()(const nullptr_t&, const PtrDialog& b) const {
			return operator()(b, nullptr);
		}
	};
	static const JudgePtrDialogT JudgePtrDialog;

	class JudgeTalkT {
	public:
		JudgeVT operator()(const Talk& a, const Talk& b) const {
			JudgeVT rst = 0;
			if (a.Type() == b.Type()) rst += 10;
			if (a.ChrId() == b.ChrId()) rst += 5;
			if (a.DialogsNum() == b.DialogsNum()) rst += 30;

			rst += alg::seq_align_value(a.Dialogs().begin(), a.Dialogs().end(), b.Dialogs().begin(), b.Dialogs().end(), JudgeDialog);
			return rst;
		}
		JudgeVT operator()(const Talk& a, const nullptr_t&) const {
			return -200;
		}
		JudgeVT operator()(const nullptr_t&, const Talk& b) const {
			return operator()(b, nullptr);
		}
	};
	static const JudgeTalkT JudgeTalk;

	static inline auto GetMatchedTalks(const TalksT &talksA, const TalksT &talksB) {
		return alg::seq_align(talksA.begin(), talksA.end(), talksB.begin(), talksB.end(), JudgeTalk);
	}

	static inline auto GetMatchedDialogs(const PtrDialogList &dialogsA, const PtrDialogList &dialogsB) {
		return alg::seq_align(dialogsA.begin(), dialogsA.end(), dialogsB.begin(), dialogsB.end(), JudgePtrDialog);
	}

	static constexpr int InvalidTalkType = Talk::InvalidTalk;
	static constexpr int InvalidChrId = Talk::InvalidChrId;
	static const Talk EmptyTalk(-1, Talk::InvalidTalk);
	static const Dialog EmptyDialog(const_cast<Talk&>(EmptyTalk), -1, {";INVALID_DIALOG"});
	static const std::string EmptyLine;
#define ELSE_EMPTY_LINE(output) else output << '\n'
	static inline void OutputTwoTalks(std::ostream& os1, const Talk& talk1, std::ostream& os2, const Talk& talk2) {
		os1 << '\n';
		os2 << '\n';
		os1 << ";----------------------------------------------------------------------------------\n";
		os2 << ";----------------------------------------------------------------------------------\n";
		os1 << ";----------------------------------------------------------------------------------\n";
		os2 << ";----------------------------------------------------------------------------------\n";

		if (talk1.Type() != InvalidTalkType)os1 << Talk::Str_TalkTypes[talk1.Type()] << " #"
				<< std::dec << talk1.No() << '\n';
		else os1 << ";INVALID_TALK\n";
		if (talk2.Type() != InvalidTalkType) os2 << Talk::Str_TalkTypes[talk2.Type()] << " #"
				<< std::dec << talk2.No() << '\n';
		else os2 << ";INVALID_TALK\n";

		if (talk1.ChrId() != InvalidChrId || talk2.ChrId() != InvalidChrId) {
			if (talk1.ChrId() != InvalidChrId) os1 << "0x" << std::hex << talk1.ChrId() << '\n'; ELSE_EMPTY_LINE(os1);
			if (talk2.ChrId() != InvalidChrId) os2 << "0x" << std::hex << talk2.ChrId() << '\n'; ELSE_EMPTY_LINE(os2);
		}

		if (talk1.Type() == Talk::NpcTalk || talk2.Type() == Talk::NpcTalk) {
			if (talk1.Type() == Talk::NpcTalk) os1 << talk1.Name() << '\n'; ELSE_EMPTY_LINE(os1);
			if (talk2.Type() == Talk::NpcTalk) os2 << talk2.Name() << '\n'; ELSE_EMPTY_LINE(os2);
		}

		for (int i = 0; i < std::max(talk1.DialogsNum(), talk2.DialogsNum()); i++) {
			const auto& dlg1 = i < talk1.DialogsNum() ? talk1[i] : EmptyDialog;
			const auto& dlg2 = i < talk2.DialogsNum() ? talk2[i] : EmptyDialog;

			os1 << '\n';
			os2 << '\n';

			for (int j = 0; j < std::max(dlg1.LinesNum(), dlg2.LinesNum()); j++) {
				const auto& line1 = j < dlg1.LinesNum() ? dlg1[j] : EmptyLine;
				const auto& line2 = j < dlg2.LinesNum() ? dlg2[j] : EmptyLine;

				os1 << TOut::TalkStr2OutStr(line1) << '\n';
				os2 << TOut::TalkStr2OutStr(line2) << '\n';
			}
		}
	}

	static inline void OutputTwoPtrDialog(std::ostream& os1, const PtrDialog& pdlg1, std::ostream& os2, const PtrDialog& pdlg2) {
		const Dialog& dlg1 = pdlg1 ? *pdlg1 : EmptyDialog;
		const Dialog& dlg2 = pdlg2 ? *pdlg2 : EmptyDialog;

		if(dlg1.No() == 0 || dlg2.No() == 0) {
			const Talk& talk1 = dlg1.Parent();
			const Talk& talk2 = dlg2.Parent();

			os1 << '\n';
			os2 << '\n';

			if (talk1.Type() != InvalidTalkType && dlg1.No() == 0) {
				os1 << ";----------------------------------------------------------------------------------\n"
					   ";----------------------------------------------------------------------------------\n"
					   "\n"
					<< Talk::Str_TalkTypes[talk1.Type()] << " #" << std::dec << talk1.No()  << '\n';
			} else os1 << "\n\n\n\n";
			if (talk2.Type() != InvalidTalkType && dlg2.No() == 0) {
				os2 << ";----------------------------------------------------------------------------------\n"
					   ";----------------------------------------------------------------------------------\n"
					   "\n"
					<< Talk::Str_TalkTypes[talk2.Type()] << " #" << std::dec << talk2.No()  << '\n';
			} else os2 << "\n\n\n\n";

			if (talk1.ChrId() != InvalidChrId || talk2.ChrId() != InvalidChrId) {
				if (talk1.ChrId() != InvalidChrId && dlg1.No() == 0) os1 << "0x" << std::hex << talk1.ChrId() << '\n'; ELSE_EMPTY_LINE(os1);
				if (talk2.ChrId() != InvalidChrId && dlg2.No() == 0) os2 << "0x" << std::hex << talk2.ChrId() << '\n'; ELSE_EMPTY_LINE(os2);
			}

			if (talk1.Type() == Talk::NpcTalk || talk2.Type() == Talk::NpcTalk) {
				if (talk1.Type() == Talk::NpcTalk) os1 << talk1.Name() << '\n'; ELSE_EMPTY_LINE(os1);
				if (talk2.Type() == Talk::NpcTalk) os2 << talk2.Name() << '\n'; ELSE_EMPTY_LINE(os2);
			}
		}

		os1 << '\n';
		os2 << '\n';
		for (int j = 0; j < std::max(dlg1.LinesNum(), dlg2.LinesNum()); j++) {
			const auto& line1 = j < dlg1.LinesNum() ? dlg1[j] : EmptyLine;
			const auto& line2 = j < dlg2.LinesNum() ? dlg2[j] : EmptyLine;

			os1 << TOut::TalkStr2OutStr(line1) << '\n';
			os2 << TOut::TalkStr2OutStr(line2) << '\n';
		}
	}

	static inline void OutputTalk(std::ostream& os, const Talk& talk) {
		os << '\n';
		os << ";----------------------------------------------------------------------------------\n";
		os << ";----------------------------------------------------------------------------------\n";
		os << '\n';

		if (talk.Type() != InvalidTalkType)os << Talk::Str_TalkTypes[talk.Type()] << " #"
			<< std::dec << talk.No() << '\n';
		else os << ";INVALID_TALK\n";

		if (talk.ChrId() != InvalidChrId) os << "0x" << std::hex << talk.ChrId() << '\n';
		if (talk.Type() == Talk::NpcTalk) os << talk.Name() << '\n';

		for (const auto& dlg : talk.Dialogs()) {
			os << '\n';
			for (const auto& line : dlg.Lines()) {
				os << TOut::TalkStr2OutStr(line) << '\n';
			}
		}
	}
}
