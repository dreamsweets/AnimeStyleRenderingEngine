#include "PrimitiveComponent.h"
#include "Scene/Entity.h"
#include "Render/RenderManager.h"

void PrimitiveComponent::Init()
{
	m_TransformComponent = m_Entity->GetComponent<TransformComponent>();
	RenderManager::Inst().Register(this);
}

void PrimitiveComponent::Start()
{
}

void PrimitiveComponent::Update()
{
}

void PrimitiveComponent::Destroy()
{
	RenderManager::Inst().Unregister(this);
}