#pragma once

#include "TalksFile.h"

#include <vector>
#include <string>
#include <istream>
#include <ostream>

namespace Sora {
	class Py : public TalksFile
	{
	public:
		static std::pair<bool, std::string> PyStr2TalkStr(const std::string& str);
		static std::string TalkStr2PyStr(const std::string& str);
		static void OutputTalk(std::ostream& os, const Talk& talk, bool with_cmt = false);

	public:
		using PyLineT = struct { int lineNo; std::string content; };
		using PyLinesT = std::vector<PyLineT>;

		virtual bool WriteTo(std::ostream& os, bool with_cmt = false) const override;
		virtual bool WriteTo(const std::string& filename, bool with_cmt = false) const override;

		PyLinesT& Lines() { return lines; }
		const PyLinesT& Lines() const { return lines; }

		Py(const std::string& filename);
		Py(std::istream& is);

	public:
		Py(const Py& _Other) : TalksFile(_Other), lines(_Other.lines) { }
		Py& operator=(const Py& _Other) {
			TalksFile::operator=(_Other);
			lines = _Other.lines;
			return *this;
		}
		Py(Py&& _Right) : TalksFile(std::move(_Right)), lines(_Right.lines) { }
		Py& operator=(Py&& _Right) {
			TalksFile::operator=(std::move(_Right));
			lines = std::move(_Right.lines);
			return *this;
		}

	protected:
		PyLinesT lines;

		int Create(std::istream & is);
	};
}
