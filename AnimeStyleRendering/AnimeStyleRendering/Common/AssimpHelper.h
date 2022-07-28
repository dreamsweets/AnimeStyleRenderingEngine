#pragma once
#include "Common.h"

inline math::mat4 assimpToGlmMatrix(aiMatrix4x4 mat) {
	math::mat4 m;
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			m[x][y] = mat[y][x];
		}
	}
	return m;
}

inline math::vec3 assimpToGlmVec3(aiVector3D vec) {
	return math::vec3(vec.x, vec.y, vec.z);
}

inline math::quat assimpToGlmQuat(aiQuaternion quat) {
	math::quat q;
	q.x = quat.x;
	q.y = quat.y;
	q.z = quat.z;
	q.w = quat.w;

	return q;
}

namespace test {
struct Bone {
	int id = 0; // position of the bone in final upload array
	std::string name = "";
	glm::mat4 offset = glm::mat4(1.0f);
	std::vector<Bone> children = {};
};

struct BoneTransformTrack {
	std::vector<float> positionTimestamps = {};
	std::vector<float> rotationTimestamps = {};
	std::vector<float> scaleTimestamps = {};

	std::vector<glm::vec3> positions = {};
	std::vector<glm::quat> rotations = {};
	std::vector<glm::vec3> scales = {};
};

struct Animation {
	float duration = 0.0f;
	float ticksPerSecond = 1.0f;
	std::unordered_map<std::string, BoneTransformTrack> boneTransforms = {};
};

//bool readSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable) {
//
//	if (boneInfoTable.find(node->mName.C_Str()) != boneInfoTable.end()) { // if node is actually a bone
//		boneOutput.name = node->mName.C_Str();
//		boneOutput.id = boneInfoTable[boneOutput.name].first;
//		boneOutput.offset = boneInfoTable[boneOutput.name].second;
//
//		for (int i = 0; i < node->mNumChildren; i++) {
//			Bone child;
//			readSkeleton(child, node->mChildren[i], boneInfoTable);
//			boneOutput.children.push_back(child);
//		}
//		return true;
//	}
//	else { // find bones in children
//		for (int i = 0; i < node->mNumChildren; i++) {
//			if (readSkeleton(boneOutput, node->mChildren[i], boneInfoTable)) {
//				return true;
//			}
//
//		}
//	}
//	return false;
//}
} // namespace test