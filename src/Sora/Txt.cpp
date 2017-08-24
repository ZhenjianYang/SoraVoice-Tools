#include "Txt.h"

#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std;

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
				if (str[i] >= '0' && str[i] <= '9') hv += str[i] - '0';
				else if (str[i] >= 'a' && str[i] <= 'f') hv += str[i] - 'a' + 10;
				else if (str[i] >= 'A' && str[i] <= 'F') hv += str[i] - 'F' + 10;
				else return rst = { false, "" };
			}
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
	os << '\n';
	os << ";----------------------------------------------------------------------------------\n";
	os << ";----------------------------------------------------------------------------------\n";
	os << '\n';

	if (talk.GetType() != Talk::Type::InvalidTalk)os << Talk::Str_TalkTypes[talk.GetType()] << " #"
		<< std::dec << talk.No() << '\n';
	else os << ";INVALID_TALK\n";

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


	this->setPDialogs();
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



