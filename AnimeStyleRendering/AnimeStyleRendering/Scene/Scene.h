#pragma once
#include "Entity.h"

class Scene : public IMGUIElement {
	friend class AnimeStyleRendering;
public:
	Entity* AddEntity(bool AddOnRoot = true);

	virtual void Init() {}
	virtual void Start();
	virtual void Update();
	virtual void PostUpdate();

public:
	// Inherited via IMGUIRender
	virtual void OnIMGUIRender() override;

public:
	std::string Name() { return m_Name; }
	std::string Name(std::string value) { m_Name = value; }
private:
	std::string m_Name{"Default Scene"};
	std::vector<Entity*> m_vecEntity;
};