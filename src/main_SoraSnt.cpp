#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>

#include <Sora/Snt.h>
#include <Sora/TalkOut.h>
#include <Utils/Files.h>
#include <Utils/Encode.h>

#define ATTR_SNT "._SN.txt"

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
			"\t" "SoraSnt dir_snt [dir_snt_out]\n"
			"\n"
			"Default: dir_snt_out = dir_snt.out\n"
			<< endl;
		return 0;
	}

	string dir_snt = params[0];  while (dir_snt.back() == '/' || dir_snt.back() == '\\') dir_snt.pop_back();
	string dir_out = params.size() >= 2 ? params[1] : dir_snt + ".out";

	Utils::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_snt.push_back('\\');
	vector<string> fn_snts;
	Utils::SearchFiles(dir_snt + "*" ATTR_SNT, fn_snts);

	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.find('.'));
		cout << "Working with " << fn_snt << "..." << flush;

		Snt snt;
		auto rst = snt.Create(dir_snt + fn_snt);
		if (rst) {
			std::cout << "Error with mbin file, Offset: 0x" << hex << rst << endl;
			system("pause");
			continue;
		}

		ofstream ofs(dir_out + name + ".txt");

		for (const auto& talk : snt.Talks()) {
			TOut::OutputTalk(ofs, talk);
		}
		ofs << endl;
		ofs.close();

		cout << endl;
	}

	return 0;
}
