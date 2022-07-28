#include "AnimationClip.h"
#include "Skeleton.h"
#include "Common/AssimpHelper.h"

bool AnimationClip::Load(std::string filePath)
{
	if (!Path::IsExist(filePath)) return false;

	Assimp::Importer importer;
	const aiScene* pScene = importer.ReadFile(filePath,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_ConvertToLeftHanded |
		aiProcess_LimitBoneWeights
		//| aiProcess_FlipUVs
	);

	if (!pScene->HasAnimations()) return false;

	return Load(pScene->mAnimations[0]);
}

bool AnimationClip::Load(aiAnimation* animationClip)
{
	if (!animationClip) return false;

	m_FramePerSecond = animationClip->mTicksPerSecond;
	m_PlayTime = animationClip->mDuration / 1000; // Duration이 ms로 들어오므로 Second로 단위 변환
	m_FrameLength = m_PlayTime * animationClip->mTicksPerSecond + 1;

	m_EndFrame = m_FrameLength - 1;
	m_EndTime = m_PlayTime;
	m_FrameTime = m_PlayTime / m_FrameLength;

	for (int j = 0; j < animationClip->mNumChannels; ++j)
	{
		auto& boneAnimationKeys = animationClip->mChannels[j];
		m_mapBoneKeyFrame.insert({ boneAnimationKeys->mNodeName.C_Str(), {} });
		auto& boneKeyFrame = m_mapBoneKeyFrame[boneAnimationKeys->mNodeName.C_Str()];

		for (int k = 0; k < boneAnimationKeys->mNumPositionKeys; ++k)
		{
			auto& posKey = boneAnimationKeys->mPositionKeys[k];
			auto& rotKey = boneAnimationKeys->mRotationKeys[k];
			auto& scaleKey = boneAnimationKeys->mScalingKeys[k];

			double time = posKey.mTime / 1000;

			math::vec3 pos = assimpToGlmVec3(posKey.mValue);
			
			math::quat rot = assimpToGlmQuat(rotKey.mValue);
			
			math::vec3 scale = assimpToGlmVec3(scaleKey.mValue);

			boneKeyFrame.push_back({ time, pos, rot ,scale });
		}
	}

	return true;
}

bool AnimationClip::SetSkeleton(std::shared_ptr<Skeleton> skeleton)
{
	if (!skeleton) return false;

	m_Skeleton = skeleton;
	m_FrameTransBuffer.clear();
	int boneCount = m_Skeleton->GetBoneCount();

	m_FrameTransBuffer.resize(m_FrameLength * boneCount);

	for (auto& [boneName, keyFrame] : m_mapBoneKeyFrame)
	{
		auto boneIndex = m_Skeleton->GetBoneIndex(boneName);
		if (boneIndex < 0) continue;
		int currentFrame = 0;
		for (int key = 0; key < keyFrame.size(); ++key)
		{
			int criticalPoint = keyFrame[key].time * m_FramePerSecond;
			if (criticalPoint >= m_FrameLength) criticalPoint = m_FrameLength;
			for (; currentFrame < criticalPoint; ++currentFrame)
			{
				m_FrameTransBuffer[currentFrame * boneCount + boneIndex].Translate = math::vec4{ keyFrame[key].position, 1.f };
				m_FrameTransBuffer[currentFrame * boneCount + boneIndex].Rotation = keyFrame[key].rotation;
				m_FrameTransBuffer[currentFrame * boneCount + boneIndex].Scale = math::vec4{ keyFrame[key].scale, 0.f };
			}
		}
	}

	return true;
}
