#include "Snt.h"

#include "Encode.h"

#include <string>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std;
using namespace Sora;

static constexpr int MAXCH_ONELINE = 10000;
static const auto& GetChCountFun = Encode::GetChCoutFun(Encode::Encode::GBK);

inline static std::string createErrMsg(int ret) {
	if (ret == 0) return "";

	stringstream ss;
	ss << "Error with mbin file, Offset: 0x" << hex << ret;
	return ss.str();
}

inline static auto getHexes(const std::string& str) {
	vector<int> rst;
	int val = 0;
	int cnt = 0;
	for (auto c : str) {
		if (c == ' ' || c == '\t') {
			if (cnt) {
				rst.push_back(val);
				val = cnt = 0;
			}
			continue;
		};

		int tval = 0;
		if (c >= '0' && c <= '9') tval = c - '0';
		else if(c >= 'a' && c <= 'f') tval = c - 'a' + 10;
		else if(c >= 'A' && c <= 'F') tval = c - 'A' + 10;
		else return vector<int>();

		val <<= 4;
		val |= tval;
		cnt++;

		if (cnt == 2) {
			rst.push_back(val);
			val = cnt = 0;
		}
	}
	if (cnt) {
		rst.push_back(val);
	}
	return rst;
}

std::pair<bool, std::string> Sora::Snt::SntStr2TalkStr(const std::string& str) {
	std::pair<bool, string> rst{ true, ""};

	size_t i = 0;
	while (i < str.length()) {
		if (str[i] == '\\') {
			if (str[i + 1] >= '0' && str[i + 1] <= '9') {
				rst.second.push_back(str[i + 1] - '0');
				i += 2;
			}
			else return rst = { false, "" };
		}
		else if (str[i] == '\'') {
			if (str[i + 1] != '[') return rst = { false, "" };

			auto right = str.find("]'", i + 2);
			if(right == string::npos) return rst = { false, "" };

			auto hexes = getHexes(str.substr(i + 2, right - i - 2));
			if(hexes.empty()) return rst = { false, "" };

			for (auto h : hexes) {
				rst.second.push_back(h);
			}
			i = right + 2;
		}
		else {
			int len = GetChCountFun(str.c_str() + i);
			while (len > 0) {
				rst.second.push_back(str[i++]);
				len--;
			}
		}
	}
	
	return rst;
}

std::string Sora::Snt::TalkStr2SntStr(const std::string& str) {
	string rst;
	char buff[12];
	size_t i = 0;
	while (i < str.length()) {
		if(str[i] == OP::SCPSTR_CODE_ITEM) {
			std::sprintf(buff, "'[%02X%02X%02X]'", str[i], str[i+1], str[i+2]);
			rst.append(buff);
			i += 3;
		} else if (str[i] == OP::SCPSTR_CODE_COLOR) {
			std::sprintf(buff, "'[%02X%02X]'", str[i], str[i+1]);
			rst.append(buff);
			i += 2;
		} else if (str[i] > 5 && str[i] < 0x20) {
			std::sprintf(buff, "'[%02X]'", str[i]);
			rst.append(buff);
			i += 1;
		} else if (str[i] >= 0 && str[i] <= 5) {
			rst.push_back('\\');
			rst.push_back(str[i] + '0');
			i += 1;
		} else {
			int cnt = GetChCountFun(str.c_str() + i);
			while(cnt > 0) {
				rst.push_back(str[i++]);
				cnt--;
			}
		}
	}
	return rst;
}

void Sora::Snt::OutputTalk(std::ostream& os, const Talk& talk) {
	char buff[12];
	os << Snt::Str_Talks[talk.GetType()] << '\n';
	if (talk.GetType() != Talk::AnonymousTalk) {
		std::sprintf(buff, "[%02X %02X]", talk.ChrId() & 0xFF, (talk.ChrId() >> 8) & 0xFF);
		os << buff << '\n';
	}
	if (talk.GetType() == Talk::NpcTalk) {
		os << "\t\t\t\t'" << talk.Name() << "\"\n";
	}
	os << "'";

	for (const auto& dlg : talk.Dialogs()) {
		os << '\n';
		for (const auto& line : dlg.Lines()) {
			os << "\t\t\t\t" << Sora::Snt::TalkStr2SntStr(line) << '\n';
		}
	}
	os << '"' << '\n';
}


Sora::Snt::Snt(const std::string& filename) {
	std::ifstream ifs(filename);
	if (!ifs) {
		this->err = "Open file failed.";
		return;
	}
	this->err = createErrMsg(Create(ifs));
}

Sora::Snt::Snt(std::istream& is) {
	this->err = createErrMsg(Create(is));
}

int Sora::Snt::Create(std::istream & is)
{
	lines.clear();
	talks.clear();
	pDialogs.clear();

	char buff[MAXCH_ONELINE + 1];
	constexpr auto InvalidTalkId = Talk::Type::InvalidTalk;

	bool name_finished = false;
	bool text_beg = false;
	auto talks_id = InvalidTalkId;
	int no = 0;
	for (int line_no = 1; is.getline(buff, sizeof(buff)); line_no++) {

		string s(line_no == 0 && buff[0] == '\xEF' && buff[1] == '\xBB' && buff[2] == '\xBF' ? buff + 3 : buff);
		size_t is = 0;

		if (talks_id == InvalidTalkId && (s[0] == ';' || s.find(".def") == 0)) {
			lines.push_back({line_no, s});
			continue;
		}

		while (is < s.length()) {
			if (talks_id == InvalidTalkId) {
				auto idx = string::npos;
				for (auto tid : Talk::TypesList) {
					if ((idx = s.find(Str_Talks[tid], is)) != string::npos) {
						talks_id = tid;
						name_finished = talks_id != Talk::NpcTalk;
						text_beg = false;
						if (idx > 0) {
							lines.push_back({line_no, s.substr(is, idx - is)});
						}
						lines.push_back({-(int)talks.size(), string()});
						talks.push_back(Talk(no++, tid));
						is = idx + std::strlen(Str_Talks[tid]);
						break;
					}
				}
				if (talks_id == InvalidTalkId) {
					lines.push_back({line_no, s.substr(is)});
					break;
				}
			}
			if (is >= s.length()) continue;

			if (talks_id != (int)Talk::AnonymousTalk && talks.back().ChrId() == Talk::InvalidChrId) {
				while (s[is] == ' ' || s[is] == '\t') ++is;
				if (is >= s.length()) continue;

				if (s[is] != '[') return line_no;

				auto right = s.find(']', is + 1);
				if (right == string::npos) return line_no;

				auto nums = getHexes(s.substr(is + 1, right - is - 1));
				if (nums.size() != 2) return line_no;

				talks.back().SetChrId(nums[1] << 8 | nums[0]);
				is = right + 1;
			}

			if (talks_id == (int)Talk::NpcTalk && !name_finished) {
				while (s[is] == ' ' || s[is] == '\t') ++is;
				if (is >= s.length()) continue;

				if (s[is] != '\'') return line_no;

				auto right = s.find('"', is + 1);
				if (right == string::npos) return line_no;

				talks.back().Name().assign(s.substr(is + 1, right - is - 1));
				is = right + 1;
				name_finished = true;
			}

			if (!text_beg) {
				while (s[is] == ' ' || s[is] == '\t') ++is;
				if (is >= s.length()) continue;

				if (s[is] != '\'') return line_no;
				text_beg = true;
				is++;
			}

			while (s[is] == '\t') ++is;

			auto idx = s.find('"', is);
			auto fixed = SntStr2TalkStr(s.substr(is, idx - is));
			if (!fixed.first) return line_no;

			if (!talks.back().Add(fixed.second, GetChCountFun)) {
				return line_no;
			};
			if (idx != string::npos) {
				talks_id = InvalidTalkId;
				is = idx + 1;
			}
			else {
				break;
			}
		}//while (i < s.length())
	}//for line_no

	this->setPDialogs();

	return 0;
}

bool Sora::Snt::WriteTo(std::ostream& os) const {
	for(const auto& line : lines) {
		if(line.lineNo > 0) os << line.content << '\n';
		else OutputTalk(os, talks[-line.lineNo]);
	}
	os << std::flush;

	return true;
}

bool Sora::Snt::WriteTo(const std::string& filename) const {
	std::ofstream ofs(filename);
	if (!ofs) return false;
	return WriteTo(ofs);
}

