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

	using TalksT = std::vector<Talk>;
	using SntLineT = struct { int lineNo; std::string content; };
	using SntLinesT = std::vector<SntLineT>;

	TalksT& Talks() { return talks; }
	const TalksT& Talks() const { return talks; }

	int Create(const std::string& filename, std::function<int(const char*)>getChbytes);
	int Create(std::istream& is, int size, std::function<int(const char*)>getChbytes);
	int Create(const char* buff, int size, std::function<int(const char*)>getChbytes);

	MBin() = default;
protected:
	TalksT talks;
};
