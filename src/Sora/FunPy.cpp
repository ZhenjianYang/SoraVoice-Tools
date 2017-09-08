#include "FunPy.h"
#include <fstream>
#include <iomanip>
#include <algorithm>

using namespace std;

#define STR_TBL "    "
#define STR_BEG_ScpFunction STR_TBL "ScpFunction("
#define STR_END_ScpFunction STR_TBL ")"

#define STR_BEG_DEFFUN STR_TBL "def Function_"
#define STR_BEG_DEFFUN_SHORT STR_TBL "def "
#define STR_END_DEFFUN STR_TBL "SaveToFile()"

static const string str_ScpFunction = "ScpFunction(";
static const string str_TBL = "    ";

static constexpr int MAXCH_ONELINE = 10000;

Sora::FunPy::FunPy(const std::string & fileName)
{
	ifstream ifs(fileName);
	Create(ifs);
}

Sora::FunPy::FunPy(std::istream & istr)
{
	Create(istr);
}

void Sora::FunPy::WriteTo(std::ostream & os) const
{
	constexpr int com_start = 35;

	for (const auto& line : lines) {
		if (line.line_no == Index_DefScpFun) {
			os << STR_BEG_ScpFunction "\n";

			for (size_t funid = 0; funid < scpFuns.size(); funid++ ) {
				const auto& fun = scpFuns[funid];
				os << STR_TBL STR_TBL "\"" << fun.Name() << "\",";
				for (size_t i = 0; i < com_start + 1 - fun.Name().length() - sizeof(STR_TBL STR_TBL "\"" "\","); i++) {
					os << ' ';
				}
				os << "# " << setw(2) << setfill('0') << uppercase << hex << funid << setw(1) << ", " << dec << funid << '\n';
			}

			os << STR_END_ScpFunction "\n";
		}
		else if (line.line_no == Index_ScpFuns) {
			for (const auto& fun : scpFuns) {
				fun.WriteTo(os);
			}
		}
		else {
			os << line.line  << '\n';
		}
	}

	os.flush();
}

int Sora::FunPy::Create(std::istream & is)
{
	char buff[MAXCH_ONELINE + 1];
	lines.clear();
	scpFuns.clear();

	int cur = 0;
	bool DefScpFun_got = false;
	bool ScpFuns_got = false;

	for (int line_no = 1; is.getline(buff, sizeof(buff)); line_no++) {
		string s(line_no == 1 && buff[0] == '\xEF' && buff[1] == '\xBB' && buff[2] == '\xBF' ? buff + 3 : buff);

		if (cur == 0 && !DefScpFun_got) {
			if (std::equal(std::begin(STR_BEG_ScpFunction), std::end(STR_BEG_ScpFunction) - 1, s.c_str())) {
				DefScpFun_got = true;
				cur = Index_DefScpFun;

				lines.push_back({ cur, "" });
			}
		}

		if (cur == 0 && !ScpFuns_got) {
			if (std::equal(std::begin(STR_BEG_DEFFUN), std::end(STR_BEG_DEFFUN) - 1, s.c_str())) {
				ScpFuns_got = true;
				cur = Index_ScpFuns;

				lines.push_back({ cur, "" });
			}
		}

		if(cur == Index_ScpFuns
			&& (std::equal(std::begin(STR_END_DEFFUN), std::end(STR_END_DEFFUN) - 1, s.c_str()))) {
			cur = 0;
		}

		if (cur == 0) {
			lines.push_back({ line_no, std::move(s) });
		}
		else if (cur == Index_DefScpFun) {
			if (std::equal(std::begin(STR_END_ScpFunction), std::end(STR_END_ScpFunction) - 1, s.c_str())) {
				cur = 0;
			}
		}
		else {
			if (std::equal(std::begin(STR_BEG_DEFFUN), std::end(STR_BEG_DEFFUN) - 1, s.c_str())) {
				auto idx = s.find('(');
				if (idx == string::npos) {
					scpFuns.clear();
					return line_no;
				}

				AddFun(ScpFun(s.substr(sizeof(STR_BEG_DEFFUN_SHORT) - 1, idx + 1 - sizeof(STR_BEG_DEFFUN_SHORT))));
			}
			scpFuns.back().AddLine(s);
		}
	}

	return 0;
}
