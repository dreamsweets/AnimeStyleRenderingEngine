#pragma once
#include "Components/Component.h"

class CameraComponent;

class CameraMoveScript : public Component {

public:
	CameraMoveScript(Entity* entity) : Component{ entity } {}

	virtual void Init() {}
	virtual void Destroy() {}

	virtual void Start() override;
	virtual void Update() override;

private:
	float m_MoveMultiplier{2.f};
	float m_RotateMultiplier{90.f};

};