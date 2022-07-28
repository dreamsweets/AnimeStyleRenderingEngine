#pragma once
#include "PrimitiveComponent.h"

class MeshRendererComponent;
class Animation;

class AnimatorComponent : public PrimitiveComponent {

private:
	std::vector<std::shared_ptr<MeshRendererComponent>> m_vecAnimationMeshRenderer;
	Animation* m_Animation{nullptr};
	std::vector<Entity*> m_vecBoneEntity;

public:
	AnimatorComponent(Entity* entity) : PrimitiveComponent{ entity } {}
	void SetBoneEntityData(std::vector<Entity*>& entities) { m_vecBoneEntity = entities; }
	void SetAnimation(Animation* animation);
	Animation* GetAnimation() { return m_Animation; }
	void AddMeshRenderer(std::vector<std::shared_ptr<MeshRendererComponent>> vecMeshComponent);

public:
	virtual void Start();
	virtual void Update();

	virtual void Render() override;


};