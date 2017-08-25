#pragma once

#include "TalksFile.h"
#include "Encode.h"

#include <string>
#include <istream>

namespace Sora {

	class Txt : public TalksFile
	{
	public:
		static std::pair<bool, std::string> TxtStr2TalkStr(const std::string& str);
		static std::string TalkStr2TxtStr(const std::string& str);
		static void OutputTalk(std::ostream& os, const Talk& talk);

	public:
		Txt(const std::string& filename);
		Txt(std::istream& is);

		template<typename TalksContainer>
		Txt(const TalksContainer &talks) : TalksFile() {
			this->talks.reserve(std::size(talks));
			for (const auto& talk : talks) {
				this->talks.push_back(talk);
			}
			this->setPDialogs();
		}
		template<typename TalksContainer>
		Txt& operator=(const TalksContainer &talks) {
			this->talks.clear();
			this->talks.reserve(std::size(talks));
			for (const auto& talk : talks) {
				this->talks.push_back(talk);
			}
			this->err.clear();
			this->setPDialogs();
			return *this;
		}
		Txt(TalksT &&talks) : TalksFile() {
			this->talks = std::move(talks);
			this->setPDialogs();
		}
		Txt& operator=(TalksT &&talks) {
			this->talks = std::move(talks);
			this->err.clear();
			this->setPDialogs();
			return *this;
		}

		Txt(const TalksFile &talksFile) : TalksFile(talksFile) { }
		Txt& operator= (const TalksFile &talksFile) { TalksFile::operator=(talksFile); return *this; }
		Txt(TalksFile &&talksFile) : TalksFile(std::move(talksFile)) { }
		Txt& operator= (TalksFile &&talksFile) { TalksFile::operator=(std::move(talksFile)); return *this; }

		virtual bool WriteTo(std::ostream& os) const override;
		virtual bool WriteTo(const std::string& filename) const override;

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
