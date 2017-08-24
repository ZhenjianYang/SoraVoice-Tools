#pragma once

#include "TalksFile.h"
#include "Encode.h"

#include <string>
#include <istream>

namespace Sora {

	class MBin : public TalksFile
	{
	public:
		static constexpr const int Id_Talks[] = {
			3, //AnonymousTalk
			1, //ChrTalk
			2, //NpcTalk
		};

		Encode::Encode GetEncode() const { return encode; }

		MBin(const std::string& filename, Encode::Encode encode = Encode::Encode::SJIS);
		MBin(std::istream& is, int size, Encode::Encode encode = Encode::Encode::SJIS);
		MBin(const char* buff, int size, Encode::Encode encode = Encode::Encode::SJIS);

	public:
		MBin(const MBin& _Other) : TalksFile(_Other), encode(_Other.encode) { }
		MBin& operator=(const MBin& _Other) {
			TalksFile::operator=(_Other);
			encode = _Other.encode;
			return *this;
		}
		MBin(MBin&& _Right) : TalksFile(std::move(_Right)), encode(_Right.encode) { }
		MBin& operator=(MBin&& _Right) {
			TalksFile::operator=(std::move(_Right));
			encode = _Right.encode;
			return *this;
		}

	protected:
		int Create(const char* buff, int size);
		Encode::Encode encode;
	};
}
