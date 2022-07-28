#pragma once
#include "DX12/ComputeShader.h"
#include "Skeleton.h"

class AnimationClip;

struct AnimationUpdateInfo {
	int boneCount{0};
	int currentFrame{ 0 };
	int nextFrame{ 0 };
	float animationRatio{ 0 };
	int FrameCount{ 0 };
	int AnimationRowIndex{ 0 };
	bool ChangeAnimation{ false };
	float ChangeRatio{ 0 };
	int ChangeFrameCount{ 0 };
	math::vec3 empty;
};

class Animation {
public:
	bool Init();
	void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
	Skeleton* GetSkeleton() { return m_Skeleton.get(); }
	bool BoneNameExist(std::string name);
	bool AddAnimationClip(std::string clipName, AnimationClip* clip);
	ComPtr<ID3D12Resource> GetBoneMatrixResource() { return m_BoneTransformComputeShader.GetResultBuffer(); }
	u32 GetBoneCount() { return m_Skeleton ? m_Skeleton->GetBoneCount() : 0; }
	std::vector<math::mat4x4>& GetBoneMatrixData() { return m_vecBoneMatrix; }

public:
	void UpdateBoneTransform(float DeltaTime);

private:
	void SetFrameBuffers();

private:
	AnimationUpdateInfo m_UpdateInfo;
	std::shared_ptr<Skeleton> m_Skeleton{nullptr};
	std::vector<math::mat4x4> m_vecBoneMatrix;
	ComputeShader m_BoneTransformComputeShader;
	
	AnimationClip* m_CurrentSequence{nullptr};
	AnimationClip* m_NextSequence{nullptr};
	
	std::unordered_map<std::string, AnimationClip*> m_mapAnimationClip;
	std::vector<std::vector<KeyFrame>> m_vecBoneFrameTransforms;
	std::unordered_map<std::string, u32> m_mapBoneKey;

	float m_PlayTime = 0.f;
	float m_SequenceProgress = 0.f;

	float           m_ChangeTimeAcc;
	float           m_ChangeTime;

	bool m_End{false};
};