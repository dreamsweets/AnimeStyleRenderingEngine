#include "CameraManager.h"

IMPLEMENT_SINGLE(CameraManager)

void CameraManager::RegisterComponent(CameraComponent* c)
{
	if (IsAleadyRegistered(c)) return;
	m_CameraComponents.push_back(c);
}

void CameraManager::UnregisterComponent(CameraComponent* c)
{
	auto iter = std::find(m_CameraComponents.begin(), m_CameraComponents.end(), c);
	if(iter != m_CameraComponents.end()) m_CameraComponents.erase(iter);
}

CameraComponent* CameraManager::GetMainCamera()
{
	if (m_MainCamera)
		return m_MainCamera;

	if (!m_CameraComponents.empty())
	{
		m_MainCamera = *m_CameraComponents.begin();
		return m_MainCamera;
	}

	return nullptr;
}

void CameraManager::SetMainCamera(CameraComponent* c)
{
	m_MainCamera = c;
}

bool CameraManager::IsAleadyRegistered(CameraComponent* c)
{
	auto iter = std::find(m_CameraComponents.begin(), m_CameraComponents.end(), c);

	return iter != m_CameraComponents.end();
}