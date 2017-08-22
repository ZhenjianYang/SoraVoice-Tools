#include <iostream>
#include <fstream>
#include <string>
#include <set>

#include <Sora/MBin.h>
#include <Sora/TalkOut.h>
#include <Utils/Files.h>
#include <Utils/Encode.h>

#include <conio.h>

#define ATTR_SNT "._SN.txt"
#define ATTR_MBIN ".mbin"

using namespace std;

int main(int argc, char* argv[]) {
	set<char> switches;
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
		cout << "Usage:\n"
			"\t" "SoraOutputTalk [-u] dir_snt dir_mbin [dir_snt_out] [dir_mbin_out]\n"
			"\n"
			"    -u : set mbin's codepage to utf8\n"
			"Default: dir_snt_out = dir_snt.out\n"
			"         dir_mbin_out = dir_mbin.out\n"
			<< endl;
		return 0;
	}
	bool utf8 = switches.find('u') != switches.end();

	string dir_snt = params[0]; while(!dir_snt.empty() && (dir_snt.back() == '\\' || dir_snt.back() == '/')) dir_snt.pop_back();
	string dir_msg = params[1]; while (!dir_msg.empty() && (dir_msg.back() == '\\' || dir_msg.back() == '/')) dir_msg.pop_back();

	string dir_snt_out = params.size() > 2 ? params[2] : dir_snt + ".out";
	string dir_msg_out = params.size() > 3 ? params[3] : dir_msg + ".out";

	dir_snt.push_back('\\');
	dir_msg.push_back('\\');
	dir_snt_out.push_back('\\');
	dir_msg_out.push_back('\\');

	Utils::MakeDirectory(dir_snt_out);
	Utils::MakeDirectory(dir_msg_out);

	auto fn_snts = Utils::SearchFiles(dir_snt + "*" ATTR_SNT);

	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.find('.'));
		cout << "Working with " << fn_snt << "..." << flush;

		Snt snt;
		auto rst = snt.Create(dir_snt + fn_snt);
		if (rst) {
			cout << "Error with snt file, Line:" << rst << endl;
			system("pause");
			continue;
		}

		MBin mbin;
		rst = mbin.Create(dir_msg + name + ATTR_MBIN, utf8 ? Encode::GetChCount_Utf8 : Encode::GetChCount_SJis);
		if (rst) {
			cout << "Error with mbin file, Offset: 0x" << hex << rst << endl;
			system("pause");
			continue;
		}

		auto mtch_rst = TOut::GetMatchedTalks(snt.Talks(), mbin.Talks());

		ofstream ofs_snt_out(dir_snt_out + name + ".txt");
		ofstream ofs_msg_out(dir_msg_out + name + ".txt");

		for (const auto& pit : mtch_rst.second) {
			const auto& t_snt = pit.first != snt.Talks().end() ? *pit.first : TOut::EmptyTalk;
			const auto& t_mst = pit.second != mbin.Talks().end() ? *pit.second : TOut::EmptyTalk;

			TOut::OutputTwoTalks(ofs_snt_out, t_snt, ofs_msg_out, t_mst);
		}

		ofs_snt_out.close();
		ofs_msg_out.close();

		cout << endl;
	}

	return 0;
}
