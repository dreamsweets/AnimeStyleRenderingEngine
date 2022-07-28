#pragma once
#include "Components/Component.h"
#include "DX12/DX12Common.h"

class TransformComponent;

class LightComponent : public Component {
public:
	LightComponent(Entity* entity) : Component{ entity } {}

	void Init() override;
	void Update() override;
	void Destroy() override {}

public:
	LightType GetLightype() {return m_Light.lightType;}
	
	virtual LightData& GetData() = 0;

	math::vec3 GetColor() { return m_Light.color; }
	void SetColor(math::vec3 value) { m_Light.color = value; }
	bool GetCastShadow() { return m_CastShadow; }
	void SetCastShadow(bool value) { m_CastShadow = value; }

protected:
	std::shared_ptr<TransformComponent> m_TransformComponent{ nullptr };
	LightData m_Light;
	bool m_CastShadow = false;
};

class DirectionalLightComponent : public LightComponent{
public:
	DirectionalLightComponent(Entity* entity) : LightComponent{ entity } {}
	void Init() override;
	void Update() override;
	void OnIMGUIRender() override;

public:
	LightData& GetData() override;
	math::vec3 GetDirection() { return m_Light.direction; }
};

class PointLightComponent : public LightComponent {
public:
	PointLightComponent(Entity* entity) : LightComponent{ entity } {}
	void Init() override;
	void Update() override;
	void OnIMGUIRender() override;

public:
	LightData& GetData() override;

	float GetRange() { return m_Light.range; }
	void SetRange(float value) { m_Light.range = value; }
};

class SpotLightComponent : public LightComponent {
public:
	SpotLightComponent(Entity* entity) : LightComponent{ entity } {}
	void Init() override;
	void OnIMGUIRender() override;

public:
	LightData& GetData() override;

	math::vec3 GetDirection();
	void SetDirection(math::vec3 value);

	float GetSpotAngle();
	void SetSpotAngle(float value);

	float GetRange() { return m_Light.range; }
	void SetRange(float value) { m_Light.range = value; }
};