#pragma once
#include "PrimitiveComponent.h"
#include "Resources/Mesh/Model.h"

class MeshRendererComponent : public PrimitiveComponent {
	friend class RenderManager;

public:
	MeshRendererComponent(Entity* entity) : PrimitiveComponent{ entity } {}

	virtual void OnIMGUIRender() override;

public:
	void SetMesh(Mesh* mesh) { m_Mesh = mesh; }
	Mesh* GetMesh() { return m_Mesh; }

	virtual void Render() override;

private:
	Mesh* m_Mesh{nullptr};
};