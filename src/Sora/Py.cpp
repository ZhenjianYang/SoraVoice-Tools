#include "Py.h"

#include "Encode.h"

#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unordered_map>

using namespace std;
using namespace Sora;

#define SPACE "    "
#define DQOUT "\""

static constexpr int MAXCH_ONELINE = 10000;
static const auto& GetChCountFun = Encode::GetChCoutFun(Encode::Encode::GBK);

inline static std::string createErrMsg(int ret) {
	if (ret == 0) return "";

	stringstream ss;
	ss << "Error with py file, line: " << ret;
	return ss.str();
}

std::pair<bool, std::string> Sora::Py::PyStr2TalkStr(const std::string& str) {
	std::pair<bool, string> rst{ true, "" };

	size_t i = 0;
	while (i < str.length()) {
		if (str[i] == '\\' && str[i + 1] == 'x') {
			i += 2;
			int hv = 0;
			for (int j = 0; j < 2; j++, i++) {
				hv <<= 4;
				if (str[i] >= '0' && str[i] <= '9') hv += str[i] - '0';
				else if (str[i] >= 'a' && str[i] <= 'f') hv += str[i] - 'a' + 10;
				else if (str[i] >= 'A' && str[i] <= 'F') hv += str[i] - 'A' + 10;
				else return rst = { false, "" };
			}
			rst.second.push_back(hv);
		}
		else {
			rst.second.push_back(str[i++]);
		}
	}
	return rst;
}

std::string Sora::Py::TalkStr2PyStr(const std::string& str) {
	std::string rst;
	char buff[16];
	size_t i = 0;
	using byte = unsigned char;
	while (i < str.length()) {
		if (str[i] == OP::SCPSTR_CODE_ITEM) {
			std::sprintf(buff, "\\x%02X\\x%02X\\x%02X", (byte)str[i], (byte)str[i + 1], (byte)str[i + 2]);
			rst.append(buff);
			i += 3;
		}
		else if (str[i] == OP::SCPSTR_CODE_COLOR) {
			std::sprintf(buff, "\\x%02X\\x%02X", (byte)str[i], (byte)str[i + 1]);
			rst.append(buff);
			i += 2;
		}
		else if (str[i] >= 0 && str[i] < 0x20) {
			std::sprintf(buff, "\\x%02X", (byte)str[i]);
			rst.append(buff);
			i += 1;
		}
		else {
			rst.push_back(str[i]);
			i += 1;
		}
	}
	return rst;
}

void Sora::Py::OutputTalk(std::ostream& os, const Talk& talk, bool with_cmt) {
	os << SPACE << Talk::Str_TalkTypes[talk.GetType()] << "(\n";
	if (talk.GetType() != Talk::AnonymousTalk) {
		os << SPACE SPACE << "0x" << hex << uppercase << talk.ChrId() << ",\n";
	}
	if (talk.GetType() == Talk::NpcTalk) {
		os << SPACE SPACE DQOUT <<  talk.Name() << DQOUT ",\n";
	}

	bool multiLines = talk.Dialogs().size() > 1 || talk.Dialogs()[0].LinesNum() > 1;
	if(multiLines) os << SPACE SPACE "(\n";
	for (const auto& dlg : talk.Dialogs()) {
		for (const auto& line : dlg.Lines()) {
			os << (multiLines ? SPACE SPACE SPACE DQOUT : SPACE SPACE DQOUT)
				<< TalkStr2PyStr(line.text) << DQOUT ","
				<< (with_cmt && !line.cmt.empty() ? SPACE SPACE SPACE "##" + line.cmt : "")
				<< '\n';
		}
	}
	if (multiLines) os << SPACE SPACE ")\n";
	os << SPACE ")\n";
}


Sora::Py::Py(const std::string& filename) {
	std::ifstream ifs(filename);
	if (!ifs) {
		this->err = "Open file failed.";
		return;
	}
	this->err = createErrMsg(Create(ifs));
}

Sora::Py::Py(std::istream& is) {
	this->err = createErrMsg(Create(is));
}

int Sora::Py::Create(std::istream & is)
{
	lines.clear();
	talks.clear();
	pDialogs.clear();

	char buff[MAXCH_ONELINE + 1];
	int no = 0;

	constexpr int len_space = sizeof(SPACE) - 1;
	const string fun_end = SPACE ")";
	const string text_ebeg = SPACE SPACE "(";
	const string text_end = SPACE SPACE ")";
	const string str_scpstr = "scpstr";
	const vector<string> str_types{ Talk::Str_TalkTypes,  Talk::Str_TalkTypes + Talk::NumTalkTypes };
	const unordered_map<string, char> scp_map{
		{ "SCPSTR_CODE_ITEM", 0x1F },
		{ "SCPSTR_CODE_LINE_FEED", 0x01 },
		{ "SCPSTR_CODE_ENTER", 0x02 },
		{ "SCPSTR_CODE_CLEAR", 0x03 },
		{ "SCPSTR_CODE_05", 0x05 },
		{ "SCPSTR_CODE_COLOR", 0x07 },
		{ "SCPSTR_CODE_09", 0x09 },
	};

	int line_no = 0;
	while (is.getline(buff, sizeof(buff))) {
		line_no++;
		string s(line_no == 1 && buff[0] == '\xEF' && buff[1] == '\xBB' && buff[2] == '\xBF' ? buff + 3 : buff);

		auto type = Talk::InvalidTalk;
		for (auto tid : Talk::TypesList) {
			if (std::equal(str_types[tid].cbegin(), str_types[tid].cend(), s.c_str() + len_space)) {
				type = tid;
				lines.push_back({ -(int)talks.size(), "" });
				talks.push_back(Talk(no++, type));

				bool chrId_got = false;
				bool name_got = type != Talk::NpcTalk;

				while(true) {
					line_no++;
					if (!is.getline(buff, sizeof(buff))) return line_no;
					s = buff;

					if (std::equal(fun_end.cbegin(), fun_end.cend(), s.c_str())) break;

					size_t idx = 0;
					while (s[idx] == ' ') idx++;
					if (idx == s.length()) continue;

					if (!chrId_got) {
						chrId_got = true;
						if (s[idx] == '0' && s[idx + 1] == 'x') {
							int chrId = 0;
							if(!sscanf(s.c_str() + idx, "%x", &chrId)) return line_no;
							talks.back().SetChrId(chrId);
							continue;
						}
					}

					if (!name_got) {
						if (s[idx] != '"') return line_no;
						auto r = s.find('"', idx + 1);
						talks.back().Name() = s.substr(idx + 1, r - idx - 1);
						name_got = true;
						continue;
					}

					if (s[idx] == '(' || s[idx] == ')') continue;

					if (s[idx] == '"') {
						auto r = s.find('"', idx + 1);
						auto rs = PyStr2TalkStr(s.substr(idx + 1, r - idx - 1));
						if (!rs.first) return line_no;
						if(!talks.back().Add(rs.second)) return line_no;
					}
					else if (std::equal(str_scpstr.cbegin(), str_scpstr.cend(), s.c_str() + idx)){
						int code;

						string scp;
						idx += str_scpstr.length();
						if (s[idx] != '(') return line_no;
						idx++;
						if (s[idx] == '0') {
							if (!sscanf(s.c_str() + idx, "%X", &code)) return line_no;
							scp.push_back(char(code));
						}
						else {
							auto r = s.find_first_of(",)", idx);
							string scps = s.substr(idx, r - idx);
							
							auto it = scp_map.find(scps);
							if (it == scp_map.end()) return line_no;
							if (it->first == "SCPSTR_CODE_ITEM") {
								if (!sscanf(s.c_str() + r + 1, "%X", &code)) return line_no;
								scp.push_back(it->second);
								scp.push_back((char)(code & 0xFF));
								scp.push_back((char)((code >> 8) & 0xFF));
							}
							else if (it->first == "SCPSTR_CODE_COLOR") {
								if (!sscanf(s.c_str() + r + 1, "%X", &code)) return line_no;
								scp.push_back(it->second);
								scp.push_back((char)(code & 0xFF));
							}
							else {
								scp.push_back(it->second);
							}
						}
						if(!talks.back().Add(scp)) return line_no;
					}
				} //while (true)

				if (!name_got || !chrId_got) return line_no;
				break;
			}
		} //for (auto tid : Talk::TypesList)

		if (type == Talk::InvalidTalk) {
			lines.push_back({ line_no, s });
		}
	}//for line_no

	this->ResetPDialogs();

	return 0;
}

bool Sora::Py::WriteTo(std::ostream& os, bool with_cmt) const {
	bool out_no = true;
	bool opA = false;
	bool op5 = false;
	for(const auto& line : lines) {
		if (line.lineNo > 0) {
			if (opA || op5) {
				if (line.content.length() < 5 || line.content[4] != '#') {
					os << SPACE "#"
						<< (opA ? " op#A" : "")
						<< (op5 ? " op#5" : "") << '\n';
				}
				opA = op5 = false;
			}

			os << line.content << '\n';
			out_no = line.content.length() < 5 || line.content[4] != '#';
		}
		else {
			if (talks[-line.lineNo].No() < 0 || talks[-line.lineNo].GetType() == Talk::InvalidTalk) continue;

			if (out_no) {
				os << SPACE "#" << talks[-line.lineNo].No() << '\n';
			}
			OutputTalk(os, talks[-line.lineNo], with_cmt);
			opA = talks[-line.lineNo].HasOp('A');
			op5 = talks[-line.lineNo].HasOp('\x5');
		}
	}
	os << std::flush;

	return true;
}

bool Sora::Py::WriteTo(const std::string& filename, bool with_cmt) const {
	std::ofstream ofs(filename);
	if (!ofs) return false;
	return WriteTo(ofs, with_cmt);
}

