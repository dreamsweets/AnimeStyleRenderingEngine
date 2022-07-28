#pragma once
#include "imgui/Window/IMGUIWindow.h"
#include "Resources/ResourceManager.h"
#include "imgui/IMGUIManager.h"

struct IconInfo {
	std::string name;
	ImGuiTexture imguiTexture;
	bool mustDelete = false;
	bool isDirectory = false;
};

class ContentsBrowser : public IMGUIWindow
{
public:
	virtual bool Init() override;
	virtual void Update(float DeltaTime) override;

private:
	void BackToParentDirectory();
	void MoveToSubDirectory(std::string directoryName);
	void UpdateICons(std::filesystem::path CurrentDirectory);

private:
	IconInfo m_DirectoryIcon;
	IconInfo m_FileIcon;
	std::filesystem::path m_CurrentDirectory;
	std::vector<IconInfo> m_vecCurrentDirectoryEntryIcon;
};