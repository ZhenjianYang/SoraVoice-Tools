#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <vector>

namespace Utils {
	void SearchFiles(const std::string& seachFileName, std::vector<std::string>& out_foundFiles, bool nameOnly = true);
	std::vector<std::string> SearchFiles(const std::string& seachFileName, bool nameOnly = true);
	bool MakeDirectory(const std::string& dir);
}

#endif // !__COMMON_H__
