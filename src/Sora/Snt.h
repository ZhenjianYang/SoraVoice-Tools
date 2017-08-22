#pragma once

#include "Talk.h"

#include <vector>
#include <string>
#include <istream>
#include <ostream>

class Snt
{
public:
	static constexpr const char* Str_Talks[] = {
		"text", //AnonymousTalk
		"say",  //ChrTalk
		"talk", //NpcTalk
	};

	using SntLineT = struct { int lineNo; std::string content; };
	using SntLinesT = std::vector<SntLineT>;

	TalksT& Talks() { return talks; }
	const TalksT& Talks() const { return talks; }
	SntLinesT& Lines() { return lines; }
	const SntLinesT& Lines() const { return lines; }

	int Create(const std::string& filename);
	int Create(std::istream& is);

	bool WriteTo(std::ostream& os) const;
	bool WriteTo(const std::string& filename) const;
	std::string ToString() const;

	Snt() = default;
protected:
	TalksT talks;
	SntLinesT lines;
};

inline static std::ostream& operator<<(std::ostream& os, const Snt& snt) {
	snt.WriteTo(os);
	return os;
}
