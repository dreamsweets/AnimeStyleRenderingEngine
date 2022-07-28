#pragma once
#include "DX12/DX12Common.h"

class Material;

struct TextureResourceInfo {

	DirectX::ScratchImage* Image{ nullptr };
	
	unsigned int    Width{ 0 };
	unsigned int    Height{ 0 };
	std::string FileName;
	std::string PathName;
	std::string FullPath;
};

class Texture {
	friend class Material;
public:
	bool LoadTexture(std::string FilePath);
	D3D12_RESOURCE_DESC& GetDesc() { return m_TextureDesc; }
	ID3D12Resource* GetResource() { return m_TextureResource.Get(); }
private:
	bool CreateResource(int index);

private:
	std::vector<TextureResourceInfo*> m_vecResourceInfo;
	ComPtr<ID3D12Resource> m_TextureResource;
	D3D12_RESOURCE_DESC m_TextureDesc;

};