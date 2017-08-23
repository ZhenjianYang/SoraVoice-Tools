#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>

#include <Sora/MBin.h>
#include <Sora/TalkOut.h>
#include <Utils/Files.h>
#include <Utils/Encode.h>

#define ATTR_MBIN ".mbin"

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
	if (params.size() < 1) {
		std::cout << "Usage:\n"
			"\t" "SoraMBin [-u] dir_mbin [dir_mbin_out]\n"
			"\n"
			"    -u : set mbin's codepage to utf8.(Default is Shift-JIS)\n"
			"Default: dir_mbin_out = dir_mbin.out\n"
			<< endl;
		return 0;
	}
	bool utf8 = switches.find('u') != switches.end();

	string dir_mbin = params[0];  while (dir_mbin.back() == '/' || dir_mbin.back() == '\\') dir_mbin.pop_back();
	string dir_out = params.size() >= 2 ? params[1] : dir_mbin + ".out";

	Utils::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_mbin.push_back('\\');
	vector<string> fn_mbins;
	Utils::SearchFiles(dir_mbin + "*" ATTR_MBIN, fn_mbins);

	for (const auto &fn_mbin : fn_mbins) {
		const string name = fn_mbin.substr(0, fn_mbin.find('.'));
		cout << "Working with " << fn_mbin << "..." << flush;

		MBin snt;
		auto rst = snt.Create(dir_mbin + fn_mbin, Encode::GetChCount_SJis);
		if (rst) {
			std::cout << "Error with mbin file, Offset: 0x" << hex << rst << endl;
			system("pause");
			continue;
		}

		ofstream ofs(dir_out + name + ".txt");

		for(const auto& talk : snt.Talks()) {
			TOut::OutputTalk(ofs, talk);
		}
		ofs << endl;
		ofs.close();

		cout << endl;
	}

	return 0;
}
