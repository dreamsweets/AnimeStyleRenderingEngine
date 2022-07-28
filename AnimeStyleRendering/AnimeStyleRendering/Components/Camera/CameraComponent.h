#pragma once
#include "Components/Component.h"
#include "Camera.h"

class TransformComponent;

class CameraComponent : public Component {
public:
	CameraComponent(Entity* entity) : Component{ entity } {}

	void Init() override;
	void Destroy() override;

	virtual void OnIMGUIRender() override;

public:
	math::vec3 LookAt() { return m_LookAt; }

	f32 Near() { return m_NearZ; }
	f32 Far() { return m_FarZ; }
	f32 AspectRatio() { return m_AspectRatio; }
	f32 Fov() { return m_FOV; }

	bool IsPerspective() { return m_IsPerspective; }

	math::mat4x4 ViewMatrix() { return m_ViewMatrix; }
	math::mat4x4 InverseViewMatrix() { return m_InverseViewMatrix; }

	math::mat4x4 ProjMatrix() { return m_ProjectionMatrix; }
	math::mat4x4 InverseProjMatrix() { return m_InverseProjectionMatrix; }

	void SetProjection(float fov, float aspect, float zNear, float zFar);
	void SetPerspective(bool value);

	math::vec3 Position();
	void UpdateMatrices();

private:
	std::shared_ptr<TransformComponent> m_TransformComponent{ nullptr };

	f32 m_FOV{ 90.f };
	f32 m_NearZ{ 0.1f }, m_FarZ{ 1000.f };
	f32 m_AspectRatio{ 16.f / 9.f };
	math::vec3 m_LookAt{math::front};
	bool m_IsPerspective{ true };

	math::mat4x4 m_ViewMatrix, m_InverseViewMatrix;
	math::mat4x4 m_ProjectionMatrix, m_InverseProjectionMatrix;

	mutable bool m_ProjectionDirty{ true };
};