#include "Scene.h"
#include "Components/Transform/TransformComponent.h"

Entity* Scene::AddEntity(bool AddOnRoot)
{
	Entity* e = new Entity(this);
	
	if (AddOnRoot) m_vecEntity.push_back(e);

	return e;
}

void Scene::Start()
{
	for (auto& entt : m_vecEntity)
	{
		entt->Start();
	}
}

void Scene::Update()
{
	for (auto& entt : m_vecEntity)
	{
		entt->Update();
	}
}

void Scene::PostUpdate()
{
	for (auto& entt : m_vecEntity)
	{
		entt->PostUpdate();
	}

	{
		auto start = m_vecEntity.begin();
		auto end = m_vecEntity.end();

		for (; start != end; )
		{
			if (!(*start)->m_isAlive)
			{
				delete (*start);
				start = m_vecEntity.erase(start);
				end = m_vecEntity.end();
			}
			else {
				++start;
			}
		}
	}
}

void Scene::OnIMGUIRender()
{
	for (auto& entt : m_vecEntity)
	{
		entt->OnIMGUIRender();
	}
}