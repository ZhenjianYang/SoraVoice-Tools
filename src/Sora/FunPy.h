#pragma once

#include <string>
#include <vector>
#include <istream>
#include <ostream>

namespace Sora {
	class ScpFun {
	public:
		using LineT = std::string;
		using LinesT = std::vector<LineT>;

		const std::string& Name() const { return name; }
		std::string& Name() { return name; }

		const LinesT& Lines() const { return lines; }
		LinesT& Lines() { return lines; }

		void AddLine(const LineT& line) {
			lines.push_back(line);
		}
		void AddLine(LineT&& line) {
			lines.push_back(std::move(line));
		}

		void WriteTo(std::ostream& os) const {
			for (const auto& line : lines) {
				os << line << '\n';
			}
		}

		ScpFun(const std::string& name) : name(name) { }
		ScpFun(std::string&& name) : name(std::move(name)) { }

	protected:
		std::string name;
		LinesT lines;
	};

	class FunPy {
	public:
		using LineT = struct {
			int line_no;
			std::string line;
		};
		using LinesT = std::vector<LineT>;
		using ScpFunsT = std::vector<ScpFun>;

		static constexpr int Index_DefScpFun = -1;
		static constexpr int Index_ScpFuns = -2;

		const ScpFunsT& ScpFuns() const { return scpFuns; }
		ScpFunsT& ScpFuns() { return scpFuns; }

		const LinesT& Lines() const { return lines; }
		LinesT& Lines() { return lines; }

		void AddFun(const ScpFun& scpFun) {
			scpFuns.push_back(scpFun);
		}
		void AddFun(ScpFun&& scpFun) {
			scpFuns.push_back(std::move(scpFun));
		}

		FunPy(const std::string& fileName);
		FunPy(std::istream& istr);

		void WriteTo(std::ostream& os) const;

	protected:
		int Create(std::istream& is);

	protected:
		LinesT lines;
		ScpFunsT scpFuns;
	};
}

