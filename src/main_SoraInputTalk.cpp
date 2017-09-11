#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <iomanip>
#include <memory>
#include <sstream>

#include <Sora/MBin.h>
#include <Sora/Snt.h>
#include <Sora/Py.h>
#include <Sora/Txt.h>
#include <Sora/TalkOut.h>
#include <Sora/Encode.h>
#include <Utils/Files.h>

#include <conio.h>

#define ATTR_SNT "._SN.txt"
#define ATTR_PY ".py"
#define ATTR_MBIN ".mbin"
#define ATTR_TXT ".txt"

#define DFT_REPORT_NAME "report.txt"

using namespace std;
using namespace Sora;

static inline void printUsage() {
	std::cout << "Usage:\n"
		"\n"
		"  SoraInputTalk -{Switches} format dir_in dir_txt [dir_out] [report]\n"
		"\n"
		"    Switches:\n"
		"        c : Add comments to output\n"
		"        v : remove voice commands\n"
		"    Possible values of format:\n"
		"        s : ._SN.txt\n"
		"        p : .py\n"
		"    Default:\n"
		"        dir_out = dir_in.out\n"
		"        report = " DFT_REPORT_NAME "\n"
		<< endl;
}

#define ERROR_EXIST(condition) if(condition) { printUsage(); return -1; }

static inline const char* getExt(char fmt) {
	switch (fmt)
	{
	case 's':return ATTR_SNT;
	case 'p':return ATTR_PY;
	default: return nullptr;
	}
}

static inline unique_ptr<TalksFile> getTalksFile(const std::string& fileName, char fmt) {
	switch (fmt)
	{
	case 's':return make_unique<Sora::Snt>(fileName);
	case 'p':return make_unique<Sora::Py>(fileName);;
	case 't':return make_unique<Sora::Txt>(fileName);
	default: return nullptr;
	}
}

static void RemoveOp(std::string& s, char op) {
	size_t i = 0;
	size_t idx = 0;
	while (i < s.length()) {
		if (s[i] == '#') {
			auto j = i + 1;
			while (s[j] >= '0' && s[j] <= '9') j++;
			if (s[j] == op) {
				i = j + 1;
				continue;
			}
		}

		s[idx++] = s[i++];
	}
	s.resize(idx);
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
	bool with_cmt = switches.find('c') != switches.end();
	bool remove_vc = switches.find('v') != switches.end();

	ERROR_EXIST(params.size() < 3);
	string fmts = params[0];
	ERROR_EXIST(fmts.length() != 1);
	char fmt = fmts[0];
	const char* ext = getExt(fmt);
	ERROR_EXIST(!ext);

	string dir_in = params[1]; while (!dir_in.empty() && (dir_in.back() == '\\' || dir_in.back() == '/')) dir_in.pop_back();
	string dir_txt = params[2];
	string dir_out = params.size() > 3 ? params[3] : dir_in + ".out";
	string prep = params.size() > 4 ? params[4] : DFT_REPORT_NAME;

	dir_in.push_back('\\');
	if(!dir_txt.empty()) dir_txt.push_back('\\');
	dir_out.push_back('\\');
	Utils::MakeDirectory(dir_out);

	ofstream ofs_rep(prep);

	auto fn_ins = Utils::SearchFiles(dir_in + "*" + ext);
	bool has_err = false;
	for (const auto &fn_in : fn_ins) {
		const string name = fn_in.substr(0, fn_in.find('.'));

		std::cout << "Working with " << fn_in << "..." << flush;

		auto ptf = getTalksFile(dir_in + fn_in, fmt);
		auto& tf = *ptf;
		if (!tf.ErrMsg().empty()) {
			std::cout << tf.ErrMsg() << endl;
			system("pause");
			continue;
		}

		int cnt_ipt = 0;
		int cnt_npt = 0;

		stringstream ss_err;

		const string fn_txt = name + ATTR_TXT;
		auto ptf_txt = !dir_txt.empty() ? getTalksFile(dir_txt + fn_txt, 't') : std::make_unique<TalksFile>();
		if (!ptf_txt->ErrMsg().empty()) {
			if(ptf_txt->ErrMsg() != "Open file failed.")
				ss_err << "    [Error]Txt file: " << ptf_txt->ErrMsg() << '\n';
			else {
				cout << "Txt file not exists.";
			}
			ptf_txt = std::make_unique<TalksFile>();
		}

		auto &tf_txt = *ptf_txt;
		for (size_t i = 0; i < tf_txt.Talks().size(); i++) {
			const auto& talk = tf_txt.Talks()[i];

			if (talk.GetType() == Talk::InvalidTalk) continue;

			if (i >= (int)tf.Talks().size()) {
				ss_err << "    [NotInput]: #" << talk.No() << '\n';
				cnt_npt++;
				continue;
			}

			auto& talk_ori = tf.Talks()[i];
			if (talk_ori.GetType() != talk.GetType()) {
				ss_err << "    [Wanning]: #" << talk.No() << ", Type Not Same.\n";
			}
			if (talk_ori.ChrId() != talk.ChrId()) {
				ss_err << "    [Wanning]: #" << talk.No() << ", ChrId Not Same.\n";
			}

			talk_ori = std::move(talk);
			cnt_ipt++;
		}
		tf.ResetPDialogs();

		if (remove_vc) {
			for (auto &talk : tf.Talks()) {
				for (auto &dlg : talk.Dialogs()) {
					for (auto &line : dlg.Lines()) {
						RemoveOp(line.text, 'V');
						RemoveOp(line.text, 'v');
					}
				}
			}
		}

		ofstream ofs_out(dir_out + fn_in);
		tf.WriteTo(ofs_out, with_cmt);
		ofs_out.close();

		std::cout << endl;
		ofs_rep << name << ": Total Dialogs: " << tf.Talks().size()
			<< ", Input Dialogs: " << cnt_ipt << (cnt_npt ? ", Not Input Dialogs: " + std::to_string(cnt_npt) : "") << "\n";

		if (!ss_err.str().empty()) {
			ofs_rep << ss_err.str();
			has_err = true;
		}
		ofs_rep << '\n';
	}

	ofs_rep.close();
	if (has_err) {
		cout << "Wannings/Errors found, check " << prep << " for details" << endl;
		system("pause");
	}

	return 0;
}
