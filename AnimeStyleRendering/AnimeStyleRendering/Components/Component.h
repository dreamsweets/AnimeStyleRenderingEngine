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

	//���� ������Ʈ���� ���� ����
	bool IsAlive() { return m_isAlive; }
	//Ȱ��ȭ ���� ���� Ȯ��
	bool Active() { return m_isActive; }
	//Ȱ��ȭ ���� ����
	bool Active(bool value) { m_isActive = value; }

	virtual void OnIMGUIRender() override;

protected:
	bool m_isAlive{ true };
	bool m_isActive{ true };
	Entity* m_Entity{ nullptr };
};