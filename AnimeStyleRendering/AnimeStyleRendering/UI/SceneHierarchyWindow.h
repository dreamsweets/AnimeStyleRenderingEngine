#pragma once
#include "imgui/Window/IMGUIWindow.h"
#include "Scene/Entity.h"

class SceneHierarchyWindow : public IMGUIWindow
{
public:
	virtual bool Init() override;
	virtual void Update(float DeltaTime) override;

public:
	void SetSelectedEntity(Entity* e) { m_SelectedEntity = e; }
	Entity* GetSelectedEntity() { return m_SelectedEntity; }

private:
	Entity* m_SelectedEntity{ nullptr };
};