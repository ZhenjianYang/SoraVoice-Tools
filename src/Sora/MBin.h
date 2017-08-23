#pragma once

#include "Talk.h"

#include <functional>
#include <vector>
#include <string>
#include <istream>

class MBin
{
public:
	static constexpr const int Id_Talks[] = {
		3, //AnonymousTalk
		1, //ChrTalk
		2, //NpcTalk
	};

	using MBinLineT = struct { int lineNo; std::string content; };
	using SntLinesT = std::vector<MBinLineT>;

	TalksT& Talks() { return talks; }
	const TalksT& Talks() const { return talks; }
	PtrDialogList& PtrDialogs() { return pDialogs; }
	const PtrDialogList& PtrDialogs() const { return pDialogs; }

	int Create(const std::string& filename, std::function<int(const char*)>getChbytes);
	int Create(std::istream& is, int size, std::function<int(const char*)>getChbytes);
	int Create(const char* buff, int size, std::function<int(const char*)>getChbytes);

	MBin() = default;
protected:
	TalksT talks;
	PtrDialogList pDialogs;
};
