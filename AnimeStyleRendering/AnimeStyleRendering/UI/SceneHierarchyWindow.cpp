#include "SceneHierarchyWindow.h"
#include "AnimeStyleRendering.h"

bool SceneHierarchyWindow::Init()
{
    m_Name = "SceneHierarchy";
	return true;
}

void SceneHierarchyWindow::Update(float DeltaTime)
{
    auto& scene = AnimeStyleRendering::Inst().m_CurrentScene;
    
    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_AlwaysAutoResize;
    
    ImGui::Begin(m_Name.c_str(), &m_Open, WindowFlags);
    
    //마우스를 딴 데 누르거나 하면 Entity 선택 해제
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        m_SelectedEntity = nullptr;
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
    {
        ImGui::OpenPopup("EntityPopUp");
    }

    if (ImGui::BeginPopup("EntityPopUp"))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            if (!m_SelectedEntity)
            {
                auto entity = scene->AddEntity(true);
                entity->Name("Empty Entity");
            }
            else {
                auto entity = scene->AddEntity(false);
                entity->Name("Empty Entity");
                entity->Parent(m_SelectedEntity);
            }
        }
        if (m_SelectedEntity)
        {
            if (ImGui::MenuItem("Remove Entity"))
            {
                m_SelectedEntity->Alive(false);
                m_SelectedEntity = nullptr;
            }
        }

        ImGui::EndPopup();
    }

    if (ImGui::CollapsingHeader(scene->Name().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        scene->OnIMGUIRender();
    }
    
    ImGui::End();
}
