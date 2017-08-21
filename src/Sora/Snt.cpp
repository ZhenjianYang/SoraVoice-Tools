#include "Snt.h"

#include <Utils/Encode.h>

#include <string>
#include <fstream>
#include <sstream>

using namespace std;
using TalkType = CTalk::EType;

static constexpr int MAXCH_ONELINE = 10000;

static const struct {
	string str;
	TalkType type;
} TalkTypes[] = {
	{"text", TalkType::AnonymousTalk},//text
	{"say",  TalkType::ChrTalk}, //id, text
	{"talk",  TalkType::NpcTalk} //id, name, text
};
static constexpr int NumTalks = std::extent_v<decltype(TalkTypes)>;

static auto getHexes(const std::string& str) {
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

static auto fixString(const std::string& str) {
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
			rst.second.push_back(str[i++]);
		}
	}
	
	return rst;
}

int CSnt::Create(std::istream & is)
{
	char buff[MAXCH_ONELINE + 1];
	constexpr int InvalidTalkId = -1;

	bool name_finished = false;
	bool text_beg = false;
	int talks_id = InvalidTalkId;
	for (int line_no = 1; is.getline(buff, sizeof(buff)); line_no++) {

		string s(line_no == 0 && buff[0] == '\xEF' && buff[1] == '\xBB' && buff[2] == '\xBF' ? buff + 3 : buff);
		size_t is = 0;

		if (talks_id == InvalidTalkId && (s[0] == ';' || s.find(".def") == 0)) {
			lines.push_back(s);
			continue;
		}

		while (is < s.length()) {
			if (talks_id == InvalidTalkId) {
				auto idx = string::npos;
				for (size_t tid = 0; tid < NumTalks; tid++) {
					if ((idx = s.find(TalkTypes[tid].str, is)) != string::npos) {
						talks_id = tid;
						name_finished = talks_id != (int)TalkType::NpcTalk;
						text_beg = false;
						if (idx > 0) {
							lines.push_back(s.substr(is, idx - is));
						}
						talks.push_back(CTalk(TalkTypes[tid].type, idx == 0 ? line_no : line_no + 1));
						is = idx + TalkTypes[tid].str.length();
						break;
					}
				}
			}
			if (is >= s.length()) continue;
			if (talks_id == InvalidTalkId) {
				lines.push_back(s.substr(is));
				break;
			}

			if (talks_id != (int)TalkType::AnonymousTalk && talks.back().ChrId() == CTalk::InvalidChrId) {
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

			if (talks_id == (int)TalkType::NpcTalk && !name_finished) {
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
			auto fixed = fixString(s.substr(is, idx - is));
			if (!fixed.first) return line_no;

			talks.back().Add(fixed.second, Encode::GetChCount_GBK);
			if (idx != string::npos) {
				talks_id = InvalidTalkId;
				talks.back().SetPosEnd(idx == s.length() ? line_no - 1 : line_no);
				is = idx + 1;
			}
			else {
				break;
			}
		}//while (i < s.length())
	}//for line_no

	return 0;
}

int CSnt::Create(const char * filename) {
	std::ifstream ifs(filename);
	if (!ifs) return false;
	return Create(ifs);
}

