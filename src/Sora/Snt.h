#pragma once

#include "TalksFile.h"

#include <vector>
#include <string>
#include <istream>
#include <ostream>

namespace Sora {
	class Snt : public TalksFile
	{
	public:
		static std::pair<bool, std::string> SntStr2TalkStr(const std::string& str);
		static std::string TalkStr2SntStr(const std::string& str);
		static void OutputTalk(std::ostream& os, const Talk& talk, bool with_cmt = false);

	public:
		static constexpr const char* Str_Talks[] = {
			"text", //AnonymousTalk
			"say",  //ChrTalk
			"talk", //NpcTalk
		};

		using SntLineT = struct { int lineNo; std::string content; };
		using SntLinesT = std::vector<SntLineT>;

		virtual bool WriteTo(std::ostream& os, bool with_cmt = false) const override;
		virtual bool WriteTo(const std::string& filename, bool with_cmt = false) const override;

		SntLinesT& Lines() { return lines; }
		const SntLinesT& Lines() const { return lines; }

		Snt(const std::string& filename);
		Snt(std::istream& is);

	public:
		Snt(const Snt& _Other) : TalksFile(_Other), lines(_Other.lines) { }
		Snt& operator=(const Snt& _Other) {
			TalksFile::operator=(_Other);
			lines = _Other.lines;
			return *this;
		}
		Snt(Snt&& _Right) : TalksFile(std::move(_Right)), lines(_Right.lines) { }
		Snt& operator=(Snt&& _Right) {
			TalksFile::operator=(std::move(_Right));
			lines = std::move(_Right.lines);
			return *this;
		}

	protected:
		SntLinesT lines;

		int Create(std::istream & is);
	};
}
