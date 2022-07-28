#include "RenderManager.h"
#include "Resources/ResourceManager.h"

#include "Components/Light/LightComponent.h"
#include "Components/Primitive/MeshRendererComponent.h"
#include "Components/Primitive/RayTracingMeshComponent.h"
#include "Components/Primitive/SkyComponent.h"

#include "Components/Camera/CameraManager.h"

#include"imgui/IMGUIManager.h"
#include "UI/SceneHierarchyWindow.h"
#include "UI/InspectorWindow.h"
#include "UI/TextureViewer.h"

#include "DX12/Gfx.h"

#include "DX12/Material.h"
#include "Resources/Mesh/Model.h"
#include "Resources/Sky/Sky.h"

#include "DX12/VertexBuffer.h"
#include "DX12/IndexBuffer.h"

#include "UI/TextureViewer.h"

IMPLEMENT_SINGLE(RenderManager)

void RenderManager::Init()
{
	Gfx::Init();
	IMGUIManager::Inst().Init(Gfx::GetHWND(),
		Gfx::GetDevice(),
		Gfx::g_NumFrames);
	PrepareGeometryPass();
	PrepareRaytracingPass();
	PrepareLightPass();
	PreparePostProcessingPass();
	InitImGui();
}

void RenderManager::Register(PrimitiveComponent* c)
{
	u32 id = (u32)(-1);
	if (c->GetID() != id) throw;
	if (typeid(*c) == typeid(MeshRendererComponent))
	{
		auto meshComponent = (MeshRendererComponent*)c;
		u32 componentIndex = (u32)m_vecMeshRendererComponent.size();
		m_vecMeshRendererComponent.emplace_back(meshComponent);

		if (m_deqFreeMeshIndex.empty())
		{
			id = (u32)m_vecMeshIndex.size();
			m_vecMeshIndex.emplace_back(componentIndex);
		}
		else
		{
			id = m_deqFreeMeshIndex.front();
			m_deqFreeMeshIndex.pop_front();
			m_vecMeshIndex[id] = componentIndex;
		}
		meshComponent->SetID(id);
		return;
	}

	else if (typeid(*c) == typeid(RayTracingMeshComponent))
	{
		auto raytracingMeshComponent = (RayTracingMeshComponent*)c;
		
		u32 componentIndex = (u32)m_vecRaytracingMeshComponent.size();
		m_vecRaytracingMeshComponent.emplace_back(raytracingMeshComponent);
		
		if (m_deqFreeRaytracingMeshIndex.empty())
		{
			id = (u32)m_vecRaytracingMeshIndex.size();
			m_vecRaytracingMeshIndex.emplace_back(componentIndex);
		}
		else
		{
			id = m_deqFreeRaytracingMeshIndex.front();
			m_deqFreeRaytracingMeshIndex.pop_front();
			m_vecRaytracingMeshIndex[id] = componentIndex;
		}
		
		raytracingMeshComponent->SetID(id);

		m_TopLevelASDirty = true;
		
		return;
	}
	else if (typeid(*c) == typeid(SkyComponent))
	{
		m_Sky = (SkyComponent*)c;
	}
}

void RenderManager::Unregister(PrimitiveComponent* c)
{
	if (c->GetID() == (u32)-1) return;

	if (typeid(*c) == typeid(MeshRendererComponent))
	{
		auto meshComponent = (MeshRendererComponent*)c;
		auto vecBackComponent = m_vecMeshRendererComponent.back();
		
		m_vecMeshIndex[vecBackComponent->GetID()] = m_vecMeshIndex[meshComponent->GetID()];
		EraseUnordered(m_vecMeshRendererComponent, m_vecMeshIndex[meshComponent->GetID()]);
		m_vecMeshIndex[meshComponent->GetID()] = (u32)-1;
		m_deqFreeMeshIndex.push_back(meshComponent->GetID());
		meshComponent->SetID((u32)-1);
	}

	else if (typeid(*c) == typeid(RayTracingMeshComponent))
	{
		auto raytracingMeshComponent = (RayTracingMeshComponent*)c;
		auto vecBackRaytracingComponent = m_vecRaytracingMeshComponent.back();


		m_vecRaytracingMeshIndex[vecBackRaytracingComponent->GetID()] = m_vecRaytracingMeshIndex[raytracingMeshComponent->GetID()];
		EraseUnordered(m_vecRaytracingMeshComponent, m_vecRaytracingMeshIndex[raytracingMeshComponent->GetID()]);
		m_vecRaytracingMeshIndex[raytracingMeshComponent->GetID()] = (u32)-1;
		m_deqFreeRaytracingMeshIndex.push_back(raytracingMeshComponent->GetID());
		raytracingMeshComponent->SetID((u32)-1);

		m_TopLevelASDirty = true;
	}
}

void RenderManager::Register(LightComponent* c)
{
	// TODO : 추후 32개 이상의 라이트가 있을 경우 오브젝트 컬링 등등을 통해 추려낼 수 있도록 하기
	if (m_vecLightComponent.size() >= 32) return;

	m_vecLightComponent.emplace_back(c);
}

void RenderManager::Update()
{
	// TODO : 각 패스 별로 너무 데이터가 난잡하지 않나? 좀 적은 수로 줄일 방법을 생각해야 할 듯?
	auto Camera = CameraManager::Inst().GetMainCamera();
	
	Camera->UpdateMatrices();

	//Setting Geometry Constant Buffer
	m_PerFrameData.CamPosition = Camera->Position();
	m_PerFrameData.View = math::transpose(Camera->ViewMatrix());
	m_PerFrameData.Projection = math::transpose(Camera->ProjMatrix());
	m_PerFrameData.InverseView = math::transpose(Camera->InverseViewMatrix());

	IMGUIManager::Inst().Update();


	//Setting LightData
	auto lightCount = m_vecLightComponent.size();
	
	m_LightPassData.lightCount = (u32)lightCount;
	m_RaytracingPassData.lightCount = 0;
	
	for (auto i=0u; i<lightCount; ++i)
	{
		m_LightPassData.lights[i] = m_vecLightComponent[i]->GetData();
		m_LightPassData.lights[i].direction = math::normalize(m_LightPassData.lights[i].direction);
		
		if (m_vecLightComponent[i]->GetCastShadow())
		{
			m_RaytracingPassData.lights[m_RaytracingPassData.lightCount++] = m_vecLightComponent[i]->GetData();
		}
	}

	//Setting Raytracing Constant Buffer
	m_RaytracingPassData.camera.View = math::transpose(Camera->ViewMatrix());
	m_RaytracingPassData.camera.Proj = math::transpose(Camera->ProjMatrix());

	m_RaytracingPassData.camera.InverseView = math::transpose(Camera->InverseViewMatrix());
	m_RaytracingPassData.camera.InverseProj = math::transpose(Camera->InverseProjMatrix());
	m_RaytracingPassData.camera.CamPosition = Camera->Position();
	m_RaytracingPassData.camera.Near = Camera->Near();
	m_RaytracingPassData.camera.Far = Camera->Far();
	
	for (int i = 0; i < (int)m_RaytracingMaterial->GetRaygenSize(); ++i)
	{
		if (auto perFrameBuffer = m_RaytracingMaterial->GetRayGen(i)->GetCbuffer("PerFrame"))
		{
			perFrameBuffer->SetData(m_RaytracingPassData);
		}
	}
	
	if (auto perFrameBuffer = m_RaytracingMaterial->GetHitGroup("CameraHit")->GetCbuffer("PerFrame"))
	{
		perFrameBuffer->SetData(m_RaytracingPassData);
	}
}

void RenderManager::PostUpdate()
{
	m_vecLightComponent.clear();
}

void RenderManager::Render()
{
	Gfx::BeginFrame();
	ClearViews();
	RaytracingPass();
	SkyPass();
	GeometryPass();
	LightingPass();
	PostProcessingPass();
	Gfx::ResourceBarrier(Gfx::GetCommandList(), Gfx::GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	Gfx::GetCommandList()->OMSetRenderTargets(1, &Gfx::GetBackbufferRtvHandle(), false, &m_dsvHandle);
	IMGUIManager::Inst().Render(Gfx::GetCommandList());
	Gfx::ResourceBarrier(Gfx::GetCommandList(), Gfx::GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	Gfx::EndFrame();
}

void RenderManager::PostRender()
{
	m_vecLightComponent.clear();
}

void RenderManager::PrepareLightPass()
{
	m_LightingShader = new GraphicsShader;
	m_LightingShader->LoadByPath("Shaders/LightPassShader.fx");
	m_LightingMaterial = new Material;
	m_LightingMaterial->SetShader(m_LightingShader);
	m_LightingMaterial->EnableDepth(false);

	m_LightingMaterial->SetTexture("WorldPos", m_RtvTexture.WorldPos.first.Get());
	m_LightingMaterial->SetTexture("Diffuse", m_RtvTexture.Diffuse.first.Get());
	m_LightingMaterial->SetTexture("Normal", m_RtvTexture.Normal.first.Get());
	m_LightingMaterial->SetTexture("Bitangent", m_RtvTexture.Bitangent.first.Get());
	m_LightingMaterial->SetTexture("Tangent", m_RtvTexture.Tangent.first.Get());
	m_LightingMaterial->SetTexture("Depth", m_DepthTexture.Get());
	m_LightingMaterial->SetTexture("RaytracingResult", m_RaytracingMaterial->GetOutputTexure());
	
	m_LightCombineShader = new GraphicsShader;
	m_LightCombineShader->LoadByPath("Shaders/LightCombineShader.fx");
	m_LightCombineMaterial = new Material;
	m_LightCombineMaterial->SetShader(m_LightCombineShader);
	m_LightCombineMaterial->EnableDepth(false);
	m_LightCombineMaterial->SetTexture("LightPassResult", m_RtvTexture.LightResult.first.Get());
}

void RenderManager::ClearViews()
{
	Gfx::ResourceBarrier(Gfx::GetCommandList(), Gfx::GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	float color[4] = { 0,0,0,0 };
	Gfx::GetCommandList()->ClearRenderTargetView(Gfx::GetBackbufferRtvHandle(), color, 0, nullptr);
	Gfx::GetCommandList()->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0xff, 0, nullptr);


	Gfx::GetCommandList()->ClearRenderTargetView(m_RtvTexture.WorldPos.second, color, 0, nullptr);
	Gfx::GetCommandList()->ClearRenderTargetView(m_RtvTexture.Diffuse.second, color, 0, nullptr);
	Gfx::GetCommandList()->ClearRenderTargetView(m_RtvTexture.Normal.second, color, 0, nullptr);
	Gfx::GetCommandList()->ClearRenderTargetView(m_RtvTexture.Bitangent.second, color, 0, nullptr);
	Gfx::GetCommandList()->ClearRenderTargetView(m_RtvTexture.Tangent.second, color, 0, nullptr);
	Gfx::GetCommandList()->ClearRenderTargetView(m_RtvTexture.LightResult.second, color, 0, nullptr);
	Gfx::GetCommandList()->ClearRenderTargetView(m_RtvTexture.PostProcess.second, color, 0, nullptr);

	Gfx::ResourceBarrier(Gfx::GetCommandList(), Gfx::GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void RenderManager::PrepareGeometryPass()
{
	CreateDSV();
	CreateRTVs();
}

void RenderManager::PrepareRaytracingPass()
{
	m_RaytracingMaterial = new RayTracingMaterial;
	m_RaytracingMaterial->Load("Shaders/RTShader_usingInverseMat.fx");
}

void RenderManager::PreparePostProcessingPass()
{
	m_PostProcessingShader = new GraphicsShader;
	m_PostProcessingShader->LoadByPath("Shaders/PostProcessShader.fx");
	m_PostProcessingMaterial = new Material;
	m_PostProcessingMaterial->SetShader(m_PostProcessingShader);
	m_PostProcessingMaterial->EnableDepth(false);

	m_PostProcessingMaterial->SetTexture("PostProcessing", m_RtvTexture.PostProcess.first.Get());
}

void RenderManager::CreateDSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	D3DCall(Gfx::GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)));

	SIZE_T dsvDescriptorSize = Gfx::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CLEAR_VALUE ShadowDepthOptomizedClearValue = {};
	ShadowDepthOptomizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	ShadowDepthOptomizedClearValue.DepthStencil.Depth = 1.0f;
	ShadowDepthOptomizedClearValue.DepthStencil.Stencil = 0;

	D3D12_RESOURCE_DESC DepthTextureDesc = {};
	DepthTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	DepthTextureDesc.Alignment = 0;
	DepthTextureDesc.SampleDesc.Count = 1;
	DepthTextureDesc.SampleDesc.Quality = 0;
	DepthTextureDesc.MipLevels = 1;
	DepthTextureDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DepthTextureDesc.DepthOrArraySize = 1;
	DepthTextureDesc.Width = Gfx::GetWidth();
	DepthTextureDesc.Height = Gfx::GetHeight();
	DepthTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	DepthTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3DCall(Gfx::GetDevice()->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&DepthTextureDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&ShadowDepthOptomizedClearValue,
		IID_PPV_ARGS(&m_DepthTexture)));
	m_DepthTexture->SetName(L"Depth Buffer");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Format = DepthTextureDesc.Format;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	Gfx::GetDevice()->CreateDepthStencilView(m_DepthTexture.Get(), &dsvDesc, m_dsvHandle);
}

void RenderManager::CreateRTVs()
{
	m_rtvHeap = Gfx::CreateDescriptorHeap(10, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
	SIZE_T dsvDescriptorSize = Gfx::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_RESOURCE_DESC rtvTextureDesc = {};
	rtvTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rtvTextureDesc.Alignment = 0;
	rtvTextureDesc.SampleDesc.Count = 1;
	rtvTextureDesc.SampleDesc.Quality = 0;
	rtvTextureDesc.MipLevels = 1;
	rtvTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	rtvTextureDesc.DepthOrArraySize = 1;
	rtvTextureDesc.Width = Gfx::GetWidth();
	rtvTextureDesc.Height = Gfx::GetHeight();
	rtvTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rtvTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = rtvTextureDesc.Format;
	ClearValue.Color[0] = 0;
	ClearValue.Color[1] = 0;
	ClearValue.Color[2] = 0;
	ClearValue.Color[3] = 0;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);


	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	//Create WorldPos Texture
	{
		m_RtvTexture.WorldPos.second = rtvHandle;
		
		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&rtvTextureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&ClearValue,
			IID_PPV_ARGS(&m_RtvTexture.WorldPos.first)));
		m_RtvTexture.WorldPos.first->SetName(L"RenderTarget Texture : WorldPos");
	
		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = rtvTextureDesc.Format;
		desc.Texture2D.MipSlice = 0;
		Gfx::GetDevice()->CreateRenderTargetView(m_RtvTexture.WorldPos.first.Get(), &desc, m_RtvTexture.WorldPos.second);

	}

	rtvHandle.ptr += dsvDescriptorSize;
	//Create Diffuse Texture
	{
		m_RtvTexture.Diffuse.second = rtvHandle;

		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&rtvTextureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&ClearValue,
			IID_PPV_ARGS(&m_RtvTexture.Diffuse.first)));
		m_RtvTexture.Diffuse.first->SetName(L"RenderTarget Texture : Diffuse");

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = rtvTextureDesc.Format;
		desc.Texture2D.MipSlice = 0;
		Gfx::GetDevice()->CreateRenderTargetView(m_RtvTexture.Diffuse.first.Get(), &desc, m_RtvTexture.Diffuse.second);
	}

	rtvHandle.ptr += dsvDescriptorSize;
	//Create Normal Texture
	{
		m_RtvTexture.Normal.second = rtvHandle;

		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&rtvTextureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&ClearValue,
			IID_PPV_ARGS(&m_RtvTexture.Normal.first)));
		m_RtvTexture.Normal.first->SetName(L"RenderTarget Texture : Normal");

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = rtvTextureDesc.Format;
		desc.Texture2D.MipSlice = 0;
		Gfx::GetDevice()->CreateRenderTargetView(m_RtvTexture.Normal.first.Get(), &desc, m_RtvTexture.Normal.second);
	}

	rtvHandle.ptr += dsvDescriptorSize;
	//Create Bitangent Texture
	{
		m_RtvTexture.Bitangent.second = rtvHandle;

		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&rtvTextureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&ClearValue,
			IID_PPV_ARGS(&m_RtvTexture.Bitangent.first)));
		m_RtvTexture.Bitangent.first->SetName(L"RenderTarget Texture : Bitangent");

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = rtvTextureDesc.Format;
		desc.Texture2D.MipSlice = 0;
		Gfx::GetDevice()->CreateRenderTargetView(m_RtvTexture.Bitangent.first.Get(), &desc, m_RtvTexture.Bitangent.second);
	}

	rtvHandle.ptr += dsvDescriptorSize;
	//Create Tangent Texture
	{
		m_RtvTexture.Tangent.second = rtvHandle;

		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&rtvTextureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&ClearValue,
			IID_PPV_ARGS(&m_RtvTexture.Tangent.first)));
		m_RtvTexture.Tangent.first->SetName(L"RenderTarget Texture : Tangent");

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = rtvTextureDesc.Format;
		desc.Texture2D.MipSlice = 0;
		Gfx::GetDevice()->CreateRenderTargetView(m_RtvTexture.Tangent.first.Get(), &desc, m_RtvTexture.Tangent.second);
	}
	rtvHandle.ptr += dsvDescriptorSize;


	//아래는 결과 출력을 위한 텍스처
	rtvTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ClearValue.Format = rtvTextureDesc.Format;

	//Create LightingResult Texture
	{
		m_RtvTexture.LightResult.second = rtvHandle;
		
		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&rtvTextureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&ClearValue,
			IID_PPV_ARGS(&m_RtvTexture.LightResult.first)));
		m_RtvTexture.LightResult.first->SetName(L"RenderTarget Texture : LightResult");

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = rtvTextureDesc.Format;
		desc.Texture2D.MipSlice = 0;
		Gfx::GetDevice()->CreateRenderTargetView(m_RtvTexture.LightResult.first.Get(), &desc, m_RtvTexture.LightResult.second);
	}

	rtvHandle.ptr += dsvDescriptorSize;
	//Create PostProcess Texture
	{
		m_RtvTexture.PostProcess.second = rtvHandle;

		D3DCall(Gfx::GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&rtvTextureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&ClearValue,
			IID_PPV_ARGS(&m_RtvTexture.PostProcess.first)));
		m_RtvTexture.PostProcess.first->SetName(L"RenderTarget Texture : PostProcess");

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = rtvTextureDesc.Format;
		desc.Texture2D.MipSlice = 0;
		Gfx::GetDevice()->CreateRenderTargetView(m_RtvTexture.PostProcess.first.Get(), &desc, m_RtvTexture.PostProcess.second);
	}
}

void RenderManager::SkyPass()
{
	if (!m_Sky) return;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { m_RtvTexture.PostProcess.second };
	Gfx::GetCommandList()->OMSetRenderTargets(arraysize(rtvHandles), rtvHandles, false, &m_dsvHandle);
	PerObjectData PerObject{};
	PerObject.World = math::transpose(m_Sky->GetTransform()->GetFixedWorldMatrix());
	
	for (auto& material : m_Sky->m_vecMaterial)
	{
		material->SetCBufferValue("PerFrame", m_PerFrameData);
		material->SetCBufferValue("PerObject", PerObject);
	}
	m_Sky->Render();

	Gfx::GetCommandList()->OMSetRenderTargets(0, nullptr, false, nullptr);
}

void RenderManager::GeometryPass()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { m_RtvTexture.WorldPos.second, m_RtvTexture.Diffuse.second, m_RtvTexture.Normal.second, m_RtvTexture.Bitangent.second, m_RtvTexture.Tangent.second };
	Gfx::GetCommandList()->OMSetRenderTargets(arraysize(rtvHandles), rtvHandles, false, &m_dsvHandle);

	//Testing Code
	PerObjectData PerObject{};

	for (auto& component : m_vecMeshRendererComponent)
	{
		//Material Data 갱신
		{
			PerObject.World = math::transpose(component->GetTransform()->GetFixedWorldMatrix());
			for (auto& material : component->m_vecMaterial)
			{
				if (typeid(*material) != typeid(GeometryMaterial)) continue;
				material->SetCBufferValue("PerFrame", m_PerFrameData);
				material->SetCBufferValue("PerObject", PerObject);
			}
			component->Render();
		}
	}

	Gfx::GetCommandList()->OMSetRenderTargets(0, nullptr, false, nullptr);
}

void RenderManager::LightingPass()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { m_RtvTexture.LightResult.second };
	Gfx::GetCommandList()->OMSetRenderTargets(arraysize(rtvHandles), rtvHandles, false, nullptr);

	m_LightingMaterial->SetCBufferValue("LightPassData", m_LightPassData);
	m_LightingMaterial->SetMaterial();

	Gfx::GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Gfx::GetCommandList()->IASetVertexBuffers(0, 0, nullptr);
	Gfx::GetCommandList()->DrawInstanced(4, 1, 0, 0);

	Gfx::GetCommandList()->OMSetRenderTargets(0, nullptr, false, nullptr);

	CombineLightingResult();
}

void RenderManager::CombineLightingResult()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { m_RtvTexture.PostProcess.second };
	Gfx::GetCommandList()->OMSetRenderTargets(arraysize(rtvHandles), rtvHandles, false, nullptr);
	
	m_LightCombineMaterial->SetMaterial();

	Gfx::GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Gfx::GetCommandList()->IASetVertexBuffers(0, 0, nullptr);
	Gfx::GetCommandList()->DrawInstanced(4, 1, 0, 0);

	Gfx::GetCommandList()->OMSetRenderTargets(0, nullptr, false, nullptr);
}

void RenderManager::PostProcessingPass()
{
	//BackBuffer를 RTV로 세팅
	Gfx::ResourceBarrier(Gfx::GetCommandList(), Gfx::GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	Gfx::GetCommandList()->OMSetRenderTargets(1, &Gfx::GetBackbufferRtvHandle(), false, nullptr);
	
	m_PostProcessingMaterial->SetMaterial();
	Gfx::GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Gfx::GetCommandList()->IASetVertexBuffers(0, 0, nullptr);
	Gfx::GetCommandList()->DrawInstanced(4, 1, 0, 0);

	//BackBuffer 사용종료
	Gfx::ResourceBarrier(Gfx::GetCommandList(), Gfx::GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	Gfx::GetCommandList()->OMSetRenderTargets(0, nullptr, false, nullptr);
}

void RenderManager::RaytracingPass()
{
	//RayGen Shader에서 사용하는 상수버퍼 데이터를 갱신
	//m_RaytracingMaterial->GetRayGen(0)->GetCbuffer("Camera")->SetData(m_RTCamera);
	/*if (!m_TopLevelASDirty) {
		m_RaytracingMaterial->BuildTopLevelAS(m_vecRaytracingMeshComponent, true);
	}
	else
	{
		m_RaytracingMaterial->BuildTopLevelAS(m_vecRaytracingMeshComponent);
		m_TopLevelASDirty = false;
	}*/
	m_RaytracingMaterial->BuildTopLevelAS(m_vecRaytracingMeshComponent);
	m_RaytracingMaterial->Render();

	//처음 돌린 레이트레이싱 결과값을 두번째 놈에게 전달
	Gfx::ExecuteRTCommandList();
	m_RaytracingMaterial->RemovePrevFrameBuffers();
	Gfx::ResetRTCommandList();
}

void RenderManager::InitImGui(){
	IMGUIManager::Inst().AddWindow<SceneHierarchyWindow>();
	IMGUIManager::Inst().AddWindow<InspectorWindow>();
	auto texview = IMGUIManager::Inst().AddWindow<TextureViewer>();
	texview->AddTexture("Raytracing Result", m_RaytracingMaterial->GetOutputTexure());
	texview->AddTexture("Diffuse View", m_RtvTexture.Diffuse.first.Get());
	texview->AddTexture("WorldPos View", m_RtvTexture.WorldPos.first.Get());
	texview->AddTexture("Normal View", m_RtvTexture.Normal.first.Get());
	texview->AddTexture("Bitangent View", m_RtvTexture.Bitangent.first.Get());
	texview->AddTexture("Tangent View", m_RtvTexture.Tangent.first.Get());
	texview->AddTexture("LightResult View", m_RtvTexture.LightResult.first.Get());
	texview->AddTexture("PostProcess View", m_RtvTexture.PostProcess.first.Get());
	texview->AddDepthTexture("Depth View", m_DepthTexture.Get());
}
