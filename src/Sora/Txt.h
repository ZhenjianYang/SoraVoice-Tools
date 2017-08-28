#pragma once

#include "TalksFile.h"

#include <string>
#include <istream>

namespace Sora {

	class Txt : public TalksFile
	{
	public:
		static std::pair<bool, std::string> TxtStr2TalkStr(const std::string& str);
		static std::string TalkStr2TxtStr(const std::string& str);
		static void OutputTalk(std::ostream& os, const Talk& talk, bool with_cmt = false);

	public:
		Txt(const std::string& filename);
		Txt(std::istream& is);

		Txt(const TalksFile &talksFile) : TalksFile(talksFile) { }
		Txt& operator= (const TalksFile &talksFile) { TalksFile::operator=(talksFile); return *this; }
		Txt(TalksFile &&talksFile) : TalksFile(std::move(talksFile)) { }
		Txt& operator= (TalksFile &&talksFile) { TalksFile::operator=(std::move(talksFile)); return *this; }

		virtual bool WriteTo(std::ostream& os, bool with_cmt = false) const override;
		virtual bool WriteTo(const std::string& filename, bool with_cmt = false) const override;

	public:
		Txt(const Txt& _Other) : TalksFile(_Other){ }
		Txt& operator=(const Txt& _Other) {
			TalksFile::operator=(_Other);
			return *this;
		}
		Txt(Txt&& _Right) : TalksFile(std::move(_Right)){ }
		Txt& operator=(Txt&& _Right) {
			TalksFile::operator=(std::move(_Right));
			return *this;
		}

	protected:
		int Create(std::istream& is);
	};
}
