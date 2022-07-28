#include "AnimatorComponent.h"
#include "Resources/Animation/Animation.h"
#include "MeshRendererComponent.h"

void AnimatorComponent::SetAnimation(Animation* animation)
{
	if (!animation) return;

	m_Animation = animation;
}

void AnimatorComponent::AddMeshRenderer(std::vector<std::shared_ptr<MeshRendererComponent>> vecMeshComponent)
{
	m_vecAnimationMeshRenderer = vecMeshComponent;
}

void AnimatorComponent::Start()
{
	for (auto& mesh : m_vecAnimationMeshRenderer)
	{
		mesh->GetMaterial(0)->SetStructuredBuffer<math::mat4x4>("g_SkinningBoneMatrixArray", m_Animation->GetBoneMatrixResource(), m_Animation->GetBoneCount());
	}
}

void AnimatorComponent::Update()
{
	if (!m_Animation) return;
	m_Animation->UpdateBoneTransform(Time::GetDeltaTime());
	auto& matrices = m_Animation->GetBoneMatrixData();

	for (auto& mesh : m_vecAnimationMeshRenderer)
	{
		mesh->GetMaterial(0)->SetStructuredBuffer<math::mat4x4>("g_SkinningBoneMatrixArray", m_Animation->GetBoneMatrixResource(), m_Animation->GetBoneCount());
	}

	for (int i = 0; i < m_vecBoneEntity.size(); ++i)
	{
		math::vec3 pos = GetPostion(matrices[i]);
		math::vec3 rot = GetRotationEuler(matrices[i]);
		math::vec3 scale = GetScale(matrices[i]);
		m_vecBoneEntity[i]->GetTransform()->Position(pos);
		m_vecBoneEntity[i]->GetTransform()->Rotation(rot);
		m_vecBoneEntity[i]->GetTransform()->Scale(scale);
	}
}

void AnimatorComponent::Render()
{
}
