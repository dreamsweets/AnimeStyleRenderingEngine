#include "CameraComponent.h"
#include "Components/Transform/TransformComponent.h"
#include "CameraManager.h"
#include "DX12/Gfx.h"
#include "Components/ComponentManager.h"

RegisterComponentData(CameraComponent)

void CameraComponent::Init()
{
	m_TransformComponent = m_Entity->GetComponent<TransformComponent>();
	CameraManager::Inst().RegisterComponent(this);
}

void CameraComponent::Destroy()
{
	CameraManager::Inst().UnregisterComponent(this);
}

void CameraComponent::OnIMGUIRender()
{
	if (ImGui::TreeNodeEx(this, ImGuiTreeNodeFlags_DefaultOpen, "Camera")) {
		const char* projecttionTypeStrings[] = {"Orthographic" , "Perspective"};
		const char* currentTypeString = projecttionTypeStrings[m_IsPerspective];
		if (ImGui::BeginCombo("Projection", currentTypeString))
		{
			for (int i = 0; i < 2; ++i)
			{
				bool IsSelected = currentTypeString == projecttionTypeStrings[i];
				if (ImGui::Selectable(projecttionTypeStrings[i], IsSelected))
				{
					currentTypeString = projecttionTypeStrings[i];
					SetPerspective((bool)i);
				}
				if (IsSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (IsPerspective())
		{
			float fov = m_FOV;
			if (ImGui::DragFloat("Field Of View", &fov))
			{
				m_FOV = fov;
			}
			float nearZ = m_NearZ;
			if (ImGui::DragFloat("Near", &nearZ))
			{
				m_NearZ = nearZ;
			}
			float FarZ = m_FarZ;
			if (ImGui::DragFloat("Far", &FarZ))
			{
				m_FarZ = FarZ;
			}
		}

		ImGui::TreePop();
	}
}

void CameraComponent::SetProjection(float fov, float aspect, float zNear, float zFar)
{
	m_FOV = fov;
	m_AspectRatio = aspect;
	m_NearZ = zNear;
	m_FarZ = zFar;

	m_ProjectionDirty = true;
}

void CameraComponent::SetPerspective(bool value)
{
	if (m_IsPerspective != value)
	{
		m_IsPerspective = value;
		m_ProjectionDirty = true;
	}
}

math::vec3 CameraComponent::Position()
{
	return m_TransformComponent->Position();
}

void CameraComponent::UpdateMatrices()
{
	//View Matrix는 매 프레임마다 업데이트한다.
	math::vec3 pos = m_TransformComponent->Position();
	m_LookAt = pos + m_TransformComponent->Front();
	math::vec3 up = m_TransformComponent->Up();

	m_ViewMatrix = math::lookAtLH(pos, m_LookAt, up);
	m_InverseViewMatrix = math::inverse(m_ViewMatrix);

	//Projection은 변경사항이 존재할 경우에만 업데이트한다.
	if (m_ProjectionDirty)
	{
		if(m_IsPerspective)
			m_ProjectionMatrix = math::perspectiveLH(math::radians(m_FOV), m_AspectRatio, m_NearZ, m_FarZ);
		else {
			m_ProjectionMatrix = math::orthoLH(
				0.f,
				(f32)Gfx::GetWidth(),
				0.f,
				(f32)Gfx::GetHeight(),
				m_NearZ,
				m_FarZ
			);
		}
		m_InverseProjectionMatrix = math::inverse(m_ProjectionMatrix);

		m_ProjectionDirty = false;
	}
}

