#pragma once
#include "Scene.h"

class SceneManager {
public:
	void SetScene(Scene* scene)
	{
		m_pScene = scene;
	}

private:
	Scene* m_pScene;
};