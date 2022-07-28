#include "CameraMoveScript.h"
#include "Scene/Entity.h"
#include "Components/Transform/TransformComponent.h"
#include "Components/Camera/CameraComponent.h"
#include "Common/Input.h"
#include "Common/Time.h"
#include "Components/ComponentManager.h"

RegisterComponentData(CameraMoveScript)

void CameraMoveScript::Start()
{
}

void CameraMoveScript::Update()
{
	if (Input::Inst().MouseButton(Input::Mouse::MButton))
	{
		auto transform = m_Entity->GetTransform();
		auto pos = transform->Position();
		auto rot = transform->Rotation();
		if (Input::Inst().KeyButton(Input::Key::W))
		{
			pos += transform->Front() * m_MoveMultiplier * Time::GetDeltaTime();
		}
		if (Input::Inst().KeyButton(Input::Key::A))
		{
			pos += transform->Left() * m_MoveMultiplier * Time::GetDeltaTime();
		}
		if (Input::Inst().KeyButton(Input::Key::S))
		{
			pos += transform->Back() * m_MoveMultiplier * Time::GetDeltaTime();
		}
		if (Input::Inst().KeyButton(Input::Key::D))
		{
			pos += transform->Right() * m_MoveMultiplier * Time::GetDeltaTime();
		}
		transform->Position(pos);

		auto delta = Input::Inst().MouseDelta();
		if (delta.x != 0)
		{
			rot.y += delta.x * m_RotateMultiplier * Time::GetDeltaTime();
		}
		if (delta.y != 0)
		{
			rot.x += delta.y * m_RotateMultiplier * Time::GetDeltaTime();
		}
		transform->Rotation(rot);
	}

}
