#pragma once

#include <string>
#include <vector>

class CTalk {
public:
	enum class EType {
		AnonymousTalk,
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

	int LineEnd() const { return lineEnd; }
	void SetLineEnd(int lineEnd) { this->lineEnd = lineEnd; }
	int LineStart() const { return lineStart; }
	void SetLineStart(int lineStart) { this->lineStart = lineStart; }

	int DialogsNum() const { return dialogs.size(); }

public:
	CTalk(EType type, int lineStart = -1, int chrId = InvalidChrId)
		: type(type), chrId(chrId),
		  dialogs({{""}}),
		  lineStart(lineStart), lineEnd(lineStart){
	}

	bool Add(const std::string& content, int (*getChbytes)(const char*)) {
		const char* p = content.c_str();
		while(*p) {
			int bytes = getChbytes(p);
			if(bytes <= 0) return false;

			bool isSymbol = false;
			if(*p < 0x20 || *p == '#') {
				COP op { *p, 0 };

				switch(op.op) {
				case '#':
					while(p[bytes] >= '0' && p[bytes] <= '9') {
						op.oprnd *= 10;
						op.oprnd += p[bytes] - '0';
						bytes++;
					}
					op.op = p[bytes];
					if(!(op.op >= 'a' && op.op <= 'z') && !(op.op >= 'A' && op.op <= 'Z')) return false;
					break;
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

			AddChars(p, bytes, isSymbol);
			p += bytes;
		}
		return true;
	}

private:
	void AddChars(const char* p, int count, bool isSymbol) {
		if(!isSymbol) {
			if(_flags.newDlg) { dialogs.push_back({""}); _flags.newDlg = _flags.newLine = false; }
			if(_flags.newLine) { dialogs.back().push_back(""); _flags.newLine = false; }
		}
		dialogs.back().back().append(p, count);
	}

private:
	EType type;
	int chrId;
	std::string name;
	CDialogs dialogs;
	COPs ops;

	int lineStart;
	int lineEnd;

private:
	struct {
		bool newLine;
		bool newDlg;
	} _flags { };
};

