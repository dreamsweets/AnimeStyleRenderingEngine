#include "LightComponent.h"
#include "Components/Transform/TransformComponent.h"
#include "Render/RenderManager.h"
#include "UI/UIHelper.h"
#include "Components/ComponentManager.h"

RegisterComponentData(DirectionalLightComponent)
RegisterComponentData(PointLightComponent)

void LightComponent::Init()
{
	m_TransformComponent = m_Entity->GetComponent<TransformComponent>();
}

void LightComponent::Update()
{
	RenderManager::Inst().Register(this);
}

void DirectionalLightComponent::Init()
{
	LightComponent::Init();

	m_Light.lightType = LightType::Directional;
}

void DirectionalLightComponent::Update()
{
	LightComponent::Update();
	m_Light.direction = m_Entity->GetTransform()->Front();
}

void DirectionalLightComponent::OnIMGUIRender()
{
	ImGui::PushID("DirectionalLight");
	if (ImGui::TreeNodeEx(this, ImGuiTreeNodeFlags_DefaultOpen, "Directional Light")) {
		ImGui::Checkbox("CastShadow", &m_CastShadow);
		DrawVec3Control("Direction", m_Light.direction);
		static float color[3];
		math::vec3& colorVec3 = *reinterpret_cast<math::vec3*>(color);
		colorVec3 = m_Light.color;
		ImGui::Text("Color");
		ImGui::SameLine();
		if (ImGui::ColorEdit3("##Color", color))
		{
			m_Light.color = colorVec3;
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

LightData& DirectionalLightComponent::GetData()
{
	return m_Light;
}

void PointLightComponent::Init()
{
	LightComponent::Init();
	m_Light.lightType = LightType::Point;
}

void PointLightComponent::Update()
{
	LightComponent::Update();
}

void PointLightComponent::OnIMGUIRender()
{
	ImGui::PushID("PointLight");
	if (ImGui::TreeNodeEx(this, ImGuiTreeNodeFlags_DefaultOpen, "Point Light")) {
		ImGui::Checkbox("CastShadow", &m_CastShadow);
		ImGui::DragFloat("##Range", &m_Light.range);
		static float color[3];
		math::vec3& colorVec3 = *reinterpret_cast<math::vec3*>(color);
		colorVec3 = m_Light.color;
		ImGui::Text("Color");
		ImGui::SameLine();
		if (ImGui::ColorEdit3("##Color", color))
		{
			m_Light.color = colorVec3;
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

LightData& PointLightComponent::GetData()
{
	m_Light.position = m_TransformComponent->Position();
	return m_Light;
}

void SpotLightComponent::Init()
{
	LightComponent::Init();
	m_Light.lightType = LightType::Spot;
}

LightData& SpotLightComponent::GetData()
{
	m_Light.position = m_TransformComponent->Position();
	return m_Light;
}

math::vec3 SpotLightComponent::GetDirection()
{
	return m_Light.direction;
}

void SpotLightComponent::SetDirection(math::vec3 value)
{
	m_Light.direction = value;
}

float SpotLightComponent::GetSpotAngle()
{
	return m_Light.spotAngle;
}

void SpotLightComponent::SetSpotAngle(float value)
{
	m_Light.spotAngle = value;
}
