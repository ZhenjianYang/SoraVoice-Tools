#include "MBin.h"

#include <Utils/Encode.h>

#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std;

constexpr int MAX_TALKS_IN_MBIN = 5000;
#define GET_INT(ptr) *(const int*)(ptr)
#define GET_U16(ptr) *(const unsigned short*)(ptr)

inline static std::string createErrMsg(int ret) {
	if(ret == 0) return "";

	stringstream ss;
	ss << "Error with mbin file, Offset: 0x" << hex << ret;
	return ss.str();
}

MBin::MBin(const std::string& filename, Encode encode /*= Encode::SJIS*/) : TalksFile(), encode(encode) {
	std::ifstream ifs(filename, ios::binary);
	if (!ifs) {
		this->err = "Open file failed.";
		return;
	}

	ifs.seekg(0, ios::end);
	int size = (int)ifs.tellg();
	ifs.seekg(0, ios::beg);

	std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
	ifs.read(buff.get(), size);

	this->err = createErrMsg(Create(buff.get(), size));
}
MBin::MBin(std::istream& is, int size, Encode encode /*= Encode::SJIS*/) : TalksFile(), encode(encode) {
	std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
	is.read(buff.get(), size);

	this->err = createErrMsg(Create(buff.get(), size));
}
MBin::MBin(const char* buff, int size, Encode encode /*= Encode::SJIS*/) : TalksFile(), encode(encode) {
	this->err = createErrMsg(Create(buff, size));
}

int MBin::Create(const char* buff, int size) {
	std::function<int(const char*)> getChbytes = this->encode == Encode::SJIS ?
			::Encode::GetChCount_SJis: ::Encode::GetChCount_Utf8;

	talks.clear();
	pDialogs.clear();

	int ip = 0;

	int num = GET_INT(buff);
	if(num > MAX_TALKS_IN_MBIN) return ip;
	ip += 4;

	vector<pair<int, int>> type_off_list;
	type_off_list.reserve(num + 1);
	for (int i = 0; i < num; i++) {
		int type = GET_INT(buff + ip);
		int roff = GET_INT(buff + ip + 4);
		if (type != 0) {
			int tmp = -1;
			for(int j = 0; j < Talk::NumTalkTypes; j++) {
				if(Id_Talks[j] == type) {
					tmp = j; break;
				}
			}
			if(tmp < 0) return ip;
			type_off_list.push_back({ tmp, roff + 4 + 8 * num });
		}
		ip += 8;
	}
	type_off_list.push_back({0, size});

	talks.reserve(type_off_list.size() - 1);

	for(int i = 0; i < (int)type_off_list.size() - 1; i++) {
		int start = type_off_list[i].second;
		int end = type_off_list[i+1].second;
		int type = type_off_list[i].first;

		if(end <= start) return start;

		ip = start + 1;
		int chrId = GET_U16(buff + ip); ip += 2;
		talks.push_back(Talk(i, type, chrId));
		auto& talk = talks.back();

		if(type == Talk::NpcTalk) {
			talk.Name().assign(buff + ip);
			ip += talk.Name().length() + 1;
		}
		if(ip >= end) return start;

		if (!talk.Add(string(buff + ip, end - ip - 1), getChbytes)) {
			return start;
		};
	}

	for(auto& talk : this->talks) {
		for(auto& dlg : talk.Dialogs()) {
			pDialogs.push_back(&dlg);
		}
	}

	return 0;
}

