#pragma once
#include "Scene/Entity.h"

class ComponentManager {
public:
	using ComponentCreator = std::shared_ptr<Component>(*)(Entity* entity);

private:
	std::vector<ComponentCreator>				m_vecComponentCreator;
	std::unordered_map<std::string, u32>		m_mapComponentIndexByString;
	std::unordered_map<std::type_index, u32>	m_mapComponentIndexByType;
	std::vector<std::string>					m_vecComponentName;

public:
	u8 RegisterComponentInfo(std::string ComponentName, std::type_index type, ComponentCreator Creator);
	const std::vector<std::string>& GetComponentNames() { return m_vecComponentName; }
	
	ComponentCreator GetCreator(std::string ComponentName);
	ComponentCreator GetCreator(std::type_index Type);

	DECLARE_SINGLE(ComponentManager)
};

template<typename T>
std::shared_ptr<Component> CreateComponent(Entity* entity)
{
	return entity->AddComponent<T>();
}

#define RegisterComponentData(Component)																														\
namespace{																																						\
u8 g_Reg_Registered_##Component = ComponentManager::Inst().RegisterComponentInfo(#Component, typeid(Component), CreateComponent<Component>);					\
}