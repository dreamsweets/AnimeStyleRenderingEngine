#include "Animation.h"
#include "AnimationClip.h"
#include "Resources/ResourceManager.h"

bool Animation::Init()
{
	m_BoneTransformComputeShader.LoadByPath("Shaders/ComputeShader/AnimationComputeShader.fx");
	return true;
}

void Animation::SetSkeleton(std::shared_ptr<Skeleton> skeleton)
{
	m_Skeleton = skeleton;
	m_vecBoneMatrix.resize(m_Skeleton->GetBoneCount(), math::identity<math::mat4x4>());
	m_BoneTransformComputeShader.SetAsRWBuffer("g_BoneMatrixArray", m_Skeleton->GetOffsetMatrixBuffer());
}

bool Animation::BoneNameExist(std::string name)
{
	auto result = m_mapBoneKey.find(name);
	if (result == m_mapBoneKey.end()) return false;
	return true;
}

bool Animation::AddAnimationClip(std::string clipName, AnimationClip* clip)
{
	if (clip == nullptr) return false;

	auto iter = m_mapAnimationClip.find(clipName);
	
	if (iter == m_mapAnimationClip.end())
	{
		//없는 경우 새로 공간 생성후 넣기
		m_mapAnimationClip.insert({ clipName, clip });

		if (!m_CurrentSequence)
			m_CurrentSequence = clip;
		return true;
	}

	//이미 있는 경우 덮어쓰기
	m_mapAnimationClip[clipName] = clip;
	
	if (!m_CurrentSequence)
	{
		m_CurrentSequence = clip;
	}
	
	return true;
}

void Animation::UpdateBoneTransform(float DeltaTime)
{
	if (!m_CurrentSequence) return;

	m_PlayTime += DeltaTime;
	
	bool Change = false;
	bool ChangeEnd = false;
	if (m_NextSequence) {
		Change = true;
		m_ChangeTimeAcc += DeltaTime;

		if (m_ChangeTimeAcc >= m_ChangeTime)
		{
			m_ChangeTimeAcc = m_ChangeTime;
			ChangeEnd = true;
		}
	}
	else {
		m_SequenceProgress = m_PlayTime / m_CurrentSequence->GetPlayTime();
		while (m_PlayTime >= m_CurrentSequence->GetPlayTime())
		{
			m_PlayTime -= m_CurrentSequence->GetPlayTime();
			m_End = true;
		}

		m_UpdateInfo.ChangeAnimation = false;
		m_UpdateInfo.ChangeRatio = 0.f;
		m_UpdateInfo.ChangeFrameCount = 0;

		float AnimationTime = m_PlayTime + m_CurrentSequence->GetStartTime();

		int StartFrame = 0;
		int EndFrame = m_CurrentSequence->GetFrameLength() - 1;

		int FrameIndex = (int)(AnimationTime / m_CurrentSequence->GetFrameTime());
		int NextFrameIndex = FrameIndex + 1;

		if (NextFrameIndex > EndFrame) NextFrameIndex = StartFrame;

		float Ratio = (AnimationTime - m_CurrentSequence->GetFrameTime() * FrameIndex) / m_CurrentSequence->GetFrameTime();
		m_UpdateInfo.FrameCount = m_CurrentSequence->GetFrameLength();
		m_UpdateInfo.currentFrame = FrameIndex;
		m_UpdateInfo.nextFrame = NextFrameIndex;
		m_UpdateInfo.animationRatio = Ratio;
		m_UpdateInfo.boneCount = m_Skeleton->GetBoneCount();
		m_UpdateInfo.AnimationRowIndex = 0;


		if (m_End)
		{
			// Animation End 에 대한 Handling
		}
	}

	if (Change) {
		m_UpdateInfo.ChangeRatio = m_ChangeTimeAcc / m_ChangeTime;
		m_UpdateInfo.ChangeAnimation = true;
		m_UpdateInfo.ChangeFrameCount = m_NextSequence->GetFrameLength();


	}

	m_BoneTransformComputeShader.CBufferSetValue("AnimationUpdateCBuffer", m_UpdateInfo);
	SetFrameBuffers();
	m_BoneTransformComputeShader.Dispatch(m_Skeleton->GetBoneCount());

	//Test Code -> 제대로 실행되었는지 확인

	{
		m_BoneTransformComputeShader.GetResourceFromResult((void*)m_vecBoneMatrix.data());
	}
}

void Animation::SetFrameBuffers()
{
	if (m_Skeleton)
	{
		m_BoneTransformComputeShader.SetAsStructuredBuffer("g_OffsetArray", m_Skeleton->GetOffsetMatrixBuffer());
	}
	if (m_CurrentSequence)
	{
		m_BoneTransformComputeShader.SetAsStructuredBuffer("g_FrameTransArray", m_CurrentSequence->GetKeyFrames());
	}
	if (m_NextSequence)
	{
		m_BoneTransformComputeShader.SetAsStructuredBuffer("g_ChangeFrameTransArray", m_NextSequence->GetKeyFrames());
	}
}
