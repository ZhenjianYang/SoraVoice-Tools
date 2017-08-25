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
				case OP::SCPSTR_CODE_COLOR: return -30;
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

		class JudgeTalkT {
		public:
			JudgeVT operator()(const Talk& a, const Talk& b) const {
				JudgeVT rst = 0;
				if (a.GetType() == b.GetType()) rst += 10;
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

		template <typename TalksContainer>
		static inline auto GetMatchedTalks(const TalksContainer &talksA, const TalksContainer &talksB) {
			return alg::seq_align(std::begin(talksA), std::end(talksA), std::begin(talksB), std::end(talksB), JudgeTalk);
		}

		template <typename PtrDialogsContainer>
		static inline auto GetMatchedDialogs(const PtrDialogsContainer &dialogsA, const PtrDialogsContainer &dialogsB) {
			return alg::seq_align(std::begin(dialogsA), std::end(dialogsA), std::begin(dialogsB), std::end(dialogsB), JudgePtrDialog);
		}

		static constexpr int InvalidTalkType = Talk::InvalidTalk;
		static constexpr int InvalidChrId = Talk::InvalidChrId;
		static const Talk EmptyTalk(-1, Talk::InvalidTalk);
		static const Dialog EmptyDialog(const_cast<Talk&>(EmptyTalk), -1, { ";INVALID_DIALOG" });
		static const std::string EmptyLine;
#define ELSE_EMPTY_LINE(output) else output << '\n'
		static inline void OutputTwoTalks(std::ostream& os1, const Talk& talk1, std::ostream& os2, const Talk& talk2) {
			os1 << '\n';
			os2 << '\n';
			os1 << ";----------------------------------------------------------------------------------\n";
			os2 << ";----------------------------------------------------------------------------------\n";
			os1 << ";----------------------------------------------------------------------------------\n";
			os2 << ";----------------------------------------------------------------------------------\n";

			if (talk1.GetType() != InvalidTalkType)os1 << Talk::Str_TalkTypes[talk1.GetType()] << " #"
				<< std::dec << talk1.No() << '\n';
			else os1 << ";INVALID_TALK\n";
			if (talk2.GetType() != InvalidTalkType) os2 << Talk::Str_TalkTypes[talk2.GetType()] << " #"
				<< std::dec << talk2.No() << '\n';
			else os2 << ";INVALID_TALK\n";

			if (talk1.ChrId() != InvalidChrId || talk2.ChrId() != InvalidChrId) {
				if (talk1.ChrId() != InvalidChrId) os1 << "0x" << std::hex << talk1.ChrId() << '\n'; ELSE_EMPTY_LINE(os1);
				if (talk2.ChrId() != InvalidChrId) os2 << "0x" << std::hex << talk2.ChrId() << '\n'; ELSE_EMPTY_LINE(os2);
			}

			if (talk1.GetType() == Talk::NpcTalk || talk2.GetType() == Talk::NpcTalk) {
				if (talk1.GetType() == Talk::NpcTalk) os1 << talk1.Name() << '\n'; ELSE_EMPTY_LINE(os1);
				if (talk2.GetType() == Talk::NpcTalk) os2 << talk2.Name() << '\n'; ELSE_EMPTY_LINE(os2);
			}

			for (int i = 0; i < std::max(talk1.DialogsNum(), talk2.DialogsNum()); i++) {
				const auto& dlg1 = i < talk1.DialogsNum() ? talk1[i] : EmptyDialog;
				const auto& dlg2 = i < talk2.DialogsNum() ? talk2[i] : EmptyDialog;

				os1 << '\n';
				os2 << '\n';

				for (int j = 0; j < std::max(dlg1.LinesNum(), dlg2.LinesNum()); j++) {
					const auto& line1 = j < dlg1.LinesNum() ? dlg1[j] : EmptyLine;
					const auto& line2 = j < dlg2.LinesNum() ? dlg2[j] : EmptyLine;

					os1 << Sora::Txt::TalkStr2TxtStr(line1) << '\n';
					os2 << Sora::Txt::TalkStr2TxtStr(line2) << '\n';
				}
			}
		}

		static inline void OutputTwoPtrDialog(std::ostream& os1, const PtrDialog& pdlg1, std::ostream& os2, const PtrDialog& pdlg2) {
			const Dialog& dlg1 = pdlg1 ? *pdlg1 : EmptyDialog;
			const Dialog& dlg2 = pdlg2 ? *pdlg2 : EmptyDialog;

			if (dlg1.No() == 0 || dlg2.No() == 0) {
				const Talk& talk1 = dlg1.Parent();
				const Talk& talk2 = dlg2.Parent();

				os1 << '\n';
				os2 << '\n';

				if (talk1.GetType() != InvalidTalkType && dlg1.No() == 0) {
					os1 << ";----------------------------------------------------------------------------------\n"
						";----------------------------------------------------------------------------------\n"
						"\n"
						<< Talk::Str_TalkTypes[talk1.GetType()] << " #" << std::dec << talk1.No() << '\n';
				}
				else os1 << "\n\n\n\n";
				if (talk2.GetType() != InvalidTalkType && dlg2.No() == 0) {
					os2 << ";----------------------------------------------------------------------------------\n"
						";----------------------------------------------------------------------------------\n"
						"\n"
						<< Talk::Str_TalkTypes[talk2.GetType()] << " #" << std::dec << talk2.No() << '\n';
				}
				else os2 << "\n\n\n\n";

				if (talk1.ChrId() != InvalidChrId || talk2.ChrId() != InvalidChrId) {
					if (talk1.ChrId() != InvalidChrId && dlg1.No() == 0) os1 << "0x" << std::hex << talk1.ChrId() << '\n'; ELSE_EMPTY_LINE(os1);
					if (talk2.ChrId() != InvalidChrId && dlg2.No() == 0) os2 << "0x" << std::hex << talk2.ChrId() << '\n'; ELSE_EMPTY_LINE(os2);
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

				os1 << Sora::Txt::TalkStr2TxtStr(line1) << '\n';
				os2 << Sora::Txt::TalkStr2TxtStr(line2) << '\n';
			}
		}
	}
}