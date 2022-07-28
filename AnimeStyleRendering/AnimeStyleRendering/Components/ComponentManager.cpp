#include "ComponentManager.h"

IMPLEMENT_SINGLE(ComponentManager)

u8 ComponentManager::RegisterComponentInfo(std::string ComponentName, std::type_index type, ComponentCreator Creator)
{
	u32 index = m_vecComponentCreator.size();
	m_vecComponentCreator.push_back(Creator);
	m_mapComponentIndexByString[ComponentName] = index;
	m_mapComponentIndexByType[type] = index;
	m_vecComponentName.push_back(ComponentName);
	return true;
}

ComponentManager::ComponentCreator ComponentManager::GetCreator(std::string ComponentName)
{
	auto iter = m_mapComponentIndexByString.find(ComponentName);
	if (iter == m_mapComponentIndexByString.end()) return nullptr;
	
	return m_vecComponentCreator[iter->second];
}

ComponentManager::ComponentCreator ComponentManager::GetCreator(std::type_index Type)
{
	auto iter = m_mapComponentIndexByType.find(Type);
	if (iter == m_mapComponentIndexByType.end()) return nullptr;

	return m_vecComponentCreator[iter->second];
}
