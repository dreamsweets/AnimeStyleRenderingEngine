#include "ContentsBrowser.h"

bool ContentsBrowser::Init()
{
	{
		IconInfo info;
		info.name = "DirectoryIcon.png";
		auto texture = ResourceManager::Inst().GetTexture("DirectoryIcon.png");

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		info.imguiTexture = IMGUIManager::Inst().AddTexture(texture->GetResource(), srvDesc);
		m_DirectoryIcon = info;
		info.mustDelete = false;
	}
	{
		IconInfo info;
		info.name = "FileIcon.png";
		auto texture = ResourceManager::Inst().GetTexture("FileIcon.png");

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		info.imguiTexture = IMGUIManager::Inst().AddTexture(texture->GetResource(), srvDesc);
		info.mustDelete = false;
		m_FileIcon = info;
	}
	
	m_CurrentDirectory = ResourceManager::Inst().GetAssetDirectory();
	
	UpdateICons(m_CurrentDirectory.string());
	
	return true;
}

void ContentsBrowser::Update(float DeltaTime)
{
	ImGui::Begin("Content Browser");
	if (ImGui::Button("<-"))
	{
		BackToParentDirectory();
	}

	static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	float cellSize = thumbnailSize + padding;

	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = (int)(panelWidth / cellSize);
	if (columnCount < 1)
		columnCount = 1;
	
	ImGui::Columns(columnCount, 0, false);

	for (auto& icon : m_vecCurrentDirectoryEntryIcon)
	{
		ImGui::PushID(icon.name.c_str());
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		
		ImGui::ImageButton((ImTextureID)icon.imguiTexture.handle.ptr, { thumbnailSize, thumbnailSize });
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", icon.name.c_str(), (icon.name.size() + 1) * sizeof(char));
			ImGui::EndDragDropSource();
		}
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if (!icon.isDirectory && ResourceManager::Inst().IsModel(icon.name))
			{
				auto model = ResourceManager::Inst().GetModel(icon.name);
				if (model)
				{
					
				}
			}
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (icon.isDirectory) MoveToSubDirectory(icon.name);
		}

		ImGui::Text(icon.name.c_str());
		
		ImGui::PopStyleColor();
		ImGui::NextColumn();
		ImGui::PopID();
	}
	ImGui::Columns(1);

	ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
	ImGui::SliderFloat("Padding", &padding, 0, 32);

	ImGui::End();
}

void ContentsBrowser::BackToParentDirectory()
{
	m_CurrentDirectory = m_CurrentDirectory.parent_path();
	UpdateICons(m_CurrentDirectory);
}

void ContentsBrowser::MoveToSubDirectory(std::string directoryName)
{
	m_CurrentDirectory /= directoryName;
	UpdateICons(m_CurrentDirectory);
}

void ContentsBrowser::UpdateICons(std::filesystem::path CurrentDirectory)
{
	for (auto& icon : m_vecCurrentDirectoryEntryIcon)
	{
		if (icon.mustDelete)
			IMGUIManager::Inst().RemoveTexture(icon.imguiTexture);
	}
	m_vecCurrentDirectoryEntryIcon.clear();

	for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
	{
		const auto& path = directoryEntry.path();
		auto relativePath = std::filesystem::relative(path, ResourceManager::Inst().GetAssetDirectory());
		std::string filenameString = relativePath.filename().string();

		auto CreateIcon = [&](const std::filesystem::directory_entry& entry, std::string filename)
		{
			IconInfo info;
			info.name = filename;
			if (entry.is_directory())
			{
				info.imguiTexture = m_DirectoryIcon.imguiTexture;
				info.mustDelete = m_DirectoryIcon.mustDelete;
				info.isDirectory = true;
				return info;
			}
			if (ResourceManager::Inst().IsTexture(filename))
			{
				auto texture = ResourceManager::Inst().GetTexture(filename);

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
				ZeroMemory(&srvDesc, sizeof(srvDesc));
				srvDesc.Format = texture->GetDesc().Format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				info.imguiTexture = IMGUIManager::Inst().AddTexture(texture->GetResource(), srvDesc);
				info.mustDelete = true;
				return info;
			}
			info.imguiTexture = m_FileIcon.imguiTexture;
			info.mustDelete = m_FileIcon.mustDelete;

			return info;
		};

		m_vecCurrentDirectoryEntryIcon.push_back(CreateIcon(directoryEntry, filenameString));
	}
}