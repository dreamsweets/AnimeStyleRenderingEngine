#pragma once
#include "DX12/DX12Common.h"
#include "Resources/Animation/Skeleton.h"

class VertexBuffer;
class IndexBuffer;
class Material;
class Model;

struct Face {
	u32 index[3];
};

class Mesh {
	friend class Model;
	friend class RenderManager;
public:
	Mesh() = default;
	virtual ~Mesh();

	void CreateUserDefined(std::vector<Vertex_SkeletalMesh> vertices);
	void CreateUserDefined(std::vector<Vertex_SkeletalMesh> vertices, std::vector<u32> indices);
	void CreateUserDefined(std::vector<math::vec3> vertices, std::vector<u32> indices);

	//내부 데이터가 있는 상태에서 내부 데이터를 이용해 Vertex_Mesh를 생성하는 함수
	void CreateVertices(u32 UVChannel = 0, u32 ColorChannel = 0);
	
	//VertexBuffer와 IndexBuffer 생성, RT가속구조체 생성
	void CreateBuffers();
	void CreateBuffersByPositions();
	//VertexBuffer와 IndexBuffer를 CmdList에 바인딩
	void Render();
	
	bool IsSkeletalMesh() { return m_Skeleton != nullptr; }

public:	
	void SetName(std::string value) { m_Name = value; }
	std::string GetName() { return m_Name; }
	Model* GetModel() { return m_pModel; }
	VertexBuffer* GetVertexBuffer() { return m_VertexBuffer; }
	IndexBuffer* GetIndexBuffer() { return m_IndexBuffer; }
	u32 GetVertexCount() { return (u32)m_Vertices.size(); }
	u32 GetIndexCount() { return (u32)m_Indices.size(); }
	std::vector<math::vec3> GetPositions() { return m_Positions; }
	std::vector<math::vec4>	GetWeights() { return m_BoneWeights; }
private:
	std::string m_Name{"Undefined Mesh"};
	unsigned int m_MaterialIndex{ 0 };

	//내부 데이터
	std::vector<std::vector<math::vec2>> m_vecUVChannel;
	std::vector<std::vector<math::vec4>> m_vecColorChannel;
	std::vector<math::vec3> m_Positions;
	std::vector<math::vec3> m_Normals;
	std::vector<math::vec3> m_Bitangents;
	std::vector<math::vec3> m_Tangents;
	std::vector<math::vec4>	m_BoneWeights;
	std::vector<math::vec4> m_BoneIndices;
	
	std::vector<Face> m_Faces;
	
	//내부 데이터로부터 생성된 Vertex, index 데이터 (CreatVertices를 호출하면 내부데이터를 가지고 만듦)
	std::vector<Vertex_SkeletalMesh> m_Vertices;
	std::shared_ptr<Skeleton> m_Skeleton;
	std::vector<u32> m_Indices;

	//GPU데이터
	VertexBuffer* m_VertexBuffer{ nullptr };
	IndexBuffer* m_IndexBuffer{ nullptr };

	//Parent Model : 없을 수도 있음.
	Model* m_pModel{ nullptr };
};

class Model{
public:
	bool LoadModel(std::string FilePath);
	std::string GetName() { return m_Name; }
	const std::vector<Mesh*>& GetVecMesh() const { return m_vecMesh; }
	std::shared_ptr<Skeleton> GetSkeleton() { return m_Skeleton; }

private:
	void ReadSkeleton(const aiScene* pScene);

private:
	std::shared_ptr<Skeleton> m_Skeleton;

	std::string m_Name;
	std::vector<Mesh*> m_vecMesh;
	std::vector<Material*> m_vecMaterialSlot;
};