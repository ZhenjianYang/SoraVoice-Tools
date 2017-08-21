#pragma once

#include <string>
#include <vector>
#include <functional>

class CTalk {
public:
	enum class EType {
		AnonymousTalk = 0,
		ChrTalk,
		NpcTalk
	};

	struct COP {
		char op;
		int oprnd;
	};
	using CLine = std::string;
	using CDialog = std::vector<CLine>;
	using CDialogs = std::vector<CDialog>;
	using COPs = std::vector<COP>;

	static constexpr int InvalidChrId = 0x80000000;
	static constexpr char SCPSTR_CODE_ITEM = '\x1F';
	static constexpr char SCPSTR_CODE_COLOR = '\x07';
	static constexpr char SCPSTR_CODE_LINE_FEED = '\x01';
	static constexpr char SCPSTR_CODE_ENTER = '\x02';
	static constexpr char SCPSTR_CODE_CLEAR = '\x03';

	EType Type() const { return type; }
	int ChrId() const { return chrId; }
	void SetChrId(int chrId) { this->chrId = chrId; }

	std::string& Name() { return name; }
	const std::string& Name() const { return name; }
	CDialogs& Dialogs() { return dialogs; }
	const CDialogs& Dialogs() const { return dialogs; }
	COPs& OPs() { return ops; }
	const COPs& OPs() const { return ops; }

	int PosEnd() const { return end; }
	void SetPosEnd(int end) { this->end = end; }
	int PosStart() const { return start; }
	void SetPosStart(int start) { this->start = start; }

	int DialogsNum() const { return dialogs.size(); }
	CDialog& operator[] (int index) { return dialogs[index]; }
	const CDialog& operator[] (int index) const { return dialogs[index]; }

public:
	CTalk(EType type, int start = -1, int chrId = InvalidChrId)
		: type(type), chrId(chrId),
		  dialogs({{""}}),
		  start(start), end(start){
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
					COP op{ 0, 0 };
					while (p[bytes] >= '0' && p[bytes] <= '9') {
						op.oprnd *= 10;
						op.oprnd += p[bytes] - '0';
						bytes++;
					}
					op.op = p[bytes++];
					if (!(op.op >= 'a' && op.op <= 'z') && !(op.op >= 'A' && op.op <= 'Z')) return false;
					ops.push_back(op);
				}
			}
			else if(0 <= p[i] && p[i] < 0x20) {
				COP op { p[i], 0 };

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

private:
	void AddChars(const char* p, int count, bool isSymbol) {
		
	}

private:
	EType type;
	int chrId;
	std::string name;
	CDialogs dialogs;
	COPs ops;

	int start;
	int end;

private:
	struct {
		bool newLine;
		bool newDlg;
	} _flags { };
};

