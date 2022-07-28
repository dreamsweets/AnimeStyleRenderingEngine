#pragma once
#include "Common/Common.h"

class Entity;

class Component : public std::enable_shared_from_this<Component>, public IMGUIElement {
	friend class Entity;
public:
	Component(Entity* entity) : m_Entity{ entity } {}
	
	virtual void Init() = 0;
	virtual void Start() {}
	virtual void Update() {}
	virtual void Destroy() = 0;

	//죽은 컴포넌트인지 여부 판정
	bool IsAlive() { return m_isAlive; }
	//활성화 상태 여부 확인
	bool Active() { return m_isActive; }
	//활성화 상태 여부
	bool Active(bool value) { m_isActive = value; }

	virtual void OnIMGUIRender() override;

protected:
	bool m_isAlive{ true };
	bool m_isActive{ true };
	Entity* m_Entity{ nullptr };
};