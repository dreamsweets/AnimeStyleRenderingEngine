#pragma once
#include "imgui/Window/IMGUIWindow.h"
#include "SceneHierarchyWindow.h"

class Entity;

class InspectorWindow : public IMGUIWindow
{
public:
	virtual bool Init() override;
	virtual void Update(float DeltaTime) override;

private:
	Entity* m_Context;
	SceneHierarchyWindow* m_HierarchyWindow;
};