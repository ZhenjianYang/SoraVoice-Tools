#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <iomanip>
#include <memory>

#include <Sora/MBin.h>
#include <Sora/Snt.h>
#include <Sora/Txt.h>
#include <Sora/TalkOut.h>
#include <Sora/Encode.h>
#include <Utils/Files.h>

#include <conio.h>

#define ATTR_SNT "._SN.txt"
#define ATTR_MBIN ".mbin"
#define ATTR_TXT ".txt"

#define DFT_REPORT_NAME ""

using namespace std;
using namespace Sora;

static inline void printUsage() {
	std::cout << "Usage:\n"
		"\n"
		"  SoraOutputTalk format1[format2] dir_in_1 [dir_in_2] [dir_out_1] [dir_out_2] [report]\n"
		"\n"
		"    format1/format2: assign formats of input folders dir_in_1 and dir_in_2.\n"
		"                     (If format2 isn't assigned, only output from dir_in_1)\n"
		"         Possible values of format1 and format2:\n"
		"             s : ._SN.txt\n"
		"             m : .mbin\n"
		"             t : .txt (same format with output files)\n"
		"\n"
		"   dir_in_1/dir_in_2: first/second input folder\n"
		"\n"
		"   dir_out_1/dir_out_2: first/second output folder\n"
		"      Default values:\n"
		"         dir_out_1 = dir_in_1.out\n"
		"         dir_out_2 = dir_in_2.out\n"
		"\n"
		"   report : report file\n"
		"\n"
		"   Examples:\n"
		"\n"
		"       SoraOutputTalk sm dir1 dir2 dir1out dir2out report.txt\n"
		"           Output dir1(._SN.txt) and dir2(.mbin) to dir1out and dir2out, with report file report.txt\n"
		"\n"
		"       SoraOutputTalk tt dir1 dir2\n"
		"           Output dir1(.txt) and dir2(.txt) to dir1.out and dir2.out\n"
		"\n"
		"       SoraOutputTalk m dir1\n"
		"           Output dir1(.mbin) to dir1.out\n"
		<< endl;
}

#define ERROR_EXIST(condition) if(condition) { printUsage(); return -1; }

static inline const char* getExt(char fmt) {
	switch (fmt)
	{
	case 's':return ATTR_SNT;
	case 'm':return ATTR_MBIN;
	case 't':return ATTR_TXT;
	default: return nullptr;
	}
}

static inline unique_ptr<TalksFile> getTalksFile(const std::string& fileName, char fmt) {
	switch (fmt)
	{
	case 's':return make_unique<Sora::Snt>(fileName);
	case 'm':return make_unique<Sora::MBin>(fileName);
	case 't':return make_unique<Sora::Txt>(fileName);
	default: return nullptr;
	}
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
	ERROR_EXIST(params.size() < 2);
	string fmts = params[0];
	ERROR_EXIST(fmts.length() != 1 && fmts.length() != 2);
	char fmt1 = fmts[0], fmt2 = fmts[1];
	const char* ext1 = getExt(fmt1);
	const char* ext2 = getExt(fmt2);
	ERROR_EXIST(!ext1);
	ERROR_EXIST(fmt2 && !ext2);

	string dir1, dir2, dir1_out, dir2_out, prep;
	ERROR_EXIST(ext2 && params.size() < 3);

	dir1 = params[1]; while(!dir1.empty() && (dir1.back() == '\\' || dir1.back() == '/')) dir1.pop_back();
	if (ext2) { dir2 = params[2]; while (!dir2.empty() && (dir2.back() == '\\' || dir2.back() == '/')) dir2.pop_back(); }

	if (ext2) {
		dir1_out = params.size() > 3 ? params[3] : dir1 + ".out";
		dir2_out = params.size() > 4 ? params[4] : dir2 + ".out";
		if (params.size() > 5) prep = params[5];

		dir1.push_back('\\');
		dir2.push_back('\\');
		dir1_out.push_back('\\');
		dir2_out.push_back('\\');
		Utils::MakeDirectory(dir1_out);
		Utils::MakeDirectory(dir2_out);
	}
	else {
		dir1_out = params.size() > 2 ? params[2] : dir1 + ".out";
		if (params.size() > 3) prep = params[3];

		dir1.push_back('\\');
		dir1_out.push_back('\\');
		Utils::MakeDirectory(dir1_out);
	}
	
	dir1_out.push_back('\\');

	auto fns1 = Utils::SearchFiles(dir1 + "*" + ext1);

	ofstream ofs_rep;
	if (!prep.empty()) ofs_rep.open(prep);

	ofs_rep << std::setiosflags(ios::left) << std::setw(10) << "NAME" << std::setw(12) << "TalksNum"
		<< std::setw(12) << "DlgsNum" << std::setw(12) << "NotMatchedDlgNum" << "\n"
		"--------------------------------------------------""\n";

	for (const auto &fn1 : fns1) {
		const string name = fn1.substr(0, fn1.find('.'));
		if (fmt1 == 't' && fn1.rfind(ATTR_SNT) == fn1.length() - (sizeof(ATTR_SNT) - 1)) continue;

		std::cout << "Working with " << fn1 << "..." << flush;

		auto ptf1 = getTalksFile(dir1 + fn1, fmt1);
		const auto& tf1 = *ptf1;
		if (!tf1.ErrMsg().empty()) {
			std::cout << tf1.ErrMsg() << endl;
			system("pause");
			continue;
		}

		if (!fmt2) {
			Sora::Txt txt1(std::move(tf1));
			ofstream ofs1_out(dir1_out + name + ".txt");
			txt1.WriteTo(ofs1_out);
			ofs1_out.close();

			ofs_rep << std::setw(10) << name
				<< std::setw(12) << txt1.Talks().size()
				<< std::setw(12) << txt1.PtrDialogs().size()
				<< "\n";

			std::cout << endl;
			continue;
		}

		const string fn2 = name + ext2;
		auto ptf2 = getTalksFile(dir2 + fn2, fmt2);
		const auto &tf2 = *ptf2;
		if (!tf2.ErrMsg().empty()) {
			std::cout << tf2.ErrMsg() << endl;
			system("pause");
			continue;
		}

		if (tf1.Talks().empty() && tf2.Talks().empty()) {
			std::cout << "No talks, Skip." << endl;
			continue;
		}

		ofstream ofs1_out;
		ofstream ofs2_out;
		if(!tf1.Talks().empty()) ofs1_out.open(dir1_out + name + ".txt");
		if(!tf2.Talks().empty()) ofs2_out.open(dir2_out + name + ".txt");

		int not_mch_l = 0;
		int not_mch_r = 0;
		auto mtch_rst = TOut::GetMatchedDialogs(tf1.PtrDialogs(), tf2.PtrDialogs());
		for (const auto& pit : mtch_rst.second) {
			const auto& t_snt = pit.first != tf1.PtrDialogs().end() ? *pit.first : nullptr;
			const auto& t_mst = pit.second != tf2.PtrDialogs().end() ? *pit.second : nullptr;

			if (!t_snt) not_mch_l++;
			if (!t_mst) not_mch_r++;

			TOut::OutputTwoPtrDialog(ofs1_out, t_snt, ofs2_out, t_mst);
		}

		ofs1_out.close();
		ofs2_out.close();

		std::cout << endl;

		ofs_rep << std::setw(10) << name
			<< std::setw(12) << std::to_string(tf1.Talks().size()) + "/" + std::to_string(tf2.Talks().size())
			<< std::setw(12) << std::to_string(tf1.PtrDialogs().size()) + "/" + std::to_string(tf2.PtrDialogs().size())
			<< std::setw(12) << (not_mch_l || not_mch_r ? std::to_string(not_mch_l) + "/" + std::to_string(not_mch_r) : "")
			<< "\n";
	}

	ofs_rep.close();

	return 0;
}
