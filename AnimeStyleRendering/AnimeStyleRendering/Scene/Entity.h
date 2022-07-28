#pragma once
#include "Components/Component.h"

class Scene;
class TransformComponent;

class Entity : public IMGUIElement{
	friend class InspectorWindow;
	friend class Scene;
public:
	std::string Name() { return m_Name; }
	void Name(std::string value) { m_Name = value; }
	
	template<typename T>
	std::shared_ptr<T> AddComponent() {
		auto size = m_vecComponent.size();
		
		auto component = new T{ this };
		component->Init();

		m_vecComponent.emplace_back(component);
		return std::shared_ptr<T>(m_vecComponent[size], reinterpret_cast<T*>(m_vecComponent[size].get()));
	}

	template<typename T>
	std::shared_ptr<T> GetComponent() {

		for (auto it = m_vecComponent.begin(); it != m_vecComponent.end(); ++it)
		{
			auto& real = *it;
			std::type_index type = typeid(*real.get());
			if (type == typeid(T))
			{
				
				return std::shared_ptr<T>(real, reinterpret_cast<T*>(real.get()));
			}
		}

		return nullptr;
	}
	template<typename T>
	std::vector<std::shared_ptr<T>> GetComponents() {

		std::vector<std::shared_ptr<T>> components;
		for (auto it = m_vecComponent.begin(); it != m_vecComponent.end(); ++it)
		{
			auto& real = *it;
			if (typeid(*(real.get())) == typeid(T))
			{
				components.emplace_back(real, reinterpret_cast<T*>(real.get()));
			}
		}

		return components;
	}

public:
	Entity* Parent() { return m_Parent; }
	void Parent(Entity* entity) {
		if (m_Parent)
		{
			auto it = std::find(m_Parent->m_vecChild.begin(), m_Parent->m_vecChild.end(), this);
			m_Parent->m_vecChild.erase(it);
			m_Parent = entity;
		}
		if (entity)
		{
			entity->m_vecChild.push_back(this);
		}
	}
	Entity* GetChild(int index = 0)
	{
		if (m_vecChild.empty()) return nullptr;
		if (m_vecChild.size() <= (size_t)index) return nullptr;
		return m_vecChild[index];
	}
	
	void BreakChild(Entity* e)
	{
		auto it = std::find(m_vecChild.begin(), m_vecChild.end(), e);
		if (it == m_vecChild.end()) return;

		e->m_Parent = nullptr;

		m_vecChild.erase(it);
	}

	u32 SetChild(Entity* e)
	{
		if (e->m_Parent)
		{
			e->m_Parent->BreakChild(e);
		}
		e->m_Parent = this;
		u32 index = (u32)m_vecChild.size();
		m_vecChild.push_back(e);
		
		return index;
	}

	TransformComponent* GetTransform() {
		return m_TransformComponent.get();
	}
	
public:
	// Inherited via IMGUIRender
	virtual void OnIMGUIRender() override;
	void Start();
	void Update();
	void PostUpdate();

public:
	//bool Active() { return m_isActive; }
	//bool Active(bool value) { m_isActive = value; /* TODO: Component에게 비활성상태 알림 */ }
	bool IsAlive() { return m_isAlive; }
	void Alive(bool value);

public:
	Entity(Scene* scene);
	~Entity();

private:
	Scene* m_Scene;
	Entity* m_Parent{ nullptr };
	std::vector<Entity*> m_vecChild;

	bool m_isActive{ true }; // 일시적으로 비활성화된 상태인지 판정
	bool m_isAlive{ true }; // 사용불가 상태인지 판정

	std::string m_Name{ "GameObject" };
	std::vector<std::shared_ptr<Component>> m_vecComponent;
	std::shared_ptr<TransformComponent> m_TransformComponent;
};