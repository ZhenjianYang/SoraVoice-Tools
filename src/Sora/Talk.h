#pragma once

#include <string>
#include <vector>
#include <functional>

class Talk {
public:
	enum TalkType {
		AnonymousTalk = 0,
		ChrTalk,
		NpcTalk
	};
	static constexpr int NumTalkTypes = NpcTalk + 1;
	static constexpr const char* Str_TalkTypes[] = {
		"AnonymousTalk",
		"ChrTalk",
		"NpcTalk",
	};

	struct OP {
		char op;
		int oprnd;
	};
	using LineT = std::string;
	using DialogT = std::vector<LineT>;
	using DialogsT = std::vector<DialogT>;
	using OPsT = std::vector<OP>;

	static constexpr int InvalidChrId = 0x80000000;
	static constexpr char SCPSTR_CODE_ITEM = '\x1F';
	static constexpr char SCPSTR_CODE_COLOR = '\x07';
	static constexpr char SCPSTR_CODE_LINE_FEED = '\x01';
	static constexpr char SCPSTR_CODE_ENTER = '\x02';
	static constexpr char SCPSTR_CODE_CLEAR = '\x03';

	int Type() const { return type; }
	void SetType(int type) { this->type = type; }
	int ChrId() const { return chrId; }
	void SetChrId(int chrId) { this->chrId = chrId; }

	std::string& Name() { return name; }
	const std::string& Name() const { return name; }
	DialogsT& Dialogs() { return dialogs; }
	const DialogsT& Dialogs() const { return dialogs; }
	OPsT& OPs() { return ops; }
	const OPsT& OPs() const { return ops; }

	int DialogsNum() const { return dialogs.size(); }
	DialogT& operator[] (int index) { return dialogs[index]; }
	const DialogT& operator[] (int index) const { return dialogs[index]; }

public:
	Talk(int type, int chrId = InvalidChrId)
		: type(type), chrId(chrId),
		  dialogs({{""}}){
	}

	bool Add(const std::string& content, std::function<int(const char*)>getChbytes) {
		auto & p = content;

		size_t i = 0;
		while(i < p.length()) {
			if (p[i] == '\t') { i++; continue; }

			int bytes = getChbytes(p.c_str() + i);
			if(bytes <= 0) return false;

			bool isSymbol = false;
			if(p[i] == '#') {
				if (p[i + 1] == '#') {
					bytes = p.length() - i;
					break;
				}
				else {
					OP op{ 0, 0 };
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
					ops.push_back(op);
				}
			}
			else if(0 <= p[i] && p[i] < 0x20) {
				OP op { p[i], 0 };

				switch(op.op) {
				case SCPSTR_CODE_ITEM:
					isSymbol = true;
					bytes += 2;
					op.oprnd = p[2] << 8 | p[1];
					break;
				case SCPSTR_CODE_COLOR:
					isSymbol = true;
					bytes += 1;
					op.oprnd = p[1];
					break;
				case SCPSTR_CODE_ENTER:
					isSymbol = true;
					_flags.newDlg = true;
					break;
				case SCPSTR_CODE_CLEAR: case SCPSTR_CODE_LINE_FEED:
					isSymbol = true;
					_flags.newLine = true;
					break;
				default:
					isSymbol = true;
					break;
				}
				ops.push_back(op);
			}

			if (!isSymbol) {
				if (_flags.newDlg) { dialogs.push_back({ "" }); _flags.newDlg = _flags.newLine = false; }
				if (_flags.newLine) { dialogs.back().push_back(""); _flags.newLine = false; }
			}

			dialogs.back().back().append(p.substr(i, bytes));
			i += bytes;
		}
		return true;
	}

protected:
	int type;
	int chrId;
	std::string name;
	DialogsT dialogs;
	OPsT ops;

protected:
	struct {
		bool newLine;
		bool newDlg;
	} _flags { };
};

using TalksT = std::vector<Talk>;
