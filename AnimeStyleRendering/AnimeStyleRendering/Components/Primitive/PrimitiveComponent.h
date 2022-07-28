#pragma once
#include "Components/Transform/TransformComponent.h"
class Material;

class PrimitiveComponent : public Component{
	friend class RenderManager;
public:
	PrimitiveComponent(Entity* entity) : Component{ entity } {}
	
	void Init() override;
	void Start() override;
	void Update() override;
	void Destroy() override;
	u32 GetID() { return m_ID; }
	void SetID(u32 value) { m_ID = value; }

public:
	void SetMaterial(Material* material) { m_vecMaterial.push_back(material); }
	Material* GetMaterial(int index) { return index >= m_vecMaterial.size() ? nullptr : m_vecMaterial[index]; }
	TransformComponent* GetTransform() { return m_TransformComponent.get(); }
	
	virtual void Render() = 0;

protected:
	std::vector<Material*> m_vecMaterial;
	u32 m_ID = (u32)-1;

private:
	std::shared_ptr<TransformComponent> m_TransformComponent{ nullptr };

};