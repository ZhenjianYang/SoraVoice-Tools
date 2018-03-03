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
		"  SoraMoveFuns idx1 py1 idx2 py2 scp_fun_1 [scp_fun_2 ...]\n"
		"    scp_fun >= 0 : move scp_fun from py1 to py2 (mode 1)\n"
		//"    scp_fun <= -0 : move -scp_fun from py1 to py2 (mode 2)\n"
		//"    NOTE: 0 != -0\n"
		<< endl;
}

#define ERROR_EXIST(condition) if(condition) { printUsage(); return -1; }
#define TBL "    "
#define STR_INDEX "-index="
#define STR_CALL TBL "Call("

static int getIndex(const char* buff) {
	if(*buff == '\0') return -1;

	int rst = 0;
	while(*buff >= '0' && *buff <= '9') {
		rst *= 10;
		rst += *buff - '0';
		++buff;
	}

	if(*buff != '\0') return -1;
	return rst;
}

int main(int argc, char* argv[]) {
	ERROR_EXIST(argc < 6);

	const unsigned id1 = getIndex(argv[1]);
	const string path_py1 = argv[2];
	const unsigned id2 = getIndex(argv[3]);
	const string path_py2 = argv[4];

	const string py1 = path_py1.substr(path_py1.rfind('\\') + 1);
	const string py2 = path_py1.substr(path_py2.rfind('\\') + 1);

	ERROR_EXIST(id1 < 0 || id1 > 7);
	ERROR_EXIST(id2 < 0 || id2 > 7);

	vector<unsigned> scp_ids;
	for (int i = 5; i < argc; i++) {
		int id = getIndex(argv[i]);
		ERROR_EXIST(id < 0);
		scp_ids.push_back(id);
	}
	ERROR_EXIST(scp_ids.size() < 1);

	FunPy fp1(path_py1);
	FunPy fp2(path_py2);

	int cnt = 0;
	vector<string> errs;

	constexpr int MinLinesNeedMove = 10;

	char strbuf[1024];

	for(auto scp_id : scp_ids) {
		if (scp_id >= fp1.ScpFuns().size()) {
			std::sprintf(strbuf, "%s: scp fun %d not exists.", py1.c_str(), scp_id);
			errs.push_back(strbuf);
			continue;
		}

		auto &scp_fun = fp1.ScpFuns()[scp_id];
		if (scp_fun.Lines().size() < MinLinesNeedMove) {
			std::sprintf(strbuf, "%s: scp fun no need to be moved, lines count : %d", py1.c_str(), scp_fun.Lines().size());
			errs.push_back(strbuf);
			continue;
		}

		bool err = false;
		unsigned new_scp_id = fp2.ScpFuns().size();
		std::sprintf(strbuf, STR_CALL "%d, %d)", id2, new_scp_id);
		const string str_new_call = strbuf;

		for (size_t i = 0; i < fp1.ScpFuns().size() && !err; i++) {
			if (i == scp_id) continue;

			for (auto& line : fp1.ScpFuns()[i].Lines()) {
				int t_id, t_scp_id;
				if (2 == sscanf(line.c_str(), STR_CALL "%d, %d)", &t_id, &t_scp_id)
					&& t_id == id1 && t_scp_id == scp_id) {
					line = str_new_call;
				}
			}
		}

		for (size_t i = 0; i < fp2.ScpFuns().size(); i++) {
			for (auto& line : fp2.ScpFuns()[i].Lines()) {
				int t_id, t_scp_id;
				if (2 == sscanf(line.c_str(), STR_CALL "%d, %d)", &t_id, &t_scp_id)
					&& t_id == id1 && t_scp_id == scp_id) {
					line = str_new_call;
				}
			}
		}

		string scp_name = scp_fun.Name();
		fp2.AddFun(std::move(scp_fun));
		scp_fun.Name() = scp_name;
		scp_fun.Lines().clear();

		scp_fun.AddLine(TBL "def " + scp_name + "(): pass");
		scp_fun.AddLine("");
		scp_fun.AddLine(TBL "label(\"" + scp_name + "\")");
		scp_fun.AddLine("");
		scp_fun.AddLine(str_new_call);
		scp_fun.AddLine(TBL "Return()");
		scp_fun.AddLine("");
		scp_fun.AddLine(TBL "# " + scp_name + " end");
		scp_fun.AddLine("");

		cnt++;
	}

	if (cnt > 0) {
		ofstream ofs1(path_py1);
		fp1.WriteTo(ofs1);
		ofs1.close();

		ofstream ofs2(path_py2);
		fp2.WriteTo(ofs2);
		ofs2.close();

		cout << py1 << ": " << cnt << " funs changed." << endl;
	}
	else {
		cout << py1 << ": No changed." << endl;
	}

	if (!errs.empty()) {
		cout << "Errors exist:\n";
		for (const auto& err : errs) {
			cout << "    " << err << "\n";
		}
		cout << flush;
		system("pause");
	}

	return 0;
}
