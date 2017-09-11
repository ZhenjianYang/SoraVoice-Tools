#include <Sora/FunPy.h>

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <vector>



using namespace std;
using namespace Sora;

static inline void printUsage() {
	std::cout << "Usage:\n"
		"\n"
		"  SoraMoveFuns py1 py2 count\n"
		<< endl;
}

#define ERROR_EXIST(condition) if(condition) { printUsage(); return -1; }
#define TBL "    "

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
	ERROR_EXIST(params.size() < 3);

	const string path_py1 = params[0];
	const string path_py2 = params[1];
	const int count = std::strtol(params[2].c_str(), nullptr, 10);

	ERROR_EXIST(count <= 0);

	FunPy fp1(path_py1);
	FunPy fp2(path_py2);

	if ((int)fp1.ScpFuns().size() < count) {
		cout << "No enough scpfuns in " << path_py1 << endl;
		return -1;
	}

	int idx1 = fp1.ScpFuns().size() - count;
	int idx2 = fp2.ScpFuns().size();
	const int cnt2 = idx2;

	for (int i = 0; i < count; i++, idx1++) {
		auto& scpFun = fp1.ScpFuns()[idx1];
		if (scpFun.Lines().size() < 10) continue;

		ScpFun newFun(scpFun.Name());
		fp2.AddFun(std::move(scpFun));

		newFun.AddLine(TBL "def " + newFun.Name() + "(): pass");
		newFun.AddLine("");
		newFun.AddLine(TBL "label(\"" + newFun.Name() + "\")");
		newFun.AddLine("");
		newFun.AddLine(TBL "Call(1, " + std::to_string(idx2) + ")");
		newFun.AddLine(TBL "Return()");
		newFun.AddLine("");
		newFun.AddLine(TBL "# " + newFun.Name() + " end");
		newFun.AddLine("");

		fp1.ScpFuns()[idx1] = std::move(newFun);
		idx2++;
	}

	if (idx2 > cnt2) {
		ofstream ofs1(path_py1);
		fp1.WriteTo(ofs1);
		ofs1.close();

		ofstream ofs2(path_py2);
		fp2.WriteTo(ofs2);
		ofs2.close();

		cout << idx2 - cnt2 << " funs changed." << endl;
	}
	else {
		cout << "No changed." << endl;
	}

	return 0;
}
