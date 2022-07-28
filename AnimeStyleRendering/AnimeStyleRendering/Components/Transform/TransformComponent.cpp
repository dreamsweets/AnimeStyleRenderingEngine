#include "TransformComponent.h"
#include "UI/UIHelper.h"
#include "Components/ComponentManager.h"

RegisterComponentData(TransformComponent)

void TransformComponent::Init()
{
}

void TransformComponent::Update()
{
}

void TransformComponent::OnIMGUIRender()
{
	ImGui::PushID("Transform");
	if (ImGui::TreeNodeEx(this, ImGuiTreeNodeFlags_DefaultOpen, "Transform")) {
		DrawVec3Control("Position", m_Position);
		DrawVec3Control("Rotation", m_Rotation);
		DrawVec3Control("Scale", m_Scale);
		
		ImGui::TreePop();
	}
	ImGui::PopID();
}

math::mat4x4 TransformComponent::GetFixedWorldMatrix()
{
	if (m_Entity->Parent())
	{
		return GetWorldMatrix() * m_Entity->Parent()->GetTransform()->GetFixedWorldMatrix();
	}
	return GetWorldMatrix();
}