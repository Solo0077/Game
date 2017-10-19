#include "vfs_util.h"
#include <regex>
#include <Windows.h>

namespace vfs::str
{
	std::wstring wide(const std::string &s)
	{
		size_t srcLen = s.length();

		if (srcLen <= 0)
			return L"";

		std::wstring ret;
		ret.resize(srcLen);

		if (!MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)srcLen, ret.data(), (int)ret.size()))
			throw "Error converting UTF8 to UTF16";

		return ret;
	}

	const wchar_t *wide_c(const std::string &s)
	{
		return wide(s).c_str();
	}

	std::string narrow(const wchar_t *s)
	{
		size_t srcLen = wcslen(s);

		if (srcLen <= 0)
			return "";

		// This might expand to far more char's than wchar_t's
		int len = WideCharToMultiByte(CP_UTF8, 0, s, (int)srcLen, nullptr, 0, nullptr, nullptr);

		if (len == 0)
			throw "Error converting UTF16 to UTF8";

		std::string ret;
		ret.resize(len);

		if (WideCharToMultiByte(CP_UTF8, 0, s, (int)srcLen, ret.data(), (int)ret.size(), nullptr, nullptr) != len)
			throw "Error converting UTF16 to UTF8";

		return ret;
	}

	const char *narrow_c(const wchar_t *s)
	{
		return narrow(s).c_str();
	}
}

namespace vfs::util
{
	// "It makes the path conform to the NTFS standard"
	// https://stackoverflow.com/a/24703223 (modified)
	//const static std::regex RegexNTFSPath(R"reg(^[a-zA-Z]:(\\(((?![<>:"\/\\|?*]).)+((?<![ .])\\)?)*)?$)reg");

	const static std::regex FwdSlash("\\/");			// Matches '/'
	const static std::regex DoubleSlash(R"(\\\\)");		// Matches '\\'

	bool PathIsValid(const std::string &Path)
	{
		return true;
		//return std::regex_match(Path.begin(), Path.end(), RegexNTFSPath);
	}

	bool PathIsAbsolute(const std::string &Path)
	{
		// An absolute path can't have any aliases or expansions - ( .\ or ..\ )
		// and requires a drive letter - ( X: ).
		if (Path.find(".\\") != std::string::npos)
			return false;

		if (!isalpha(Path[0]) || Path[1] != ':')
			return false;

		return true;
	}

	void NormalizePath(std::string &Path)
	{
		// '/'  => '\'
		// '\\' => '\'
		Path = TrimSlash(std::regex_replace(std::regex_replace(Path, FwdSlash, "\\"), DoubleSlash, "\\"));
	}

	void SplitPath(const std::string &In, std::string &Path, std::string &Part)
	{
		// "Dir1\\Dir2\\Dir3\\MyFile.txt"   => {Path="Dir1\\Dir2\\Dir3\",   Part="MyFile.txt"}
		// "\\Dir1\\Dir2\\Dir3\\MyFile.txt" => {Path="\\Dir1\\Dir2\\Dir3\", Part="MyFile.txt"}
		// "Dir1\\Dir2\\Dir3\\MyDir4"       => {Path="Dir1\\Dir2\\Dir3\",   Part="MyDir4"}
		// "\\Dir1\\Dir2\\Dir3\\MyDir4"     => {Path="\\Dir1\\Dir2\\Dir3\", Part="MyDir4"}
		size_t delim = In.find_last_of('\\');

		if (delim == std::string::npos)
		{
			// '\' not found (i.e "MyFile.txt")
			Path = "";
			Part = In;
		}
		else if (delim == 0)
		{
			// '\' is at root (i.e "\\MyFile.txt")
			Path = "";
			Part = In.substr(1);
		}
		else
		{
			// Relative directory (i.e "Dir1\\Dir2\\Dir3\\MyFile.txt")
			Path = In.substr(0, delim);
			Part = In.substr(delim + 1);
		}
	}

	std::string &TrimSlash(std::string &Path)
	{
		// Remove the trailing '\' if there is one
		auto itr = Path.end() - 1;

		if (*itr == '\\')
			Path.erase(itr);

		return Path;
	}
}