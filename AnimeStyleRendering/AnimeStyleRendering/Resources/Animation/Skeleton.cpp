#include "Skeleton.h"
#include "Scene/Scene.h"

void Skeleton::GenerateOffsetBuffer()
{
	m_OffsetMatrixBuffer.clear();
	
	m_OffsetMatrixBuffer.resize(m_vecBone.size(), math::identity<math::mat4x4>());

	for (auto i = 0; i < m_vecBone.size(); ++i)
	{
		m_OffsetMatrixBuffer[i] = m_vecBone[i]->OffsetMatrix;
	}
}

void Skeleton::Init(aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable)
{
	if (boneInfoTable.empty()) return;
	m_vecBone.resize(boneInfoTable.size());
	ReadSkeleton(m_Bone, node, boneInfoTable);

	std::queue<Bone*> bones;
	bones.push(&m_Bone);

	while (!bones.empty())
	{
		auto pBone = bones.front();
		m_vecBone[pBone->Identifier] = pBone;
		m_mapBone[pBone->Name] = pBone;
		for (auto& bone : pBone->children)
		{
			bones.push(&bone);
		}
		bones.pop();
	}

	int a = 3;
}

bool Skeleton::ReadSkeleton(Bone& bone, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable)
{
	std::string nodeName = node->mName.C_Str();
	if (boneInfoTable.find(nodeName) != boneInfoTable.end()) { // if node is actually a bone
		bone.Name = nodeName;
		bone.Identifier = boneInfoTable[bone.Name].first;
		bone.OffsetMatrix = boneInfoTable[bone.Name].second;
		
		for (int i = 0; i < node->mNumChildren; i++) {
			Bone child;
			if (ReadSkeleton(child, node->mChildren[i], boneInfoTable))
			{
				bone.children.push_back(child);
			}
		}
		return true;
	}
	else { // find bones in children
		for (int i = 0; i < node->mNumChildren; i++) {
			if (ReadSkeleton(bone, node->mChildren[i], boneInfoTable)) {
				return true;
			}
		}
	}
	return false;
}

Entity* Skeleton::CreateBoneEntityHirarchy(Scene* scene, Bone& bone)
{
	auto entity = scene->AddEntity(false);
	entity->Name(bone.Name);

	for (auto& boneChild : bone.children)
	{
		auto childEntity = CreateBoneEntityHirarchy(scene, boneChild);
		entity->SetChild(childEntity);
	}
	return entity;
}

Entity* Skeleton::CreateBoneAsEntity(Scene* scene)
{
	return CreateBoneEntityHirarchy(scene, m_Bone);
}

//bool Skeleton::ReadSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable)
//{
//	if (boneInfoTable.find(node->mName.C_Str()) != boneInfoTable.end()) { // if node is actually a bone
//		boneOutput.Name = node->mName.C_Str();
//		boneOutput.Identifier = boneInfoTable[boneOutput.Name].first;
//		boneOutput.OffsetMatrix = boneInfoTable[boneOutput.Name].second;
//
//		for (int i = 0; i < node->mNumChildren; i++) {
//			Bone child;
//			ReadSkeleton(child, node->mChildren[i], boneInfoTable);
//			boneOutput.children.push_back(child);
//		}
//		return true;
//	}
//	else { // find bones in children
//		for (int i = 0; i < node->mNumChildren; i++) {
//			if (ReadSkeleton(boneOutput, node->mChildren[i], boneInfoTable)) {
//				return true;
//			}
//		}
//	}
//
//	return false;
//}
