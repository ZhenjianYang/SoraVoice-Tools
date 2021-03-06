#include "MBin.h"

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

Sora::MBin::MBin(const std::string& filename) : TalksFile() {
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
Sora::MBin::MBin(std::istream& is, int size) : TalksFile() {
	std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
	is.read(buff.get(), size);

	this->err = createErrMsg(Create(buff.get(), size));
}
Sora::MBin::MBin(const char* buff, int size) : TalksFile() {
	this->err = createErrMsg(Create(buff, size));
}

int Sora::MBin::Create(const char* buff, int size) {
	talks.clear();
	pDialogs.clear();

	int ip = 0;

	int num = GET_INT(buff);
	if(num > MAX_TALKS_IN_MBIN) return ip;
	ip += 4;

	vector<pair<Talk::Type, int>> type_off_list;
	type_off_list.reserve(num + 1);
	for (int i = 0; i < num; i++) {
		int type = GET_INT(buff + ip);
		int roff = GET_INT(buff + ip + 4);
		if (type != 0) {
			Talk::Type tmp = Talk::Type::InvalidTalk;
			for(auto t : Talk::TypesList) {
				if(type == Id_Talks[t]) {
					tmp = t;
					break;
				}
			}
			if(tmp == Talk::Type::InvalidTalk) return ip;
			type_off_list.push_back({ tmp, roff + 4 + 8 * num });
		}
		ip += 8;
	}
	type_off_list.push_back({Talk::Type::InvalidTalk, size});

	talks.reserve(type_off_list.size() - 1);

	for(int i = 0; i < (int)type_off_list.size() - 1; i++) {
		int start = type_off_list[i].second;
		int end = type_off_list[i+1].second;
		auto type = type_off_list[i].first;

		if(end <= start) return start;

		ip = start + 1;
		int chrId = GET_U16(buff + ip); ip += 2;
		talks.push_back(Talk(i, type, chrId));
		auto& talk = talks.back();

		if(type == (int)Talk::Type::NpcTalk) {
			talk.Name().assign(buff + ip);
			ip += talk.Name().length() + 1;
		}
		if(ip >= end) return start;

		if (!talk.Add(string(buff + ip, end - ip - 1))) {
			return start;
		};
	}

	this->ResetPDialogs();

	return 0;
}

