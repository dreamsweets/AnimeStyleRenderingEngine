#pragma once
#include "Scene/Scene.h"
#include "Common/Time.h"
class Material;
class VertexBuffer;

//Application
class AnimeStyleRendering {
	friend class SceneHierarchyWindow;
public:
	void Init();
	void Run();
	void Destroy();
private:
	void CreateAssets();

	void Update();
	void PostUpdate();
	void Render();

private:
	Scene* m_CurrentScene;
	Material* m_TestMaterial;
	VertexBuffer* m_TestVertexBuffer;
	DECLARE_SINGLE(AnimeStyleRendering)
};