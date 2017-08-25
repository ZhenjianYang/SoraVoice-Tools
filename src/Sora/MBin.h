#pragma once

#include "TalksFile.h"

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

		MBin(const std::string& filename);
		MBin(std::istream& is, int size);
		MBin(const char* buff, int size);

	public:
		MBin(const MBin& _Other) : TalksFile(_Other) { }
		MBin& operator=(const MBin& _Other) {
			TalksFile::operator=(_Other);
			return *this;
		}
		MBin(MBin&& _Right) : TalksFile(std::move(_Right)) { }
		MBin& operator=(MBin&& _Right) {
			TalksFile::operator=(std::move(_Right));
			return *this;
		}

	protected:
		int Create(const char* buff, int size);
	};
}
