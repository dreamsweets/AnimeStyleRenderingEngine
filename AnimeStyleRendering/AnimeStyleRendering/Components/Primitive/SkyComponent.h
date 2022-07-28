#pragma once
#include "PrimitiveComponent.h"
#include "Resources/Mesh/Model.h"

class SkyComponent : public PrimitiveComponent {
	friend class RenderManager;

public:
	SkyComponent(Entity* entity) : PrimitiveComponent{ entity } {}
	virtual void Init() override;
	virtual void Update() override;
	//virtual void OnIMGUIRender() override;
	virtual void Render() override;

private:
	Mesh* m_Mesh{ nullptr };
};