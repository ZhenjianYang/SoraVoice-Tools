#pragma once

#include <string>
#include <ostream>
#include <algorithm>

#include <alg/seq_align.h>
#include <Sora/MBin.h>
#include <Sora/Snt.h>
#include <Sora/Txt.h>
#include <Sora/Talk.h>


namespace Sora {
	namespace TOut {
		using nullptr_t = decltype(nullptr);

		using JudgeVT = int;
		class JudgeOPT {
		public:
			JudgeVT operator()(const OP& a, const OP& b) const {
				if (a.op != b.op) {
					return -50;
				}
				else {
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
				switch (a.op) {
				case 'F': return -10;
				case OP::SCPSTR_CODE_ENTER: return -20;
				case OP::SCPSTR_CODE_COLOR: return a.oprnd == 0 ? 0 : -30;
				case 'S': return -5;
				default: return -3;
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
				if (a.Parent().GetType() == b.Parent().GetType()) {
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

		template <typename PtrDialogsContainer>
		static inline auto GetMatchedDialogs(const PtrDialogsContainer &dialogsA, const PtrDialogsContainer &dialogsB) {
			return alg::seq_align(std::begin(dialogsA), std::end(dialogsA), std::begin(dialogsB), std::end(dialogsB), JudgePtrDialog);
		}

		static inline std::string GetSpOpStr(const Talk& talk) {
			std::string rst;
			bool op5 = talk.HasOp('\x5');
			bool opA = talk.HasOp('A');
			if (op5 || opA) {
				rst = ";;;;------------------";
				if (opA) rst.append(" op#A");
				if (op5) rst.append(" op#5");
			}
			return rst;
		}

		static const std::string NOT_MATCHED_DIALOG = ";NOT_MATCHED_DIALOG";
		static const std::string NOT_MATCHED_TALK = ";NOT_MATCHED_TALK";
		static constexpr int InvalidTalkType = Talk::InvalidTalk;
		static constexpr int InvalidChrId = Talk::InvalidChrId;
		static const Talk EmptyTalk(-1, Talk::InvalidTalk);
		static const Dialog EmptyDialog(const_cast<Talk&>(EmptyTalk), -1, { { NOT_MATCHED_DIALOG } });
		static const Line EmptyLine;
#define ELSE_EMPTY_LINE(output) else output << '\n'

		static inline void OutputTwoPtrDialog(std::ostream& os1, const PtrDialog& pdlg1, std::ostream& os2, const PtrDialog& pdlg2, bool with_cmt = false) {
			const Dialog& dlg1 = pdlg1 ? *pdlg1 : EmptyDialog;
			const Dialog& dlg2 = pdlg2 ? *pdlg2 : EmptyDialog;

			if (dlg1.No() == 0 || dlg2.No() == 0) {
				const Talk& talk1 = dlg1.Parent();
				const Talk& talk2 = dlg2.Parent();

				os1 << '\n';
				os2 << '\n';

				auto spop_str1 = dlg1.No() == 0 ? GetSpOpStr(talk1) : "";
				auto spop_str2 = dlg2.No() == 0 ? GetSpOpStr(talk2) : "";

				std::string empty = "\n\n\n\n";
				if (!spop_str1.empty() || !spop_str2.empty()) {
					empty.push_back('\n');
					spop_str1.push_back('\n');
					spop_str2.push_back('\n');
				}

				if (talk1.GetType() != InvalidTalkType && dlg1.No() == 0) {
					os1 << ";----------------------------------------------------------------------------------\n"
						";----------------------------------------------------------------------------------\n"
						"\n"
						<< spop_str1
						<< Talk::Str_TalkTypes[talk1.GetType()] << " #" << std::dec << talk1.No() << '\n';
				}
				else os1 << empty;
				if (talk2.GetType() != InvalidTalkType && dlg2.No() == 0) {
					os2 << ";----------------------------------------------------------------------------------\n"
						";----------------------------------------------------------------------------------\n"
						"\n"
						<< spop_str2
						<< Talk::Str_TalkTypes[talk2.GetType()] << " #" << std::dec << talk2.No() << '\n';
				}
				else os2 << empty;

				if (talk1.ChrId() != InvalidChrId || talk2.ChrId() != InvalidChrId) {
					if (talk1.ChrId() != InvalidChrId && dlg1.No() == 0) os1 << "0x" << std::hex << std::uppercase << talk1.ChrId() << '\n'; ELSE_EMPTY_LINE(os1);
					if (talk2.ChrId() != InvalidChrId && dlg2.No() == 0) os2 << "0x" << std::hex << std::uppercase << talk2.ChrId() << '\n'; ELSE_EMPTY_LINE(os2);
				}

				if (talk1.GetType() == Talk::NpcTalk || talk2.GetType() == Talk::NpcTalk) {
					if (talk1.GetType() == Talk::NpcTalk && dlg1.No() == 0) os1 << talk1.Name() << '\n'; ELSE_EMPTY_LINE(os1);
					if (talk2.GetType() == Talk::NpcTalk && dlg2.No() == 0) os2 << talk2.Name() << '\n'; ELSE_EMPTY_LINE(os2);
				}
			}

			os1 << '\n';
			os2 << '\n';
			for (int j = 0; j < std::max(dlg1.LinesNum(), dlg2.LinesNum()); j++) {
				const auto& line1 = j < dlg1.LinesNum() ? dlg1[j] : EmptyLine;
				const auto& line2 = j < dlg2.LinesNum() ? dlg2[j] : EmptyLine;

				os1 << Sora::Txt::TalkStr2TxtStr(line1.text) << (with_cmt && !line1.cmt.empty() ? "\t\t\t##" + line1.cmt : "") << '\n';
				os2 << Sora::Txt::TalkStr2TxtStr(line2.text) << (with_cmt && !line2.cmt.empty() ? "\t\t\t##" + line2.cmt : "") << '\n';
			}
		}
	}
}