#pragma once
#include "Scene/Entity.h"

class TransformComponent : public Component
{
public:
	TransformComponent(Entity* entity) : Component{ entity } {}

	void Init() override;
	void Update() override;
	void Destroy() override {}
	
	virtual void OnIMGUIRender() override;
	
public:
	math::vec3 Position() { return m_Position; }
	math::vec3 Rotation() { return m_Rotation; }
	math::vec3 Scale() { return m_Scale; }

	void Position(math::vec3 Position) { m_Position = Position; }
	void Rotation(math::vec3 Rotation) { m_Rotation = Rotation; }
	void Scale(math::vec3 Scale) { m_Scale = Scale; }

	math::quat GetRotationQuaternion() { 
		return math::quat{math::radians(m_Rotation)}; }
	
	math::mat4x4 GetPositionMatrix() { return math::translate(m_Position); }
	math::mat4x4 GetRotationMatrix() { return math::toMat4(GetRotationQuaternion()); }
	math::mat4x4 GetScaleMatrix() { return math::scale(m_Scale); }

	math::mat4x4 GetWorldMatrix() {
		return math::mat4x4{ GetPositionMatrix()* GetRotationMatrix()* GetScaleMatrix() };
	}
	// Entity의 Parent도 고려한 Matrix
	math::mat4x4 GetFixedWorldMatrix();

	math::vec3 Up() { return math::rotate(GetRotationQuaternion(), math::up); }
	math::vec3 Right(){ return math::rotate(GetRotationQuaternion(), math::right); }
	math::vec3 Front() { return math::rotate(GetRotationQuaternion(), math::front); }
	
	math::vec3 Down() { return math::rotate(GetRotationQuaternion(), -math::up); }
	math::vec3 Left() { return math::rotate(GetRotationQuaternion(), -math::right); }
	math::vec3 Back() { return math::rotate(GetRotationQuaternion(), -math::front); }

private:
	math::vec3 m_Position{0,0,0};
	math::vec3 m_Rotation{0,0,0};
	math::vec3 m_Scale{ 1,1,1 };
};