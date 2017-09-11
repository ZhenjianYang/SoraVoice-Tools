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
#include <Utils/Decoder_Ogg.h>
#include <mapping.h>

#include <Windows.h>

#include <conio.h>

#define ATTR_SNT "._SN.txt"
#define ATTR_PY ".py"
#define ATTR_MBIN ".mbin"
#define ATTR_TXT ".txt"

#define DFT_REPORT_NAME "report.txt"
#define VPATH "vpath.txt"

static constexpr int MAXCH_ONELINE = 10000;

using namespace std;
using namespace Sora;

static inline void printUsage() {
	std::cout << "Usage:\n"
		"\n"
		"  SoraInputVoiceId -{Switches} dir_txt1 dir_txt2 [dir_out] [report]\n"
		"\n"
		"    Switches:"
		"        m : Enable voice id mapping\n"
		"        l : Add voice length for op#A and op#5\n"
//		"        L : Add voice length for all voices\n"
		"            NOTE: Should add paths of voice folders to '" VPATH "'\n"
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
		for (int i = VoiceIdAdjustAdder[vid_len]; i < VoiceIdAdjustAdder[vid_len - 1] && i < (int)NUM_MAPPING; i++) {
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
	pair<bool, vector<string>> rst{false, {}};
	size_t i = 0;
	bool text = false;
	while (i < str.length()) {
		while (i < str.length() && str[i] != '#') {
			if(i + 4 < str.length() && str[i] == '[' && str[i + 1] == 'x' && str[i + 4] == ']') {
				i += 5;
			} else {
				if(str[i] != '\t') {
					text = true;
				}
				i++;
			}
		}
		if (i >= str.length()) break;
		i++;
		if (str[i] == '#') break;
		string tmp;
		while (str[i] >= '0' && str[i] <= '9') tmp.push_back(str[i++]);
		if (str[i] == 'V' || str[i] == 'v') {
			rst.second.push_back(tmp);
			if(text) {
				rst.first = true;
			}
		}
		if((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z')) i++;
	}
	return rst;
}

static auto GetOp(const string& str, char op) {
	pair<bool, vector<string>> rst{ false,{} };
	size_t i = 0;
	bool text = false;
	while (i < str.length()) {
		while (i < str.length() && str[i] != '#') {
			if (i + 4 < str.length() && str[i] == '[' && str[i + 1] == 'x' && str[i + 4] == ']') {
				i += 5;
			}
			else {
				if (str[i] != '\t') {
					text = true;
				}
				i++;
			}
		}
		if (i >= str.length()) break;
		i++;
		if (str[i] == '#') break;
		string tmp;
		while (str[i] >= '0' && str[i] <= '9') tmp.push_back(str[i++]);
		if (str[i] == op) {
			rst.second.push_back(tmp);
			if (text) {
				rst.first = true;
			}
		}
		if ((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z')) i++;
	}
	return rst.second;
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

static vector<string> vpaths;
static HMODULE ogg_dll = NULL;

static bool InitOgg() {
	ifstream ifsvp(VPATH);
	string s;
	char buff[1024];

	while (ifsvp.getline(buff, sizeof(buff))) {
		vpaths.push_back(buff);
	}
	ifsvp.close();

	if (vpaths.empty()) {
		cout << "No paths found in " VPATH ", Exit." << endl;
		return false;
	}

	constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
	constexpr char STR_ov_info[] = "ov_info";
	constexpr char STR_ov_read[] = "ov_read";
	constexpr char STR_ov_clear[] = "ov_clear";
	constexpr char STR_ov_pcm_total[] = "ov_pcm_total";
	constexpr char STR_vorbisfile_dll[] = "vorbisfile.dll";
	constexpr char STR_DIR[] = "dll";

	SetDllDirectoryA(STR_DIR);
	ogg_dll = LoadLibraryA(STR_vorbisfile_dll);
	if (!ogg_dll) {
		cout << "Load ogg's libraries failed. exit." << endl;
		return false;
	}
	if (ogg_dll) {
		void* ov_open_callbacks = (void*)GetProcAddress(ogg_dll, STR_ov_open_callbacks);
		void* ov_info = (void*)GetProcAddress(ogg_dll, STR_ov_info);
		void* ov_read = (void*)GetProcAddress(ogg_dll, STR_ov_read);
		void* ov_clear = (void*)GetProcAddress(ogg_dll, STR_ov_clear);
		void* ov_pcm_total = (void*)GetProcAddress(ogg_dll, STR_ov_pcm_total);

		if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear || !ov_pcm_total) {
			cout << "Load ogg's apis failed. exit." << endl;
			FreeLibrary(ogg_dll);
			ogg_dll = NULL;
			return false;
		}

		Ogg::SetOggApis(ov_open_callbacks, ov_info, ov_read, ov_clear, ov_pcm_total);
	}

	return true;
}

static string GetStrVlen(const string vid) {
	static auto ogg = Ogg::ogg;

	string voice_id = vid;
	if (vid.length() <= MAX_VOICEID_LEN_NEED_MAPPING) {
		unsigned i_vid = 0;
		for(char c : vid) {
			if (c < '0' || c > '9') return "";
			i_vid *= 10; i_vid += c - '0';
		}
		i_vid += VoiceIdAdjustAdder[vid.length()];
		if (i_vid >= NUM_MAPPING) {
			return "";
		}
		voice_id = VoiceIdMapping[i_vid];
	}
	char buff[24];
	for (const auto& vp : vpaths) {
		string p = vp + "\\ch" + voice_id + ".ogg";
		if (!ogg->Open(p.c_str())) return "";

		int vlen = ogg->SamplesTotal() * 1000 / ogg->WaveFormat.nSamplesPerSec;
		ogg->Close();

		if (vlen > 0x3FFFF) vlen = 0x3FFFF;
		sprintf(buff, "vlen=%d,0x%04X", vlen, vlen);
		return buff;
	}
	return "";
}

static inline bool HasOpA5(const string& str) {
	size_t i = 0;
	while (i < str.length()) {
		while (i < str.length() && str[i] != '#') i++;
		if (i >= str.length()) break;
		i++;
		if (str[i + 1] == '#') break;
		string tmp;
		while (str[i] >= '0' && str[i] <= '9') i++;
		if (str[i] == 'A') return true;
	}

	i = 0;
	while (i < str.length()) {
		while (i + 4 < str.length() && !(str[i] == '[' && str[i + 1] == 'x' && str[i + 4] == ']')) i++;
		if (i + 4 >= str.length()) break;

		int hv = 0;
		i+= 2;
		for (int j = 0; j < 2; j++) {
			hv <<= 4;
			if (str[i + j] >= '0' && str[i + j] <= '9') hv += str[i + j] - '0';
			else if (str[i + j] >= 'a' && str[i + j] <= 'f') hv += str[i + j] - 'a' + 10;
			else if (str[i + j] >= 'A' && str[i + j] <= 'F') hv += str[i + j] - 'A' + 10;
			else { hv = 0; break; };
		}
		i += 5;

		if (hv == 5) return true;
		else if (hv == Sora::OP::SCPSTR_CODE_ITEM) {
			i += 2 * 5;
		}
		else if (hv == Sora::OP::SCPSTR_CODE_COLOR) {
			i += 1 * 5;
		}
	}
	return false;
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
	bool vlenA5 = switches.find('l') != switches.end();
	bool vlenAll = false; //switches.find('L') != switches.end();

	if (vlenA5 || vlenAll) {
		if (!InitOgg()) {
			cout << "Warning: Init ogg failed." << endl;
			vlenA5 = vlenAll = false;
		}
	}

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
		string lst_vid;
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

					lst_vid.clear();
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
				if (lt != LineType::Empty) 
				{
					auto vid_rst = GetVoiceId(s2);
					const auto& vids = vid_rst.second;
					if (!vids.empty() && !std::equal(TOut::NOT_MATCHED_DIALOG.cbegin(), TOut::NOT_MATCHED_DIALOG.cend(), s.c_str())) {
						ss_err << "    [Wanning]TXT2, line " << line_no << ", VOICE ID NOT INPUT\n";
					}
				}
			}
			else {
				if (GetChrID(s2) != Talk::InvalidChrId) {
					ss_err << "    [Wanning]TXT2, line " << line_no << ",  CHRID LINE NOT MATCHED\n";
				}
				if (GetType(s2) != Talk::InvalidTalk) {
					ss_err << "    [Wanning]TXT2, line " << line_no << ", DIALOG START LINE NOT MATCHED\n";
				}

				auto vid_rst = GetVoiceId(s2);
				const auto& vids = vid_rst.second;
				if(vid_rst.first) {
					ss_err << "    [Wanning]TXT2, line " << line_no << ", VOICE ID IN MIDDLE\n";
				}
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

					if (vlenAll) {
						auto str_vlen = GetStrVlen(vids[0]);
						if (!str_vlen.empty()) {
							str_vlen = "\t\t\t## " + str_vlen;
						}
						s.append(str_vlen);
					}
					else if(vlenA5) {
						lst_vid = vids[0];
					}
				}

				if (!lst_vid.empty() && HasOpA5(s)) {
					auto str_vlen = GetStrVlen(lst_vid);
					if (!str_vlen.empty()) {
						str_vlen = "\t\t\t## " + str_vlen;
						auto opAs = GetOp(s2, 'A');
						for (const auto& opA : opAs) {
							str_vlen.append(" #" + opA + "A");
						}
					}
					s.append(str_vlen);
					lst_vid.clear();
				}
				if (s.find("\\x02") != string::npos) lst_vid.clear();
			}

			ss_new << s << '\n';
		}

		if (ss_new.str().empty()) {
			ofs_rep << name << ":Error Exist:\n" << ss_err.str() << '\n';
			has_err = true;
		}
		else {
			Txt txt(ss_new);
			if (!txt.ErrMsg().empty()) {
				ofs_rep << name << ":Error Exist:\n" << txt.ErrMsg() << "\n\n";
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
	if (ogg_dll) {
		FreeLibrary(ogg_dll);
	}
	if (has_err) {
		cout << "Wannings/Errors found, check " << prep << " for details" << endl;
		system("pause");
	}

	return 0;
}
