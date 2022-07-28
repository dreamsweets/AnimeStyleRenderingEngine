#include "Sky.h"
#include "Resources/ResourceManager.h"
#include "DX12/Material.h"

bool Sky::Init()
{
	m_SkySphere = ResourceManager::Inst().GetModel("sphere.fbx");
	m_Material = new Material;
	auto shader = ResourceManager::Inst().GetShader("SkyShader.fx");

	if (!shader) return false;

	m_Material->SetShader(shader);
	
	m_Material->SetCullMode(D3D12_CULL_MODE_NONE);
	m_Material->EnableDepth(false);

	return true;
}

void Sky::Render()
{
	if (!m_Material || !m_SkySphere) return;

	m_Material->SetMaterial();
	for (auto& mesh : m_SkySphere->GetVecMesh())
	{
		mesh->Render();
	}
}

Sky::~Sky()
{
	if (m_Material) delete m_Material;
}
