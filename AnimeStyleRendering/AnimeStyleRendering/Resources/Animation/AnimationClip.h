#pragma once
#include "Common/Common.h"

class Animation;
class Skeleton;

struct AnimationFrameTrans {
	math::vec4 Translate;
	math::quat Rotation;
	math::vec4 Scale;
};
class AnimationClip {
public:
	bool Load(std::string filePath);
	bool Load(aiAnimation* animationClip);

	bool SetSkeleton(std::shared_ptr<Skeleton> skeleton);

public:
	const std::vector<AnimationFrameTrans>& GetKeyFrames() const { return m_FrameTransBuffer; }
	void SetPlayScale(float scale) { m_PlayScale = scale; }
	void SetPlayTime(float time) { m_PlayTime = time; }
	void SetLoop(bool loop) { m_Loop = loop; }
	float GetPlayTime() { return m_PlayTime; }
	float GetStartTime() { return m_StartTime; }
	float GetEndTime() { return m_EndTime; }
	int GetFrameLength() { return m_FrameLength; }
	float GetFrameTime() { return m_FrameTime; }
	std::unordered_map<std::string, std::vector<KeyFrame>>& GetKeyFrameInfoMap() { return m_mapBoneKeyFrame; }

private:
	std::shared_ptr<Skeleton> m_Skeleton;
	std::vector<AnimationFrameTrans> m_FrameTransBuffer; // ComputeShader에 사용할 데이터 셋
	std::unordered_map<std::string, std::vector<KeyFrame>> m_mapBoneKeyFrame; // 원시(Primal) 애니메이션 데이터

	bool m_Loop{ false };
	bool m_End{ false };

	float m_StartTime{ 0.f };
	float m_EndTime{ 0.f };

	//단위 : Second
	float m_PlayTime{ 0.f };
	float m_PlayScale{ 1.f };

	int m_StartFrame{ 0 };
	int m_EndFrame{ 0 };

	int m_FrameLength{ 0 };
	float m_FrameTime{ 0 };
	int m_FramePerSecond{ 0 };
};