#include "TextureViewer.h"
#include "imgui/IMGUIManager.h"
#include "DX12/Gfx.h"

bool TextureViewer::Init()
{
	m_Name = "Texture Viewer";
	return true;
}

void TextureViewer::Update(float DeltaTime)
{
	ImGui::Begin(m_Name.c_str(), &m_Open, 0);
	for (auto& tex : m_textures)
	{
		ImGui::Text(tex.title.c_str());
		ImGui::Image((void*)tex.imguiTexture.handle.ptr, ImVec2{ (float)tex.computedSize.x, (float)tex.computedSize.y });
	}
	
	ImGui::End();
}

void TextureViewer::AddTexture(std::string title, ID3D12Resource* texture)
{
	TextureShowInfo textureInfo;
	
	textureInfo.title = title;

	D3D12_RESOURCE_DESC resourceDesc = texture->GetDesc();
	float ratio = (float)resourceDesc.Height / resourceDesc.Width;
	textureInfo.computedSize.x = g_ImageWidth;
	textureInfo.computedSize.y = g_ImageWidth * ratio;
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = resourceDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	
	textureInfo.imguiTexture = IMGUIManager::Inst().AddTexture(texture, srvDesc);

	m_textures.push_back(textureInfo);
}
void TextureViewer::AddDepthTexture(std::string title, ID3D12Resource* texture)
{
	TextureShowInfo textureInfo;

	textureInfo.title = title;

	D3D12_RESOURCE_DESC resourceDesc = texture->GetDesc();
	float ratio = (float)resourceDesc.Height / resourceDesc.Width;
	textureInfo.computedSize.x = g_ImageWidth;
	textureInfo.computedSize.y = g_ImageWidth * ratio;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT; //Depth Texture는 강제로 포맷변경
 	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	
	textureInfo.imguiTexture = IMGUIManager::Inst().AddTexture(texture, srvDesc);

	m_textures.push_back(textureInfo);
}