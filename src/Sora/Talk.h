#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace Sora {
	struct OP {
		char op;
		int oprnd;

		static constexpr char SCPSTR_CODE_ITEM = '\x1F';
		static constexpr char SCPSTR_CODE_COLOR = '\x07';
		static constexpr char SCPSTR_CODE_LINE_FEED = '\x01';
		static constexpr char SCPSTR_CODE_ENTER = '\x02';
		static constexpr char SCPSTR_CODE_CLEAR = '\x03';
	};

	struct Line {
		std::string text;
		std::string cmt;
	};
	class Talk;
	class Dialog {
		friend class Talk;

	public:
		using LinesT = std::vector<Line>;
		using OPsT = std::vector<OP>;

		int No() const { return no; }

		const OPsT& Ops() const { return ops; }

		const Talk& Parent() const { return *parent; }
		Talk& Parent() { return *parent; }

		const LinesT& Lines() const { return lines; }
		LinesT& Lines() { return lines; }
		int LinesNum() const { return lines.size(); }
		bool Empty() const { return lines.empty(); }

		Line& operator[] (int index) { return lines[index]; }
		const Line& operator[] (int index) const { return lines[index]; }

		Dialog(Talk& parent, int no, const LinesT& lines = LinesT()) : parent(&parent), no(no), lines(lines) {}
		Dialog(Talk& parent, int no, LinesT&& lines) : parent(&parent), no(no), lines(std::move(lines)) {}
	protected:
		Talk *parent;
		int no;
		LinesT lines;
		OPsT ops;
	};
	using PtrDialog = Dialog*;
	using PtrDialogList = std::vector<PtrDialog>;

	class Talk {
	public:
		enum Type {
			AnonymousTalk = 0,
			ChrTalk,
			NpcTalk,
			InvalidTalk
		};
		static constexpr int NumTalkTypes = (int)Type::InvalidTalk;
		static constexpr Type TypesList[] = { Type::AnonymousTalk, Type::ChrTalk, Type::NpcTalk };
		static constexpr const char* Str_TalkTypes[] = {
			"AnonymousTalk",
			"ChrTalk",
			"NpcTalk",
			"InvalidTalk"
		};
		using DialogsT = std::vector<Dialog>;
		static constexpr int InvalidChrId = 0x80000000;

		static constexpr char Dft_Ignore_List_A[] = { 'J', 'V', 'v', 'B', 'R', 'I' };

		int No() const { return no; }
		Type GetType() const { return type; }
		void SetType(Type type) { this->type = type; }
		int ChrId() const { return chrId; }
		void SetChrId(int chrId) { this->chrId = chrId; }

		std::string& Name() { return name; }
		const std::string& Name() const { return name; }

		DialogsT& Dialogs() { return dialogs; }
		const DialogsT& Dialogs() const { return dialogs; }
		int DialogsNum() const { return dialogs.size(); }
		bool Empty() const { return dialogs.empty(); }

		Dialog& operator[] (int index) { return dialogs[index]; }
		const Dialog& operator[] (int index) const { return dialogs[index]; }

	public:
		Talk(int no, Type type, int chrId = InvalidChrId)
			: no(no), type(type), chrId(chrId),
			dialogs({ Dialog{*this, 0, Dialog::LinesT{ Line{ } } } }) {
		}

		bool HasOp(char ch) const {
			for (const auto& dlg : this->dialogs) {
				for (const auto& op : dlg.Ops()) {
					if (op.op == ch) return true;
				}
			}
			return false;
		}

		template<typename IgnoreList = decltype(Dft_Ignore_List_A)>
		bool Add(const std::string& content, const IgnoreList& ignore_list = Dft_Ignore_List_A) {
			auto & p = content;

			size_t i = 0;
			while (i < p.length()) {
				if (p[i] == '\t') { i++; continue; }

				int bytes = 1;
				OP op{ 0, 0 };
				bool isSymbol = false;
				if (p[i] == '#') {
					if (p[i + 1] == '#') {
						bytes = p.length() - i;
						dialogs.back().lines.back().cmt.append(p.c_str() + i + 2);
						break;
					}
					else {
						while (p[i + bytes] >= '0' && p[i + bytes] <= '9') {
							op.oprnd *= 10;
							op.oprnd += p[i + bytes] - '0';
							bytes++;
						}
						if (bytes == 1
							|| (!(p[i + bytes] >= 'a' && p[i + bytes] <= 'z') && !(p[i + bytes] >= 'A' && p[i + bytes] <= 'Z'))) {
							op.op = '#';
							if (bytes == 1) {
								op.oprnd = 0x80000000;
							}
						}
						else {
							op.op = p[i + bytes++];
						}
					}
				}
				else if (0 <= p[i] && p[i] < 0x20) {
					op.op = p[i];
					switch (op.op) {
					case OP::SCPSTR_CODE_ITEM:
						isSymbol = true;
						bytes += 2;
						op.oprnd = p[2] << 8 | p[1];
						if (i + bytes > p.length()) return false;
						break;
					case OP::SCPSTR_CODE_COLOR:
						isSymbol = true;
						bytes += 1;
						if (i + bytes > p.length()) return false;
						op.oprnd = p[1];
						break;
					case OP::SCPSTR_CODE_ENTER:
						isSymbol = true;
						_flags.newDlg = true;
						break;
					case OP::SCPSTR_CODE_CLEAR: case OP::SCPSTR_CODE_LINE_FEED:
						isSymbol = true;
						_flags.newLine = true;
						break;
					default:
						isSymbol = true;
						break;
					}
				}

				if (!isSymbol) {
					if (_flags.newDlg) {
						dialogs.push_back(Dialog{ *this, (int)dialogs.size(), Dialog::LinesT{ Line{ } } });
						_flags.newDlg = _flags.newLine = false;
					}
					if (_flags.newLine) {
						dialogs.back().lines.push_back(Line{ });
						_flags.newLine = false;
					}
				}

				if (op.op && std::find(std::begin(ignore_list), std::end(ignore_list), op.op) == std::end(ignore_list)) {
					dialogs.back().ops.push_back(op);
				}
				for(; bytes > 0;bytes --) dialogs.back().lines.back().text.push_back(p[i++]);
			}
			return true;
		}


	public:
		Talk(const Talk& _Other)
			: no(_Other.no), type(_Other.type), chrId(_Other.chrId),
			name(_Other.name), dialogs(_Other.dialogs), _flags(_Other._flags) {
			for (auto& dlg : this->dialogs) {
				dlg.parent = this;
			}
		}
		Talk& operator=(const Talk& _Other) {
			this->no = _Other.no;
			this->type = _Other.type;
			this->chrId = _Other.chrId;
			this->name = _Other.name;
			this->dialogs = _Other.dialogs;
			this->_flags = _Other._flags;
			for (auto& dlg : this->dialogs) {
				dlg.parent = this;
			}
			return *this;
		}
		Talk(Talk&& _Right)
			: no(_Right.no), type(_Right.type), chrId(_Right.chrId),
			name(std::move(_Right.name)), dialogs(std::move(_Right.dialogs)), _flags(_Right._flags) {
			for (auto& dlg : this->dialogs) {
				dlg.parent = this;
			}
		}
		Talk& operator=(Talk&& _Right) {
			this->no = _Right.no;
			this->type = _Right.type;
			this->chrId = _Right.chrId;
			this->name = std::move(_Right.name);
			this->dialogs = std::move(_Right.dialogs);
			this->_flags = _Right._flags;
			for (auto& dlg : this->dialogs) {
				dlg.parent = this;
			}
			return *this;
		}

	protected:
		int no;
		Type type;
		int chrId;
		std::string name;
		DialogsT dialogs;

	protected:
		struct {
			bool newLine;
			bool newDlg;
		} _flags{ };
	};
}
