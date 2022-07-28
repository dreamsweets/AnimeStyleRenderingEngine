#pragma once
#include <filesystem>

#ifdef GetCurrentDirectory
#undef GetCurrentDirectory
#endif

class Path
{
public:
	static void SetDirectory(std::string path);
	static std::string GetCurrentDirectory();
	static std::string Load(std::string path);
	static std::string GetShaderPath();
	static bool IsDirectory(std::string path);
	static bool IsFile(std::string path);
	static bool IsExist(std::string path);
	static std::string GetFileName(std::string path);
	static std::string GetExtension(std::string path);
	static std::vector<std::string> SearchDirectory(std::string path);
	static std::vector<std::string> FilesInDirectory(std::string path);
};