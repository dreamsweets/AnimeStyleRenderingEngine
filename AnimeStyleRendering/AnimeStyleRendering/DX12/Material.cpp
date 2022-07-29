#include "Material.h"
#include "Gfx.h"
#include "Resources/Texture/Texture.h"
#include "imgui/IMGUIManager.h"
#include "UI/TextureViewer.h"
#include "Resources/ResourceManager.h"

RootSignature::RootSignature()
{
	//Empty RootSignature
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		D3DCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		D3DCall(Gfx::GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
	}
}

Material::Material() : RootSignature{}
{
	//
}

void Material::SetShader(GraphicsShader* shader)
{
	m_Shader = shader;
	CreateRootSignature(shader);
	CreatePSO();
}

void Material::CreatePSO()
{
	SetDefaultPSOVariables();
	D3DCall(Gfx::GetDevice()->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PipelineState)));
}

void Material::SetDefaultPSOVariables()
{
	//Data Setting
	{
		auto DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		DepthStencilDesc.DepthEnable = m_EnableDepth;
		DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		DepthStencilDesc.StencilEnable = FALSE;

		auto RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		RasterizerDesc.CullMode = m_CullMode;

		m_PSODesc.InputLayout = { m_Shader->m_InputElements.data(), (unsigned int)m_Shader->m_InputElements.size() };
		m_PSODesc.pRootSignature = m_RootSignature.Get();
		m_PSODesc.VS = CD3DX12_SHADER_BYTECODE{ m_Shader->m_VertexShader.ShaderBlob.Get() };
		m_PSODesc.PS = CD3DX12_SHADER_BYTECODE{ m_Shader->m_PixelShader.ShaderBlob.Get() };
		m_PSODesc.RasterizerState = RasterizerDesc;
		m_PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		m_PSODesc.DepthStencilState = DepthStencilDesc;
		m_PSODesc.SampleMask = UINT_MAX;
		m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_PSODesc.NumRenderTargets = 1;
		m_PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_PSODesc.DSVFormat = m_EnableDepth? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;
		m_PSODesc.SampleDesc.Count = 1;
	}
}

void Material::CreateRootSignature(GraphicsShader* shader)
{
	std::vector<CD3DX12_ROOT_PARAMETER> vecRootParameter;
	std::vector<D3D12_DESCRIPTOR_RANGE> vecVSRange;
	std::vector<D3D12_DESCRIPTOR_RANGE> vecPSRange;
	std::vector<D3D12_STATIC_SAMPLER_DESC> vecSamplers;

	u32 HeapCurrentIndex = 0;

	auto CreateResourceData = [&](std::string Name, D3D_SHADER_INPUT_TYPE Type) ->MaterialResourceData& {
		auto index = (u32)m_vecMaterialResourceData.size();
		MaterialResourceData resoureData;
		m_vecMaterialResourceData.push_back(resoureData);
		m_mapMaterialResourceIndex.insert({ Name, index });
		auto& resourceData = m_vecMaterialResourceData.back();
		resourceData.Name = Name;
		resourceData.Type = Type;
		resourceData.HeapIndex = HeapCurrentIndex++;

		return resourceData;
	};

	//VertexShader 데이터 파싱
	for (auto& [Name, Data] : m_Shader->m_VertexShader.MapMetaData)
	{
		CD3DX12_DESCRIPTOR_RANGE range{};
		switch (Data.Type)
		{
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER:
		{
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, Data.BindPoint);
			vecVSRange.push_back(range);
			auto& resourceData = CreateResourceData(Name, Data.Type);

			resourceData.ConstantBuffer.Init(Gfx::GetDevice(), Data.DataSize);
			resourceData.Resource = resourceData.ConstantBuffer.GetResourceComPtr();
			break;
		}
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER:
		{
			CD3DX12_STATIC_SAMPLER_DESC StaticSampler = {};
			StaticSampler.Init(
				Data.BindPoint,
				D3D12_FILTER_ANISOTROPIC,
				//D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				0.0f,
				1U,
				D3D12_COMPARISON_FUNC_LESS_EQUAL,
				D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
				0.0f,
				9.0f,
				D3D12_SHADER_VISIBILITY_VERTEX,
				0U
			);
			vecSamplers.push_back(StaticSampler);
			break;
		}
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED:
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE:
		{
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Data.BindPoint);
			vecVSRange.push_back(range);
			CreateResourceData(Name, Data.Type);
			break;
		}
		default:
			"망험ㅋ";
			throw;
			break;
		}
	}

	//VertexShader에 리소스가 존재할 경우 파라미터 추가
	if (!vecVSRange.empty()) {
		CD3DX12_ROOT_PARAMETER param;
		param.InitAsDescriptorTable(vecVSRange.size(), vecVSRange.data(), D3D12_SHADER_VISIBILITY_VERTEX);
		vecRootParameter.push_back(param);
		m_HasParameterVS = true;
	}

	//PixelShader 데이터 파싱
	for (auto& [Name, Data] : m_Shader->m_PixelShader.MapMetaData)
	{
		CD3DX12_DESCRIPTOR_RANGE range{};
		switch (Data.Type)
		{
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER:
		{
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, Data.BindPoint);
			vecPSRange.push_back(range);
			auto& resourceData = CreateResourceData(Name, Data.Type);
			resourceData.ConstantBuffer.Init(Gfx::GetDevice(), Data.DataSize);
			resourceData.Resource = resourceData.ConstantBuffer.GetResourceComPtr();
			break;
		}
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER:
		{
			CD3DX12_STATIC_SAMPLER_DESC StaticSampler = {};
			StaticSampler.Init(
				Data.BindPoint,
				D3D12_FILTER_ANISOTROPIC,
				//D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				0.0f,
				1U,
				D3D12_COMPARISON_FUNC_LESS_EQUAL,
				D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
				0.0f,
				9.0f,
				D3D12_SHADER_VISIBILITY_PIXEL,
				0U
			);
			vecSamplers.push_back(StaticSampler);
			break;
		}
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED:
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE:
		{
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Data.BindPoint);
			vecPSRange.push_back(range);
			CreateResourceData(Name, Data.Type);
			break;
		}
		default:
			"망험ㅋ";
			throw;
			break;
		}
	}

	//픽셀 셰이더에 데이터가 존재할 경우 파라미터 추가
	if (!vecPSRange.empty()) {
		CD3DX12_ROOT_PARAMETER param;
		param.InitAsDescriptorTable(vecPSRange.size(), vecPSRange.data(), D3D12_SHADER_VISIBILITY_PIXEL);
		vecRootParameter.push_back(param);
		m_HasParameterPS = true;
	}
	// DesciptorHeap 할당
	// 현재 샘플러는 따로 Static Sampler에 넣어서 사용중인데, 이유는 설명자힙을 Sampler는 따로 생성해서 넣어주도록 되어있길래 ㅈ같아서 그럼. 걍 Static Sampler로 때려박고 공간 좀 더 커진 채로 두는 게 나은 것 같아서 냅둠.
	m_HeapSize = (u32)vecVSRange.size() + (u32)vecPSRange.size();
	if (m_HeapSize > 0)
	{
		m_DescHeap.Create(Gfx::GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (UINT)m_HeapSize, true);
	}
	//DesciptorHeap에서 PixelShader 데이터의 시작 Index가 어딘지 세팅
	m_HeapOffsetPS = (u32)vecVSRange.size();

	//RootSignatureDesc 만들기
	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	RootSignatureDesc.Init(vecRootParameter.size(), vecRootParameter.data(), vecSamplers.size(), vecSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//RootSignature 인터페이스 생성
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	if (FAILED(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
	{
		std::string errorString = (const char*)error->GetBufferPointer();
		MSGBox("Signature 생성 실패 : \n" + errorString);
		throw;
	}
	D3DCall(Gfx::GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	CreateConstantBuffers();
}

void Material::CreateShaderResourceView(u32 HeapIndex, ComPtr<ID3D12Resource> Resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc)
{
	if (HeapIndex >= m_vecMaterialResourceData.size()) return;

	m_vecMaterialResourceData[HeapIndex].Resource = Resource;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_DescHeap.hCPU(HeapIndex);
	Gfx::GetDevice()->CreateShaderResourceView(Resource.Get(), &desc, srvHandle);
}

void Material::CreateConstantBufferView(u32 HeapIndex)
{
	if (HeapIndex >= m_vecMaterialResourceData.size()) return;
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DescHeap.hCPU(HeapIndex);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
	cbDesc.BufferLocation = m_vecMaterialResourceData[HeapIndex].ConstantBuffer.GetGPUVirtualAddress();
	cbDesc.SizeInBytes = m_vecMaterialResourceData[HeapIndex].ConstantBuffer.GetBufferSize();
	Gfx::GetDevice()->CreateConstantBufferView(&cbDesc, handle);
}

void Material::CreateSamplerView(u32 HeapIndex, D3D12_SAMPLER_DESC& desc)
{
	if (HeapIndex >= m_vecMaterialResourceData.size()) return;
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DescHeap.hCPU(HeapIndex);

	Gfx::GetDevice()->CreateSampler(&desc, handle);
}

void Material::CreateConstantBuffers()
{
	for (auto& resource : m_vecMaterialResourceData)
	{
		if (resource.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
			CreateConstantBufferView(resource.HeapIndex);
	}
}

void Material::SetTexture(std::string key, Texture* texture)
{
	for (auto it = m_mapMaterialResourceIndex.lower_bound(key); it != m_mapMaterialResourceIndex.upper_bound(key); it++)
	{
		auto& resourceData = m_vecMaterialResourceData[it->second];
		if (resourceData.Type != D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE) continue;
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Texture2D.MipLevels = texture->m_TextureDesc.MipLevels;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Format = texture->m_TextureDesc.Format;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_vecMaterialResourceData[it->second].Resource = texture->m_TextureResource;
		Gfx::GetDevice()->CreateShaderResourceView(texture->m_TextureResource.Get(), &SRVDesc, m_DescHeap.hCPU(m_vecMaterialResourceData[it->second].HeapIndex));
	}
}

void Material::SetTexture(std::string key, ComPtr<ID3D12Resource> resource)
{
	if (!resource) return;
	for (auto it = m_mapMaterialResourceIndex.lower_bound(key); it != m_mapMaterialResourceIndex.upper_bound(key); it++)
	{
		auto& resourceData = m_vecMaterialResourceData[it->second];
		if (resourceData.Type != D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE) continue;
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		auto resourceDesc = resource->GetDesc();
		SRVDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Format = resourceDesc.Format;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_vecMaterialResourceData[it->second].Resource = resource;
		Gfx::GetDevice()->CreateShaderResourceView(resource.Get(), &SRVDesc, m_DescHeap.hCPU(m_vecMaterialResourceData[it->second].HeapIndex));
	}
}

void Material::SetStructuredBuffer(std::string key, ComPtr<ID3D12Resource> resource, u32 StructStride, u32 DataCount)
{
	for (auto it = m_mapMaterialResourceIndex.lower_bound(key); it != m_mapMaterialResourceIndex.upper_bound(key); it++)
	{
		auto& resourceData = m_vecMaterialResourceData[it->second];
		if (resourceData.Type != D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED) continue;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = DataCount;
		srvDesc.Buffer.StructureByteStride = StructStride;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		m_vecMaterialResourceData[it->second].Resource = resource;
		Gfx::GetDevice()->CreateShaderResourceView(resource.Get(), &srvDesc, m_DescHeap.hCPU(m_vecMaterialResourceData[it->second].HeapIndex));
	}
}

void Material::OnIMGUIRender()
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

	if(ImGui::TreeNodeEx((void*)this, flags, this->m_Name.c_str()))
	{
		ImGui::Text("Shader");
		ImGui::SameLine();
		if (m_Shader)
		{
			if (ImGui::Selectable(m_Shader->m_Name.c_str()))
			{
				//TODO : Shader 다른 걸로 교체가능하도록 기능 만들기
			}
		}
			ImGui::Text("Parameters");

			for (auto& data : m_vecMaterialResourceData)
			{
				switch (data.Type)
				{
				case D3D_SIT_CBUFFER:
				{
					ImGui::Text("< CBuffer Property >");
					ImGui::Text(data.Name.c_str());
					break;
				}
				case D3D_SIT_TEXTURE:
				{
					ImGui::Text("< Texture Property >");
					ImGui::Text(data.Name.c_str());
					if (data.ImGuiTexture.handle.ptr) IMGUIManager::Inst().RemoveTexture(data.ImGuiTexture);

					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
					srvDesc.Format = data.Resource->GetDesc().Format;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = 1;
					srvDesc.Texture2D.MostDetailedMip = 0;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

					data.ImGuiTexture = IMGUIManager::Inst().AddTexture(data.Resource.Get(), srvDesc);
					ImGui::SameLine();
					ImGui::ImageButton((ImTextureID)data.ImGuiTexture.handle.ptr, { 100, 100 });


					if (ImGui::BeginDragDropTarget())
					{
						const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM");
						if (payload)
						{
							std::string fileName = (char*)payload->Data;
							auto texture = ResourceManager::Inst().GetTexture(fileName);
							if (texture)
							{
								SetTexture(fileName, texture);
							}
						}
						ImGui::EndDragDropTarget();
					}
				}
				default:
					break;
				}
			}
		
		ImGui::TreePop();
	}
}

void Material::SetMaterial()
{
#ifdef _DEBUG
	if (m_PipelineState)
	#endif
	{
		Gfx::GetCommandList()->SetGraphicsRootSignature(m_RootSignature.Get());

		Gfx::GetCommandList()->SetPipelineState(m_PipelineState.Get());

		if (m_HasParameterVS || m_HasParameterPS) {
			ID3D12DescriptorHeap* ppHeaps[] = { m_DescHeap };
			Gfx::GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		}

		if (m_HasParameterVS){
			if (m_HasParameterPS){
				Gfx::GetCommandList()->SetGraphicsRootDescriptorTable(0, m_DescHeap.hGPUHeapStart);
				Gfx::GetCommandList()->SetGraphicsRootDescriptorTable(1, m_DescHeap.hGPU(m_HeapOffsetPS));
			}
			else{
				Gfx::GetCommandList()->SetGraphicsRootDescriptorTable(0, m_DescHeap.hGPUHeapStart);
			}
		}
		else {
			if (m_HasParameterPS) {
				Gfx::GetCommandList()->SetGraphicsRootDescriptorTable(0, m_DescHeap.hGPU(m_HeapOffsetPS));
			}
			else {
			}
		}
	}
}

void GeometryMaterial::SetDefaultPSOVariables()
{
	Material::SetDefaultPSOVariables();

	m_PSODesc.NumRenderTargets = 5;

		//현재 RenderManager에서 5개를 사용중
	m_PSODesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT; // WorldPos
	m_PSODesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT; // Diffuse
	m_PSODesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT; // Normal
	m_PSODesc.RTVFormats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT; // Bitangent
	m_PSODesc.RTVFormats[4] = DXGI_FORMAT_R32G32B32A32_FLOAT; // Tangent
	
	D3DCall(Gfx::GetDevice()->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PipelineState)));
}