#pragma once

#include "Talk.h"

#include <vector>
#include <istream>

class CSnt
{
public:
	using CTalks = std::vector<CTalk>;
	using CLines = std::vector<std::string>;

	CTalks& Talks() { return talks; }
	const CTalks& Talks() const { return talks; }
	CLines& Lines() { return lines; }
	const CLines& Lines() const { return lines; }

	int Create(const char* filename);
	int Create(std::istream& is);

	CSnt() = default;
private:
	CTalks talks;
	CLines lines;
};
