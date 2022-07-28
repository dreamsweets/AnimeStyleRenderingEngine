#include "Path.h"
#include <assert.h>
#include <fstream>

namespace fs = std::filesystem;

void
Path::SetDirectory(std::string path) {
	fs::current_path(path);
}

std::string Path::GetCurrentDirectory()
{
	return fs::current_path().string();
}

std::string
Path::Load(std::string path) {
	assert(IsExist(path));
	std::string buf{};
	std::ifstream fin{ path };

	if (fin.is_open())
	{
		fin >> buf;
	}
	return buf;
}
std::string Path::GetShaderPath() {
	return "C:/Projects/Git Repository/Anime/Engine/Shaders";
}
bool
Path::IsDirectory(std::string path) {
	std::filesystem::path filepath{ path };
	return std::filesystem::is_directory(filepath);
}
bool
Path::IsFile(std::string path) {
	std::filesystem::path filepath{ path };
	return std::filesystem::is_regular_file(filepath);
}
bool
Path::IsExist(std::string path) {
	std::filesystem::path filepath{ path };
	return std::filesystem::exists(filepath);
}
std::string Path::GetFileName(std::string path) {
	fs::path file_path(path);
	if (!file_path.has_filename()) return std::string{};
	return file_path.filename().string();
}
std::string Path::GetExtension(std::string path) {
	fs::path file_path(path);
	if (!file_path.has_extension())return std::string{};
	return file_path.extension().string();
}
std::vector<std::string> Path::SearchDirectory(std::string path) {
	std::vector<std::string> paths;
	std::filesystem::path directory_path{ path };
	assert(fs::exists(path));
	fs::directory_iterator itr(directory_path);
	while (itr != fs::end(itr)) {
		const fs::directory_entry& entry = *itr;
		paths.emplace_back(entry.path().string());
		itr++;
	}
	return paths;
}
std::vector<std::string> Path::FilesInDirectory(std::string path) {
	assert(IsExist(path));
	std::vector<std::string> files;
	for (const fs::directory_entry& entry : fs::recursive_directory_iterator(fs::path{ path }))
	{
		if (fs::is_regular_file(entry.path()))
		{
			files.emplace_back(entry.path().string());
		}
	}
	return files;
}
