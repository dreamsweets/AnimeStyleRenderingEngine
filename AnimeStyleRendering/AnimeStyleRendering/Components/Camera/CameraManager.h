#pragma once
#include "CameraComponent.h"

class CameraManager {
public:
	void RegisterComponent(CameraComponent* c);
	void UnregisterComponent(CameraComponent* c);
	
	CameraComponent* GetMainCamera();
	void SetMainCamera(CameraComponent* c);

private:
	bool IsAleadyRegistered(CameraComponent* c);

private:
	CameraComponent* m_MainCamera{nullptr};
	std::list<CameraComponent*> m_CameraComponents;

	DECLARE_SINGLE(CameraManager)
};