#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <iomanip>
#include <memory>
#include <sstream>

#include <Sora/Txt.h>
#include <Sora/TalkOut.h>
#include <Sora/Encode.h>
#include <Utils/Files.h>
#include <mapping.h>

#include <conio.h>

#define ATTR_SNT "._SN.txt"
#define ATTR_PY ".py"
#define ATTR_MBIN ".mbin"
#define ATTR_TXT ".txt"

#define DFT_REPORT_NAME "report.txt"

static constexpr int MAXCH_ONELINE = 10000;

using namespace std;
using namespace Sora;

static inline void printUsage() {
	std::cout << "Usage:\n"
		"\n"
		"  SoraMergeTalk [-m] dir_txt1 dir_txt2 [dir_out] [report]\n"
		"\n"
		"    -m     : Enable voice id mapping.\n"
		"\n"
		"    Default:\n"
		"        dir_out = dir_txt1.out\n"
		"        report = " DFT_REPORT_NAME "\n"
		<< endl;
}

#define ERROR_EXIST(condition) if(condition) { printUsage(); return -1; }

static inline unique_ptr<TalksFile> getTalksFile(const std::string& fileName, char fmt) {
	switch (fmt)
	{
	case 't':return make_unique<Sora::Txt>(fileName);
	default: return nullptr;
	}
}

static auto GetVoiceIdMap() {
	unordered_map<string, string> map_vid;

	constexpr int buff_len = 7;
	static_assert(buff_len >= MAX_VOICEID_LEN_NEED_MAPPING + 1, "buff_len not enougt");
	char buff_vid[buff_len + 1];

	for (int vid_len = MAX_VOICEID_LEN_NEED_MAPPING; vid_len > 0; vid_len--) {
		for (int i = VoiceIdAdjustAdder[vid_len]; i < VoiceIdAdjustAdder[vid_len - 1] && i < NUM_MAPPING; i++) {
			if (VoiceIdMapping[i][0]) {
				sprintf(buff_vid, "%07d", i - VoiceIdAdjustAdder[vid_len]);
				string vid_ori = VoiceIdMapping[i];
				string vid_mapped = buff_vid + buff_len - vid_len;
				map_vid.insert({ vid_ori, vid_mapped });
			}
		}
	}

	return map_vid;
}

static auto GetVoiceId(const string& str) {
	vector<string> rst;
	size_t i = 0;
	while (i < str.length()) {
		while (i < str.length() && str[i] != '#') i++;
		if (i >= str.length()) break;
		i++;
		string tmp;
		while (str[i] >= '0' && str[i] <= '9') tmp.push_back(str[i++]);
		if (str[i] == 'V' || str[i] == 'v') rst.push_back(tmp);
	}
	return rst;
}

static inline Talk::Type GetType(const string& str) {
	static const vector<string> str_types{ Talk::Str_TalkTypes,  Talk::Str_TalkTypes + Talk::NumTalkTypes };
	for (auto tt : Talk::TypesList) {
		if (std::equal(str_types[tt].cbegin(), str_types[tt].cend(), str.c_str())) {
			return tt;
		}
	}
	return Talk::InvalidTalk;
}

static inline bool IsEmptyLine(const string& str) {
	return str.empty() || str[0] == ';' || str[0] == '\t';
}

static inline int GetChrID(const string& str) {
	if (str[0] != '0' || str[1] != 'x') return Talk::InvalidChrId;

	int chrId = 0;
	size_t idx = 2;
	while (idx < str.length()) {
		chrId <<= 4;
		if (str[idx] >= '0' && str[idx] <= '9') chrId += str[idx] - '0';
		else if (str[idx] >= 'a' && str[idx] <= 'f') chrId += str[idx] - 'a' + 10;
		else if (str[idx] >= 'A' && str[idx] <= 'F') chrId += str[idx] - 'A' + 10;
		else break;
		idx++;
	}
	if (idx == 2) return Talk::InvalidChrId;
	while (idx < str.length()) {
		if(str[idx] != ' ' && str[idx] != '\t') return Talk::InvalidChrId;
		idx++;
	}
	return chrId;
}

int main(int argc, char* argv[]) {
	unordered_set<char> switches;
	vector<string> params;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (int j = 1; argv[i][j]; j++) switches.insert(argv[i][j]);
		}
		else {
			params.push_back(argv[i]);
		}
	}

	bool enable_mapping = switches.find('m') != switches.end();

	string dir1 = params[0]; while (!dir1.empty() && (dir1.back() == '\\' || dir1.back() == '/')) dir1.pop_back();
	string dir2 = params[1];
	string dir_out = params.size() > 2 ? params[2] : dir1 + ".out";
	string prep = params.size() > 3 ? params[3] : DFT_REPORT_NAME;

	dir1.push_back('\\');
	dir2.push_back('\\');
	dir_out.push_back('\\');
	Utils::MakeDirectory(dir_out);

	ofstream ofs_rep(prep);

	using MapType = decltype(GetVoiceIdMap());
	MapType vid_map = enable_mapping ? GetVoiceIdMap() : MapType{};

	auto fn1s = Utils::SearchFiles(dir1 + "*" + ATTR_TXT);
	bool has_err = false;
	for (const auto &fn : fn1s) {
		const string name = fn.substr(0, fn.find('.'));

		std::cout << "Working with " << fn << "..." << flush;

		ifstream if1(dir1 + fn);
		ifstream if2(dir2 + fn);

		stringstream ss_new;
		stringstream ss_err;

		char buf1[MAXCH_ONELINE], buf2[MAXCH_ONELINE];

		Talk::Type type = Talk::InvalidTalk;
		bool chrId_got = false;
		bool name_got = false;

		enum class LineType {
			None,
			Empty,
			Type,
			ChrId,
			Name,
			Text
		};

		int cnt = 0;
		for (int line_no = 1; if1.getline(buf1, sizeof(buf1)); line_no++) {
			string s(line_no == 0 && buf1[0] == '\xEF' && buf1[1] == '\xBB' && buf1[2] == '\xBF' ? buf1 + 3 : buf1);
			string s2;
			if (if2.getline(buf2, sizeof(buf2))) {
				if (line_no == 0 && buf2[0] == '\xEF' && buf2[1] == '\xBB' && buf2[2] == '\xBF') s2 = buf2 + 3;
				else s2 = buf2;
			}

			LineType lt = LineType::None;
			if (IsEmptyLine(s)) {
				lt = LineType::Empty;
				if (std::equal(TOut::NOT_MATCHED_DIALOG.cbegin(), TOut::NOT_MATCHED_DIALOG.cend(), s.c_str())) {
					ss_err << "    [Wanning]TXT1, line " << line_no << ", NOT MATCHED DIALOG\n";
				}
			}

			if (lt == LineType::None) {
				auto tt = GetType(s);
				if (tt != Talk::InvalidTalk) {
					type = tt;

					chrId_got = false;
					name_got = type != Talk::NpcTalk;
					lt = LineType::Type;

					if (GetType(s2) == Talk::InvalidTalk && !IsEmptyLine(s2)) {
						ss_err << "    [Wanning]TXT1, line " << line_no << ", DIALOG START LINE NOT MATCHED\n";
					}
				}
			}
			if (lt == LineType::None && type == Talk::InvalidTalk) {
				ss_err.str("");
				ss_err << "    [Error]TXT1, line " << line_no << ", BAD START\n";
				ss_new.str("");
				break;
			}

			if (lt == LineType::None && !chrId_got) {
				int chrId = GetChrID(s);
				if (chrId != Talk::InvalidChrId) {
					lt = LineType::ChrId;

					if (!IsEmptyLine(s2) && GetChrID(s2) == Talk::InvalidChrId) {
						ss_err << "    [Wanning]TXT1, line " << line_no << ", CHRID LINE NOT MATCHED\n";
					}
				}
				chrId_got = true;
			}

			if (lt == LineType::None && !name_got) {
				if (GetChrID(s2) != Talk::InvalidChrId) {
					ss_err << "    [Wanning]TXT1, line " << line_no << ", NAME LINE NOT MATCHED\n";
				}

				lt = LineType::Name;
				name_got = true;
			}

			if (lt != LineType::None) {
				auto vids = GetVoiceId(s2);
				if (!vids.empty() && !std::equal(TOut::NOT_MATCHED_DIALOG.cbegin(), TOut::NOT_MATCHED_DIALOG.cend(), s.c_str())) {
					ss_err << "    [Wanning]TXT2, line " << line_no << ", VOICE ID NOT INPUT\n";
				}
			}
			else {
				if (GetChrID(s2) != Talk::InvalidChrId) {
					ss_err << "    [Wanning]TXT2, line " << line_no << ",  CHRID LINE NOT MATCHED\n";
				}
				if (GetType(s2) != Talk::InvalidTalk) {
					ss_err << "    [Wanning]TXT2, line " << line_no << ", DIALOG START LINE NOT MATCHED\n";
				}

				auto vids = GetVoiceId(s2);
				if (vids.size() > 1) {
					ss_err << "    [Wanning]TXT2, line " << line_no << ", MULTIPLE VOICE IDs\n";
				}

				if (!vids.empty()) {
					auto it = vid_map.find(vids[0]);
					if (it != vid_map.end()) {
						s = "#" + it->second + "v" + s;
					}
					else {
						s = "#" + vids[0] + "v" + s;
					}
					cnt++;
				}
			}

			ss_new << s << '\n';
		}

		if (ss_new.str().empty()) {
			ofs_rep << name << "Error Exist:\n" << ss_err.str() << '\n';
			has_err = true;
		}
		else {
			Txt txt(ss_new);
			if (!txt.ErrMsg().empty()) {
				ofs_rep << name << "Error Exist:\n" << txt.ErrMsg() << "\n\n";
				has_err = true;
			}
			else {
				ofstream ofs(dir_out + fn);
				ofs << ss_new.str();
				ofs.close();

				ofs_rep << name << ": Total Dialogs: " << txt.Talks().size() << ", Input Voice IDs: " << cnt << "\n";
				if (!ss_err.str().empty()) {
					ofs_rep << ss_err.str();
					has_err = true;
				}
				ofs_rep << '\n';
			}
		}

		std::cout << endl;
	}

	ofs_rep.close();
	if (has_err) {
		cout << "Wannings/Errors found, check " << prep << " for details" << endl;
		system("pause");
	}

	return 0;
}
