#include "InspectorWindow.h"
#include "Scene/Entity.h"

#include "Components/ComponentManager.h"

#include "imgui/IMGUIManager.h"
#include "SceneHierarchyWindow.h"

bool InspectorWindow::Init()
{
	m_Name = "Inspector View";
	m_HierarchyWindow = IMGUIManager::Inst().AddWindow<SceneHierarchyWindow>();
	return true;
}

void InspectorWindow::Update(float DeltaTime)
{	
	ImGui::Begin(m_Name.c_str(), &m_Open, 0);

	m_Context = m_HierarchyWindow->GetSelectedEntity();
	
	if (!m_Context) {
		ImGui::End();
		return;
	}
	if (ImGui::Button("Add Component"))
	{
		ImGui::OpenPopup("AddComponentPopUp");
	}
	if (ImGui::BeginPopup("AddComponentPopUp"))
	{
		for (auto& name : ComponentManager::Inst().GetComponentNames())
		{
			if (ImGui::Selectable(name.c_str()))
			{
				ComponentManager::Inst().GetCreator(name)(m_Context);
			}
		}
		ImGui::EndPopup();
	}

	static char buf[128];
	sprintf(buf, m_Context->Name().c_str());
	ImGui::Text("Entity name: ");
	ImGuiInputTextFlags InputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue;
	ImGui::SameLine();  
	if(ImGui::InputText("##edit", buf, IM_ARRAYSIZE(buf), InputTextFlags))
		m_Context->Name(buf);
	
	// TODO : Componentµé UI Ãâ·Â
	for (auto& component : m_Context->m_vecComponent)
	{
		component->OnIMGUIRender();
	}
	ImGui::End();
}
