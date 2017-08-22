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

int MBin::Create(const char* buff, int size, std::function<int(const char*)>getChbytes) {
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
		talks.push_back(Talk(type, chrId));
		auto& talk = talks.back();

		if(type == Talk::NpcTalk) {
			talk.Name().assign(buff + ip);
			ip += talk.Name().length() + 1;
		}
		if(ip >= end) return start;

		talk.Add(string(buff + ip, end - ip - 1), getChbytes);
	}

	return 0;
}

int MBin::Create(std::istream & is, int size, std::function<int(const char*)>getChbytes)
{
	std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
	is.read(buff.get(), size);
	return Create(buff.get(), size, getChbytes);
}

int MBin::Create(const std::string& filename, std::function<int(const char*)>getChbytes) {
	std::ifstream ifs(filename, ios::binary);
	if (!ifs) return false;

	ifs.seekg(0, ios::end);
	int size = (int)ifs.tellg();
	ifs.seekg(0, ios::beg);

	return Create(ifs, size, getChbytes);
}

