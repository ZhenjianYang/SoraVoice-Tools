#include "Txt.h"

#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

using namespace std;

static constexpr int MAXCH_ONELINE = 10000;

inline static std::string createErrMsg(int ret) {
	if(ret == 0) return "";

	stringstream ss;
	ss << "Error with txt file, line: "  << ret;
	return ss.str();
}

std::pair<bool, std::string> Sora::Txt::TxtStr2TalkStr(const std::string& str) {
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

std::string Sora::Txt::TalkStr2TxtStr(const std::string& str) {
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

void Sora::Txt::OutputTalk(std::ostream& os, const Talk& talk) {
	if (talk.No() < 0 || talk.GetType() == Talk::InvalidTalk) return;

	os << '\n';
	os << ";----------------------------------------------------------------------------------\n";
	os << ";----------------------------------------------------------------------------------\n";
	os << '\n';

	bool op5 = talk.HasOp('\x5');
	bool opA = talk.HasOp('A');

	if (op5 || opA) {
		os << ";";
		if (opA) os << " op#A";
		if (op5) os << " op#5";
		os << '\n';
	}

	os << Talk::Str_TalkTypes[talk.GetType()] << " #"
		<< std::dec << talk.No() << '\n';

	if (talk.ChrId() != Talk::InvalidChrId) os << "0x" << std::hex << talk.ChrId() << '\n';
	if (talk.GetType() == Talk::NpcTalk) os << talk.Name() << '\n';

	for (const auto& dlg : talk.Dialogs()) {
		os << '\n';
		for (const auto& line : dlg.Lines()) {
			os << TalkStr2TxtStr(line) << '\n';
		}
	}
}


Sora::Txt::Txt(const std::string& filename) : TalksFile() {
	std::ifstream ifs(filename);
	if (!ifs) {
		this->err = "Open file failed.";
		return;
	}

	this->err = createErrMsg(Create(ifs));
}
Sora::Txt::Txt(std::istream& is) : TalksFile() {
	this->err = createErrMsg(Create(is));
}

int Sora::Txt::Create(std::istream& is) {
	talks.clear();
	pDialogs.clear();
	char buff[MAXCH_ONELINE + 1];

	auto talks_type = Talk::Type::InvalidTalk;
	bool chrId_got = false;
	bool name_got = false;
	bool new_talk = false;

	const vector<string> str_types {Talk::Str_TalkTypes,  Talk::Str_TalkTypes + Talk::NumTalkTypes};

	for (int line_no = 1; is.getline(buff, sizeof(buff)); line_no++) {
		string s(line_no == 0 && buff[0] == '\xEF' && buff[1] == '\xBB' && buff[2] == '\xBF' ? buff + 3 : buff);

		if(s.empty()) continue;
		if(s[0] == '\t' || s[0] == ';') continue;

		for (auto tt : Talk::TypesList) {
			if(std::equal(str_types[tt].cbegin(), str_types[tt].cend(), s.c_str())) {
				talks_type = tt;

				auto idx = str_types[tt].length();
				while(s[idx] == ' ' || s[idx] == '\t') idx++;

				if(s[idx] != '#') return line_no;

				idx++;
				int no = 0;
				while(s.back() == ' ' || s.back() == '\t') s.pop_back();
				if(idx >= s.length()) return line_no;
				while(idx < s.length()) {
					no *= 10;
					if(s[idx] >= '0' && s[idx] <= '9') no += s[idx] - '0';
					else return line_no;
					idx++;
				}
				if (no >= TalksFile::MaxTalksNum) {
					return line_no;
				}

				talks.push_back(Talk(no, talks_type));
				name_got = talks_type != Talk::Type::NpcTalk;
				chrId_got = false;
				new_talk = true;

				break;
			}
		}
		if(talks.empty()) {
			return line_no;
		}
		if(new_talk) {
			new_talk = false;
			continue;
		}

		if(!chrId_got && s[0] == '0' && s[1] == 'x') {
			size_t idx = 2;
			int chrId = 0;
			while(s.back() == ' ' || s.back() == '\t') s.pop_back();
			if(idx >= s.length()) return line_no;
			while(idx < s.length()) {
				chrId <<= 4;
				if(s[idx] >= '0' && s[idx] <= '9') chrId += s[idx] - '0';
				else if (s[idx] >= 'a' && s[idx] <= 'f') chrId += s[idx] - 'a' + 10;
				else if (s[idx] >= 'A' && s[idx] <= 'F') chrId += s[idx] - 'A' + 10;
				else return line_no;
				idx++;
			}
			talks.back().SetChrId(chrId);
			continue;
		}
		chrId_got = true;

		if(!name_got) {
			talks.back().Name() = s;
			name_got = true;
			continue;
		}

		auto tlkStr = TxtStr2TalkStr(s);
		if(!tlkStr.first) return line_no;

		talks.back().Add(tlkStr.second);
	}

	this->ResetPDialogs();
	return 0;
}

bool Sora::Txt::WriteTo(std::ostream& os) const {
	for (const auto& talk : talks) {
		OutputTalk(os, talk);
	}
	os << std::flush;

	return true;
}

bool Sora::Txt::WriteTo(const std::string& filename) const {
	std::ofstream ofs(filename);
	if (!ofs) return false;
	return WriteTo(ofs);
}



