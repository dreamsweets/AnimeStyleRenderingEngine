#include "SkyComponent.h"
#include "DX12/Material.h"
#include "Components/ComponentManager.h"
#include "Resources/ResourceManager.h"
#include "Components/Camera/CameraManager.h"

RegisterComponentData(SkyComponent)

void SkyComponent::Init()
{
	PrimitiveComponent::Init();
	m_Mesh = ResourceManager::Inst().GetModel("sphere.fbx")->GetVecMesh()[0];
}

void SkyComponent::Update()
{
	PrimitiveComponent::Update(); 
	auto Camera = CameraManager::Inst().GetMainCamera();
	GetTransform()->Position(Camera->Position());
}

void SkyComponent::Render()
{
	if (m_vecMaterial.empty() || !m_Mesh) return;

	for (auto& mat : m_vecMaterial)
	{
		mat->SetMaterial();
		m_Mesh->Render();
	}
}