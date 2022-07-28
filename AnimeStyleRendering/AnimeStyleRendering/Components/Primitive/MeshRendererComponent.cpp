#include "MeshRendererComponent.h"
#include "DX12/Material.h"
#include "Components/ComponentManager.h"
#include "Resources/ResourceManager.h"

RegisterComponentData(MeshRendererComponent)

void MeshRendererComponent::OnIMGUIRender()
{
	if (ImGui::TreeNodeEx(this, ImGuiTreeNodeFlags_DefaultOpen, "MeshRenderer")) {

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 100);
		ImGui::Text("Mesh");
		ImGui::NextColumn();
		std::string str;
		if (m_Mesh)
		{
			str = m_Mesh->GetName();
		}
		else {
			str = "Mesh is Not Setted";
		}

		ImGui::Selectable(str.c_str());

		ImGui::Columns(1);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 100);
		ImGui::Text("Material");
		ImGui::NextColumn();
		for (auto& mat : m_vecMaterial)
		{
			mat->OnIMGUIRender();
		}

		ImGui::Columns(1);

		ImGui::TreePop();
	}
}

void MeshRendererComponent::Render()
{
	if (m_vecMaterial.empty() || !m_Mesh) return;

	for (auto& mat : m_vecMaterial)
	{
		mat->SetMaterial();
		m_Mesh->Render();
	}
}