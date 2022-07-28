#pragma once
#include "Common/Common.h"
#include "Common/AssimpHelper.h"

struct aiNode;
class Entity;
class Scene;
// Skeleton : Bone Container
class Skeleton {
	using BoneName = std::string;
	using BoneIndex = u32;

private:
	std::vector<math::mat4x4> m_OffsetMatrixBuffer;
	std::unordered_map<std::string, Bone*>	m_mapBone;
	std::vector<Bone*>						m_vecBone;
	Bone m_Bone;

public:
	void GenerateOffsetBuffer();
	void Init(aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable);

private:
	bool ReadSkeleton(Bone& bone, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable);
	Entity* CreateBoneEntityHirarchy(Scene* scene, Bone& bone);
public:
	const Bone& GetRootBone() { return m_Bone; }
	
	const u32 GetBoneCount() const { return (u32)m_mapBone.size(); }
	
	Bone* GetBone(u32 boneIndex = 0) {
		if(boneIndex < m_vecBone.size())
		return m_vecBone[boneIndex];
		return nullptr;
	}
	
	Bone* GetBone(std::string boneName) {
		auto result = m_mapBone.find(boneName);
		if (result == m_mapBone.end()) return nullptr;
		return result->second;
	}
	
	int GetBoneIndex(std::string boneName) {
		auto result = m_mapBone.find(boneName);
		if (result == m_mapBone.end()) return -1;
		return result->second->Identifier;
	}
	
	const std::vector<math::mat4x4>& GetOffsetMatrixBuffer() const { return m_OffsetMatrixBuffer; }

	Entity* CreateBoneAsEntity(Scene* scene);
};