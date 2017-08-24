#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <iomanip>

#include <Sora/MBin.h>
#include <Sora/TalkOut.h>
#include <Utils/Files.h>
#include <Utils/Encode.h>

#include <conio.h>

#define ATTR_SNT "._SN.txt"
#define ATTR_MBIN ".mbin"

#define DFT_REPORT_NAME "report.txt"

using namespace std;

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
	if (params.size() < 2) {
		std::cout << "Usage:\n"
			"\t" "SoraOutputTalk [-u] dir_snt dir_mbin [dir_snt_out] [dir_mbin_out] [report]\n"
			"\n"
			"    -u : set mbin's codepage to utf8.(Default is Shift-JIS)\n"
			"Default: dir_snt_out = dir_snt.out\n"
			"         dir_mbin_out = dir_mbin.out\n"
			"         report = " DFT_REPORT_NAME "\n"
			<< endl;
		return 0;
	}
	bool utf8 = switches.find('u') != switches.end();

	string dir_snt = params[0]; while(!dir_snt.empty() && (dir_snt.back() == '\\' || dir_snt.back() == '/')) dir_snt.pop_back();
	string dir_msg = params[1]; while (!dir_msg.empty() && (dir_msg.back() == '\\' || dir_msg.back() == '/')) dir_msg.pop_back();

	string dir_snt_out = params.size() > 2 ? params[2] : dir_snt + ".out";
	string dir_msg_out = params.size() > 3 ? params[3] : dir_msg + ".out";

	string path_report = params.size() > 4 ? params[4] : DFT_REPORT_NAME;

	dir_snt.push_back('\\');
	dir_msg.push_back('\\');
	dir_snt_out.push_back('\\');
	dir_msg_out.push_back('\\');

	Utils::MakeDirectory(dir_snt_out);
	Utils::MakeDirectory(dir_msg_out);

	auto fn_snts = Utils::SearchFiles(dir_snt + "*" ATTR_SNT);

	ofstream ofs_rep(path_report);
	ofs_rep << std::setiosflags(ios::left) << std::setw(10) << "NAME" << std::setw(12) << "TalksNum"
		<< std::setw(12) << "DlgsNum" << std::setw(12) << "NotMatchedDlgNum" << "\n"
		"--------------------------------------------------""\n";
	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.find('.'));
		std::cout << "Working with " << fn_snt << "..." << flush;

		Snt snt;
		auto rst = snt.Create(dir_snt + fn_snt);
		if (rst) {
			std::cout << "Error with snt file, Line:" << rst << endl;
			system("pause");
			continue;
		}

		MBin mbin(dir_msg + name + ATTR_MBIN, utf8 ? MBin::Encode::UTF8 : MBin::Encode::SJIS);
		if (!mbin.ErrMsg().empty()) {
			std::cout << mbin.ErrMsg() << endl;
			system("pause");
			continue;
		}

		if(snt.Talks().empty() || mbin.Talks().empty()) {
			std::cout << "No talks, Skip." << endl;
			continue;
		}

		ofstream ofs_snt_out(dir_snt_out + name + ".txt");
		ofstream ofs_msg_out(dir_msg_out + name + ".txt");

		int not_mch_l = 0;
		int not_mch_r = 0;
		auto mtch_rst = TOut::GetMatchedDialogs(snt.PtrDialogs(), mbin.PtrDialogs());
		for (const auto& pit : mtch_rst.second) {
			const auto& t_snt = pit.first != snt.PtrDialogs().end() ? *pit.first : nullptr;
			const auto& t_mst = pit.second != mbin.PtrDialogs().end() ? *pit.second : nullptr;

			if (!t_snt) not_mch_l++;
			if (!t_mst) not_mch_r++;

			TOut::OutputTwoPtrDialog(ofs_snt_out, t_snt, ofs_msg_out, t_mst);
		}

//		auto mtch_rst = TOut::GetMatchedTalks(snt.Talks(), mbin.Talks());
//		for (const auto& pit : mtch_rst.second) {
//			const auto& t_snt = pit.first != snt.Talks().end() ? *pit.first : TOut::EmptyTalk;
//			const auto& t_mst = pit.second != mbin.Talks().end() ? *pit.second : TOut::EmptyTalk;
//
//			TOut::OutputTwoTalks(ofs_snt_out, t_snt, ofs_msg_out, t_mst);
//		}

		ofs_snt_out.close();
		ofs_msg_out.close();

		std::cout << endl;

		ofs_rep << std::setw(10) << name
			<< std::setw(12) << std::to_string(snt.Talks().size()) + "/" + std::to_string(mbin.Talks().size())
			<< std::setw(12) << std::to_string(snt.PtrDialogs().size()) + "/" + std::to_string(mbin.PtrDialogs().size())
			<< std::setw(12) << (not_mch_l || not_mch_r ? std::to_string(not_mch_l) + "/" + std::to_string(not_mch_r) : "")
			<< "\n";
	}

	ofs_rep.close();

	return 0;
}
