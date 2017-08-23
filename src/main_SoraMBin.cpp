#include <iostream>
#include <fstream>
#include <string>

#include <Sora/MBin.h>
#include <Utils/Files.h>
#include <Utils/Encode.h>

#define ATTR_SNT ".mbin"

using namespace std;

string fix(const string& str) {
	string rst;
	char buff[12];
	for (char c : str) {
		if (c < 0x20 && c >= 0x00) {
			sprintf(buff, "\\x%02X", (int)c);
			rst.append(buff);
		}
		else {
			rst.push_back(c);
		}
	}
	return rst;
}

int main(int argc, char* argv[]) {

	string dir_snt = argv[1];  while (dir_snt.back() == '/' || dir_snt.back() == '\\') dir_snt.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_snt + ".out";

	Utils::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_snt.push_back('\\');
	vector<string> fn_snts;
	Utils::SearchFiles(dir_snt + "*" ATTR_SNT, fn_snts);

	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.rfind(ATTR_SNT));
		cout << "Working with " << fn_snt << "..." << flush;

		MBin snt;
		auto rst = snt.Create(dir_snt + fn_snt, Encode::GetChCount_SJis);
		if (rst) {
			cout << "Error at Line:" << rst << endl;
			continue;
		}

		ofstream ofs(dir_out + fn_snt + ".txt");

		for(const auto& talk : snt.Talks()) {
			ofs << Talk::Str_TalkTypes[talk.Type()] << '\n';
			ofs << "0x" << std::uppercase << std::hex << talk.ChrId() << '\n';
			if(talk.Type() == Talk::NpcTalk) {
				ofs << "Name: " << talk.Name() << '\n';
			}

			for(const auto& dlg : talk.Dialogs()) {
				ofs << '\n';
				for(const auto& line : dlg.Lines()) {
					ofs << fix(line) << '\n';
				}
			}
			ofs << "---------------------------------------" << '\n';
		}
		ofs << endl;
		ofs.close();
		cout << endl;
	}

	return 0;
}
